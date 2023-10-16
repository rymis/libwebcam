#define WBY_IMPLEMENTATION 1

#include "http.h"
#include "web.h"

#define PATH_MAX 256

struct free_list_st {
    void *ptr;
    void (*action)(void*);
    struct free_list_st *next;
};
typedef struct free_list_st free_list_t;

struct http_context_st {
    struct wby_con* con;
    const struct wby_request* request;
    char* body;
    size_t body_len;
    int status;
    struct wby_header* headers;
    size_t header_count;
    char query_param[1024];
};

struct http_handler_st {
    const char* path;
    int (*get)(http_context_t*, void*);
    int (*post)(http_context_t*, const void*, size_t, void*);
    void* ctx;
};
typedef struct http_handler_st http_handler_t;

struct http_server_st {
    http_handler_t* handlers;
    size_t handler_count;
    struct wby_config config;
    struct wby_server server;
    void *srv_memory;
    char* addr;
    free_list_t* atexit;
};

static void free_list_free(free_list_t* lst)
{
    free_list_t* p;
    while (lst) {
        lst->action(lst->ptr);
        p = lst;
        lst = lst->next;
        free(p);
    }
}

#define HTTP_SERVER_FREE(srv) \
    do { \
        if (srv) { \
            free_list_free(srv->atexit); \
            free(srv->addr); \
            free(srv->srv_memory); \
            free(srv->handlers); \
            free(srv); \
        } \
    } while (0)

static int http_dispatch(struct wby_con *connection, void *pArg);
http_server_t* http_server_new(const char* addr, int port)
{
    http_server_t* ctx;
    size_t needed_memory;

    ctx = (http_server_t*)calloc(1, sizeof(http_server_t));
    if (!ctx) {
        return NULL;
    }

    if (!addr) {
        ctx->addr = strdup("0.0.0.0");
    } else {
        ctx->addr = strdup(addr);
    }
    if (!ctx->addr) {
        HTTP_SERVER_FREE(ctx);
        return NULL;
    }

    /* setup config */
    ctx->config.address = ctx->addr;
    ctx->config.port = port;
    ctx->config.connection_max = 32;
    ctx->config.request_buffer_size = 8192;
    ctx->config.io_buffer_size = 16384;
    ctx->config.dispatch = http_dispatch;
    ctx->config.userdata = ctx;
    // ctx->config.ws_connect = websocket_connect;
    // ctx->config.ws_connected = websocket_connected;
    // ctx->config.ws_frame = websocket_frame;
    // ctx->config.ws_closed = websocket_closed;

    /* compute and allocate needed memory and start server */
    wby_init(&ctx->server, &ctx->config, &needed_memory);
    ctx->srv_memory = calloc(needed_memory, 1);
    if (!ctx->srv_memory) {
        HTTP_SERVER_FREE(ctx);
        return NULL;
    }

    return ctx;
}

int http_server_start(http_server_t* srv)
{
    return wby_start(&srv->server, srv->srv_memory);
}

int http_server_update(http_server_t* srv)
{
    wby_update(&srv->server);
    return 0;
}

int http_server_stop(http_server_t* srv)
{
    wby_stop(&srv->server);

    return 0;
}

void http_server_free(http_server_t* srv)
{
    HTTP_SERVER_FREE(srv);
}

int http_server_atexit(http_server_t* srv, void (*action)(void*), void* ptr)
{
    free_list_t* p = (free_list_t*)calloc(sizeof(free_list_t), 1);
    if (!p) {
        return -1;
    }
    p->ptr = ptr;
    p->action = action;
    p->next = srv->atexit;
    srv->atexit = p;

    return 0;
}

static int resp_init(http_context_t* ctx, struct wby_con* con)
{
    ctx->con = con;
    ctx->request = &con->request;
    ctx->status = 200;

    ctx->body = NULL;
    ctx->body_len = 0;
    ctx->headers = (struct wby_header*)calloc(1, sizeof(struct wby_header));
    if (!ctx->headers) {
        return -1;
    }

    ctx->headers[0].name = strdup("content-type");
    if (!ctx->headers[0].name) {
        free(ctx->headers);
        return -1;
    }

    ctx->headers[0].value = strdup("text/html");
    if (!ctx->headers[0].value) {
        free((char*)ctx->headers[0].name);
        free(ctx->headers);
        return -1;
    }
    ctx->header_count = 1;

    return 0;
}

static void resp_free(http_context_t* ctx)
{
    for (size_t i = 0; i < ctx->header_count; ++i) {
        free((char*)ctx->headers[i].name);
        free((char*)ctx->headers[i].value);
    }
    free(ctx->body);
}

int http_write(http_context_t* ctx, const void* ptr, int len)
{
    if (len < 0) {
        len = strlen((const char*)ptr);
    }

    if (ctx->body) {
        void* tmp = realloc(ctx->body, ctx->body_len + len);
        if (!tmp) {
            return -1;
        }
        ctx->body = (char*)tmp;
    } else {
        ctx->body = (char*)malloc(len);
        if (!ctx->body) {
            return -1;
        }
    }

    memcpy(ctx->body + ctx->body_len, ptr, len);
    ctx->body_len += len;

    return len;
}

const char* http_get_header(http_context_t* ctx, const char* name)
{
    return wby_find_header(ctx->con, name);
}

const char* http_get_query_var(http_context_t* ctx, const char* name)
{
    int r = wby_find_query_var(ctx->request->query_params, name, ctx->query_param, sizeof(ctx->query_param));
    if (r < 0) {
        return NULL;
    }

    if (r < (int)sizeof(ctx->query_param)) {
        ctx->query_param[r] = 0;
    } else {
        ctx->query_param[sizeof(ctx->query_param) - 1] = 0;
    }

    return ctx->query_param;
}

int http_set_header(http_context_t* ctx, const char* name, const char* value)
{
    // Search for the header:
    for (size_t i = 0; i < ctx->header_count; ++i) {
        if (strcasecmp(ctx->headers[i].name, name) == 0) {
            char* val = strdup(value);
            if (!val) {
                return -1;
            }
            ctx->headers[i].value = val;
            return 0;
        }
    }

    void *tmp = realloc(ctx->headers, (ctx->header_count + 1) * sizeof(struct wby_header));
    if (!tmp) {
        return -1;
    }
    ctx->headers = (struct wby_header*)tmp;

    ctx->headers[ctx->header_count].name = strdup(name);
    if (!ctx->headers[ctx->header_count].name) {
        return -1;
    }
    ctx->headers[ctx->header_count].value = strdup(value);
    if (!ctx->headers[ctx->header_count].value) {
        free((char*)ctx->headers[ctx->header_count].name);
        return -1;
    }
    ++ctx->header_count;

    return 0;
}

static int http_dispatch(struct wby_con *connection, void *pArg)
{
    http_server_t* srv = (http_server_t*)pArg;
    http_handler_t* handlers = srv->handlers;
    http_context_t resp;
    char* body = NULL;
    int body_len = 0;
    int post = 0;
    int rv = 0;

    printf("%s %s\n", connection->request.method, connection->request.uri);

    if (strcasecmp(connection->request.method, "GET") == 0) {
        post = 0;
    } else if (strcasecmp(connection->request.method, "POST") == 0) {
        post = 1;
    } else {
        const char* err = "unsupported method";
        struct wby_header ctype;
        ctype.name = "content-type";
        ctype.value = "text/plain";
        wby_response_begin(connection, 400, strlen(err), &ctype, 1);
        wby_write(connection, err, strlen(err));
        wby_response_end(connection);

        return 0;
    }

    int match_idx = -1;
    for (size_t i = 0; i < srv->handler_count; ++i) {
        size_t l = strlen(handlers[i].path);
        if (strncasecmp(handlers[i].path, connection->request.uri, l) == 0) {
            if (post && handlers[i].post) {
                match_idx = i;
                break;
            }

            if (!post && handlers[i].get) {
                match_idx = i;
                break;
            }
        }
    }

    if (match_idx < 0) {
        const char* err = "Not found";
        struct wby_header ctype;
        ctype.name = "content-type";
        ctype.value = "text/plain";
        wby_response_begin(connection, 404, strlen(err), &ctype, 1);
        wby_write(connection, err, strlen(err));
        wby_response_end(connection);

        return 0;
    }

    if (post && connection->request.content_length > 0) {
        body = malloc(connection->request.content_length);
        if (!body) {
            return -1;
        }
        body_len = connection->request.content_length;

        for (int pos = 0; pos < connection->request.content_length;) {
            int l = wby_read(connection, body + pos, connection->request.content_length - pos);
            if (l <= 0) {
                free(body);
                return -1;
            }

            pos += l;
        }
    }

    if (resp_init(&resp, connection) < 0) {
        free(body);
        return -1;
    }

    if (post) {
        rv = handlers[match_idx].post(&resp, body, body_len, handlers[match_idx].ctx);
    } else {
        rv = handlers[match_idx].get(&resp, handlers[match_idx].ctx);
    }
    free(body);
    if (rv < 0) {
        resp_free(&resp);
        return -1;
    }

    wby_response_begin(connection, resp.status, resp.body_len, resp.headers, resp.header_count);
    wby_write(connection, resp.body, resp.body_len);
    wby_response_end(connection);
    resp_free(&resp);

    return 0;
}

static int add_handler(http_server_t* srv, const char* path, void* ctx)
{
    void *tmp = realloc(srv->handlers, (srv->handler_count + 1) * sizeof(http_handler_t));
    if (!tmp) {
        return -1;
    }
    srv->handlers = (http_handler_t*)(tmp);

    srv->handlers[srv->handler_count].path = strdup(path);
    if (!srv->handlers[srv->handler_count].path) {
        return -1;
    }
    srv->handlers[srv->handler_count].ctx = ctx;
    ++srv->handler_count;

    return 0;
}

int http_set_status(http_context_t* ctx, int status)
{
    ctx->status = status;
    return 0;
}

int http_server_get(http_server_t* srv, const char* path, int (*get)(http_context_t*, void*), void* ctx)
{
    if (add_handler(srv, path, ctx) < 0) {
        return -1;
    }

    srv->handlers[srv->handler_count - 1].get = get;

    return 0;
}

int http_server_post(http_server_t* srv, const char* path, int (*post)(http_context_t*, const void*, size_t, void*), void* ctx)
{
    if (add_handler(srv, path, ctx) < 0) {
        return -1;
    }

    srv->handlers[srv->handler_count - 1].post = post;

    return 0;
}

static int serve_static_file(http_context_t* ctx, const char* path)
{
    FILE* f = fopen(path, "rb");
    char buf[1024];

    if (!f) {
        printf("FILE: %s is not found\n", path);
        http_set_status(ctx, 404);
        http_set_header(ctx, "content-type", "text/plain");
        http_write(ctx, "Not found", -1);
        return 0;
    }

    http_set_header(ctx, "content-type", http_server_mime_type(path));

    for (;;) {
        int l = fread(buf, 1, sizeof(buf), f);
        if (l <= 0) {
            break;
        }

        http_write(ctx, buf, l);
    }
    fclose(f);

    return 0;
}

static int static_file_get(http_context_t* ctx, void* arg)
{
    const char* path = (const char*)arg;
    return serve_static_file(ctx, path);
}

int http_server_static_file(http_server_t* srv, const char* path, const char* filepath)
{
    char* fpath = strdup(filepath);
    if (!fpath) {
        return -1;
    }

    if (http_server_atexit(srv, free, fpath) < 0) {
        free(fpath);
        return -1;
    }

    return http_server_get(srv, path, static_file_get, fpath);
}

struct dir_context {
    char path[PATH_MAX + 1];
    int uri_skip;
};

static int static_dir_get(http_context_t* ctx, void* arg)
{
    const struct dir_context* dctx = (const struct dir_context*)arg;
    const char* path = dctx->path;
    const char* uri = http_uri(ctx);
    char fpath[PATH_MAX + 1];

    if (strlen(uri) < (size_t)dctx->uri_skip) {
        http_set_status(ctx, 500); // Internal server error
        http_set_header(ctx, "content-type", "text/plain");
        http_write(ctx, "Invalid route", -1);
        return 0;
    }
    uri = uri + dctx->uri_skip;
    while (uri[0] == '/') {
        ++uri;
    }

    if (strcmp(uri, "") == 0) {
        snprintf(fpath, sizeof(fpath), "%sindex.html", path);
    } else {
        snprintf(fpath, sizeof(fpath), "%s%s", path, uri);
    }

    serve_static_file(ctx, fpath);

    return 0;
}

int http_server_static_path(http_server_t* srv, const char* path, const char* dirpath)
{
    int l = strlen(dirpath);
    if (l >= PATH_MAX) {
        return -1;
    }

    struct dir_context* dctx = calloc(sizeof(struct dir_context), 1);
    if (!dctx) {
        return -1;
    }

    if (http_server_atexit(srv, free, dctx) < 0) {
        free(dctx);
        return -1;
    }

    // Remove trailing slashes and add exactly one slash:
    snprintf(dctx->path, PATH_MAX, "%s", dirpath);
    for (; l > 0 && dctx->path[l - 1] == '/'; --l) {
        dctx->path[l - 1] = 0;
    }

    if (l >= PATH_MAX) {
        return -1;
    }

    dctx->path[l] = '/';
    dctx->path[++l] = 0;
    dctx->uri_skip = strlen(path);

    return http_server_get(srv, path, static_dir_get, dctx);
}

static struct MIME {
    const char* ext;
    const char* mime;
} MIME_TYPES[] = {
    { "html", "text/html" },
    { "htm", "text/html" },
    { "shtml", "text/html" },
    { "css", "text/css" },
    { "xml", "text/xml" },
    { "gif", "image/gif" },
    { "jpeg", "image/jpeg" },
    { "jpg", "image/jpeg" },
    { "js", "application/javascript" },
    { "atom", "application/atom+xml" },
    { "rss", "application/rss+xml" },
    { "mml", "text/mathml" },
    { "txt", "text/plain" },
    { "jad", "text/vnd.sun.j2me.app-descriptor" },
    { "wml", "text/vnd.wap.wml" },
    { "htc", "text/x-component" },
    { "avif", "image/avif" },
    { "png", "image/png" },
    { "svg", "image/svg+xml" },
    { "svgz", "image/svg+xml" },
    { "tif", "image/tiff" },
    { "tiff", "image/tiff" },
    { "wbmp", "image/vnd.wap.wbmp" },
    { "webp", "image/webp" },
    { "ico", "image/x-icon" },
    { "jng", "image/x-jng" },
    { "bmp", "image/x-ms-bmp" },
    { "woff", "font/woff" },
    { "woff2", "font/woff2" },
    { "jar", "application/java-archive" },
    { "war", "application/java-archive" },
    { "ear", "application/java-archive" },
    { "json", "application/json" },
    { "hqx", "application/mac-binhex40" },
    { "doc", "application/msword" },
    { "pdf", "application/pdf" },
    { "ps", "application/postscript" },
    { "eps", "application/postscript" },
    { "ai", "application/postscript" },
    { "rtf", "application/rtf" },
    { "m3u8", "application/vnd.apple.mpegurl" },
    { "kml", "application/vnd.google-earth.kml+xml" },
    { "kmz", "application/vnd.google-earth.kmz" },
    { "xls", "application/vnd.ms-excel" },
    { "eot", "application/vnd.ms-fontobject" },
    { "ppt", "application/vnd.ms-powerpoint" },
    { "odg", "application/vnd.oasis.opendocument.graphics" },
    { "odp", "application/vnd.oasis.opendocument.presentation" },
    { "ods", "application/vnd.oasis.opendocument.spreadsheet" },
    { "odt", "application/vnd.oasis.opendocument.text" },
    { "wmlc", "application/vnd.wap.wmlc" },
    { "wasm", "application/wasm" },
    { "7z", "application/x-7z-compressed" },
    { "cco", "application/x-cocoa" },
    { "jardiff", "application/x-java-archive-diff" },
    { "jnlp", "application/x-java-jnlp-file" },
    { "run", "application/x-makeself" },
    { "pl", "application/x-perl" },
    { "pm", "application/x-perl" },
    { "prc", "application/x-pilot" },
    { "pdb", "application/x-pilot" },
    { "rar", "application/x-rar-compressed" },
    { "rpm", "application/x-redhat-package-manager" },
    { "sea", "application/x-sea" },
    { "swf", "application/x-shockwave-flash" },
    { "sit", "application/x-stuffit" },
    { "tcl", "application/x-tcl" },
    { "tk", "application/x-tcl" },
    { "der", "application/x-x509-ca-cert" },
    { "pem", "application/x-x509-ca-cert" },
    { "crt", "application/x-x509-ca-cert" },
    { "xpi", "application/x-xpinstall" },
    { "xhtml", "application/xhtml+xml" },
    { "xspf", "application/xspf+xml" },
    { "zip", "application/zip" },
    { "bin", "application/octet-stream" },
    { "exe", "application/octet-stream" },
    { "dll", "application/octet-stream" },
    { "deb", "application/octet-stream" },
    { "dmg", "application/octet-stream" },
    { "iso", "application/octet-stream" },
    { "img", "application/octet-stream" },
    { "msi", "application/octet-stream" },
    { "msp", "application/octet-stream" },
    { "msm", "application/octet-stream" },
    { "mid", "audio/midi" },
    { "midi", "audio/midi" },
    { "kar", "audio/midi" },
    { "mp3", "audio/mpeg" },
    { "ogg", "audio/ogg" },
    { "m4a", "audio/x-m4a" },
    { "ra", "audio/x-realaudio" },
    { "3gpp", "video/3gpp" },
    { "3gp", "video/3gpp" },
    { "ts", "video/mp2t" },
    { "mp4", "video/mp4" },
    { "mpeg", "video/mpeg" },
    { "mpg", "video/mpeg" },
    { "mov", "video/quicktime" },
    { "webm", "video/webm" },
    { "flv", "video/x-flv" },
    { "m4v", "video/x-m4v" },
    { "mng", "video/x-mng" },
    { "asx", "video/x-ms-asf" },
    { "asf", "video/x-ms-asf" },
    { "wmv", "video/x-ms-wmv" },
    { "avi", "video/x-msvideo" },
    { NULL, NULL }
};

static const char* mime_db_search(const char* ext)
{
    for (size_t i = 0; MIME_TYPES[i].ext; ++i) {
        if (strcasecmp(MIME_TYPES[i].ext, ext) == 0) {
            return MIME_TYPES[i].mime;
        }
    }

    return "application/octet-string";
}

const char* http_server_mime_type(const char* filename)
{
    int l = strlen(filename);
    for (int i = l; i--;) {
        if (filename[i] == '.') {
            return mime_db_search(filename + i + 1);
        }
    }

    return "application/octet-string";
}

static int redirect_handler(http_context_t* ctx, void* arg)
{
    const char* url = (const char*)arg;
    http_set_status(ctx, 307); // Temporary redirt
    http_set_header(ctx, "location", url);

    return 0;
}

int http_server_redirect(http_server_t* srv, const char* path, const char* to)
{
    char* url = strdup(to);
    if (!url) {
        return -1;
    }

    if (http_server_atexit(srv, free, url) < 0) {
        free(url);
        return -1;
    }

    return http_server_get(srv, path, redirect_handler, url);
}

const char* http_uri(http_context_t* ctx)
{
    return ctx->request->uri;
}
