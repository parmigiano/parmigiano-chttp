#include "httpx.h"

#include "routes.h"
#include "middlewarex.h"
#include "redis/redis.h"
#include "postgres/postgres.h"

#include <libchttpx/libchttpx.h>

httpx_server_t* http_server = NULL;

static void _cors();

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

    /* Cors */
    _cors();

    /* Initial middlewares */
    cHTTPX_MiddlewareRecovery();
    cHTTPX_MiddlewareLogging();
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

static void _cors()
{
    const char* env_type = getenv("TYPE");
    if (!env_type)
        return;

    const char* allowed_origins_prod[1] = {
        "https://parmigianochat.ru",
    };

    const char* allowed_origins_dev[2] = {
        "http://localhost:8080",
        "http://localhost:80",
    };

    const char** allowed_origins;
    size_t origins_count;

    if (strcmp(env_type, "PROD") == 0)
    {
        allowed_origins = allowed_origins_prod;
        origins_count = sizeof(allowed_origins_prod) / sizeof(allowed_origins_prod[0]);
    }
    else
    {
        allowed_origins = allowed_origins_dev;
        origins_count = sizeof(allowed_origins_dev) / sizeof(allowed_origins_dev[0]);
    }

    cHTTPX_Cors(allowed_origins, origins_count, NULL,
                "Content-Type, Authorization, Accept-Language, X-Real-IP, X-Forwarded-For, X-Forwarded-Proto, Upgrade, Connection, Host");
}