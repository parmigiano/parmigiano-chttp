#include "httpx.h"

#include "routes.h"
#include "postgresdb.h"
#include "middlewarex.h"

#include <libchttpx/libchttpx.h>

static void _http_cors();

void http_init(void)
{
    /* Current server */
    httpx_server_t serv_h;

    /* cHTTPX Server */
    chttpx_serv_t serv;

    if (cHTTPX_Init(&serv, HTTPX_SERVER_PORT) != 0)
    {
        printf("Failed to start server\n");
        return;
    }

    /* Timeouts */
    serv.read_timeout_sec = 120;
    serv.write_timeout_sec = 120;
    serv.idle_timeout_sec = 90;

    /* Inital database, migrations */
    PGconn* conn = db_conn();

    run_migrations(conn);

    /* Inital cors */
    _http_cors();

    /* Initial middlewares */
    cHTTPX_MiddlewareRateLimiter(5, 1);
    cHTTPX_MiddlewareUse(authenticate_middleware);
    cHTTPX_MiddlewareUse(email_confirmed_middleware);

    /* Initial routes */
    routes();

    cHTTPX_Listen();
}

static void _http_cors()
{
    const char* allowed_origins[] = {
        "https://parmigianochat.ru",
        "http://localhost:80",
    };

    cHTTPX_Cors(allowed_origins, (sizeof(allowed_origins) / sizeof(allowed_origins[0])), NULL,
                NULL);
}