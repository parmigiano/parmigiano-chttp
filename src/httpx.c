#include "httpx.h"

#include "routes.h"
#include "middlewarex.h"
#include "redis/redis.h"
#include "postgres/postgres.h"

#include <libchttpx/libchttpx.h>

httpx_server_t* http_server = NULL;

void http_init(void)
{
    http_server = (httpx_server_t*)calloc(1, sizeof(httpx_server_t));
    if (!http_server)
    {
        fprintf(stderr, "calloc failed\n");
        exit(1);
    }

    /* cHTTPX Server */
    chttpx_serv_t serv = {0};

    size_t max_clients = 65536;

    if (cHTTPX_Init(&serv, HTTPX_SERVER_PORT, &max_clients) != 0)
    {
        printf("Failed to start server\n");
        return;
    }

    /* Timeouts */
    serv.read_timeout_sec = 120;
    serv.write_timeout_sec = 120;
    serv.idle_timeout_sec = 90;

    /* Initial redis connect */
    if (!redis_conn())
    {
        fprintf(stderr, "Redis error\n");
        exit(1);
    }

    /* Inital database, migrations */
    PGconn* conn = db_conn();
    if (!conn)
    {
        fprintf(stderr, "Failed to connect to database\n");
        exit(1);
    }
    http_server->conn = conn;

    run_migrations(conn);

    /* Initial middlewares */
    cHTTPX_MiddlewareRecovery();
    cHTTPX_MiddlewareRateLimiter(5, 1);
    cHTTPX_MiddlewareUse(language_middleware);
    cHTTPX_MiddlewareUse(authenticate_middleware);
    cHTTPX_MiddlewareUse(email_confirmed_middleware);

    /* Initial routes */
    routes();

    /* At the very end, to start listening to incoming requests from users. */
    cHTTPX_Listen();

    /* Shutdown server */
    cHTTPX_Shutdown();

    free(http_server);
}