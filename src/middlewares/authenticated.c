#include "httpx.h"
#include "middlewarex.h"

#include "redis/redis_session.h"
#include "postgres/postgres_users.h"

#include <string.h>
#include <libchttpx/libchttpx.h>

chttpx_middleware_result_t authenticate_middleware(chttpx_request_t* req, chttpx_response_t* res)
{
    if (strstr(req->path, "auth/login") != NULL || strstr(req->path, "auth/create") != NULL)
    {
        return next;
    }

    const char* session_id = NULL;

    const char* authenticated_header = cHTTPX_Header(req, "Authorization");
    if (authenticated_header && strncasecmp(authenticated_header, "Bearer ", 7) == 0)
    {
        session_id = authenticated_header + 7;
    }

    if (!session_id)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusUnauthorized,
                              "{\"message\": \"пожалуйста, подключитесь к учетной записи\"}");
        return out;
    }

    session_t* session = redis_session_get(session_id);
    if (!session)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusUnauthorized,
                              "{\"message\": \"пожалуйста, подключитесь к учетной записи\"}");
        return out;
    }

    user_info_t* user = db_user_info_get_by_uid(http_server->conn, session->user_uid);
    if (!user)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusUnauthorized,
                              "{\"message\": \"пожалуйста, подключитесь к учетной записи\"}");
        return out;
    }

    req->context = user;

    redis_session_refresh(session_id);
    return next;
}