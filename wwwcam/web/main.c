#include <string.h>
#include <stdio.h>
#include "http.h"

int handle_hello(http_context_t* ctx, void* userdata)
{
    http_write(ctx, "<h1>Hello, World!</h1>", -1);
    return 0;
}


int main() {
    /* setup config */
    http_server_t *srv = http_server_new("127.0.0.1", 8888);
    if (!srv) {
        fprintf(stderr, "Error: can't create server!\n");
        return 1;
    }

    http_server_get(srv, "/hello.html", handle_hello, NULL);
    http_server_static_file(srv, "/", "index.html");
    http_server_start(srv);

    while (1) {
        http_server_update(srv);
    }

    http_server_stop(srv);

    http_server_free(srv);
    return 0;
}
