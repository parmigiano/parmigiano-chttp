#include "httpx.h"

#include "db.h"

#include <libchttpx/libchttpx.h>

static void _http_cors();
static void _http_middlewares();
static void _http_routes();

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

    /* Inital database */
    PGconn* conn = db_conn();

    /* Inital cors */
    _http_cors(&serv);

    /* Initial middlewares */
    _http_middlewares(&serv);

    /* Initial routes */
    _http_routes(&serv);

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

static void _http_middlewares()
{
}

static void _http_routes()
{
    // cHTTPX_Route(cHTTPX_MethodPost, "/auth/signin", auth_signin_h);
    // cHTTPX_Route(cHTTPX_MethodPost, "/auth/signup", auth_signup_h);
}