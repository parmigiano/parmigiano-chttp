#ifndef HTTPX_H
#define HTTPX_H

#include <libpq-fe.h>

#define HTTPX_SERVER_PORT 80

typedef struct {
    PGconn* conn;
} httpx_server_t;

void http_init(void);

#endif