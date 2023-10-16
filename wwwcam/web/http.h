#ifndef HTTP_WEB__H_INC
#define HTTP_WEB__H_INC

#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wby_con;
struct wby_header;

typedef struct http_context_st http_context_t;
typedef struct http_server_st http_server_t;

http_server_t* http_server_new(const char* addr, int port);
void http_server_free(http_server_t* srv);
int http_server_get(http_server_t* srv, const char* path, int (*get)(http_context_t*, void*), void* ctx);
int http_server_post(http_server_t* srv, const char* path, int (*get)(http_context_t*, const void*, size_t, void*), void* ctx);
int http_server_redirect(http_server_t* srv, const char* path, const char* to);
int http_server_static_file(http_server_t* srv, const char* path, const char* filepath);
int http_server_static_path(http_server_t* srv, const char* path, const char* dirpath);
int http_server_atexit(http_server_t* srv, void (*action)(void*), void* ptr);

int http_server_start(http_server_t* srv);
int http_server_update(http_server_t* srv);
int http_server_stop(http_server_t* srv);

int http_set_status(http_context_t* ctx, int status);
int http_write(http_context_t* ctx, const void* ptr, int len);
const char* http_get_header(http_context_t* ctx, const char* name);
const char* http_get_query_var(http_context_t* ctx, const char* name);
int http_set_header(http_context_t* ctx, const char* name, const char* value);
const char* http_uri(http_context_t* ctx);
const char* http_server_mime_type(const char* filename);


#ifdef __cplusplus
}
#endif

#endif

