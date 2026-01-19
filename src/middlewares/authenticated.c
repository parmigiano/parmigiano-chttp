#include "middlewarex.h"

#include <libchttpx/libchttpx.h>
#include <string.h>

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
        session_id = cHTTPX_Cookie(req, "auth-token");
    }

    if (!session_id)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusUnauthorized,
                              "{\"message\": \"пожалуйста, подключитесь к учетной записи\"}");
        return out;
    }

    // session_t* session = redis_session_get(session_id);
    // if (!session)
    // {
    //     *res = cHTTPX_ResJson(cHTTPX_StatusUnauthorized, "{\"message\": \"пожалуйста,
    //     подключитесь к учетной записи\"}"); return out;
    // }

    // user_t* user = db_get_user_by_uid(session->user_uid);
    // if (!user)
    // {
    //     *res = cHTTPX_ResJson(cHTTPX_StatusUnauthorized, "{\"message\": \"пожалуйста,
    //     подключитесь к учетной записи\"}"); return out;
    // }

    // auth_token_t *auth = malloc(sizeof(auth_token_t));
    // auth->user = user;
    // strncpy(auth->session_id, session_id, sizeof(auth->session_id));

    // req->context = auth;
    /* auth_token_t *identity = (auth_token_t *)req->context; */

    // redis_session_refresh(session_id);
    return next;
}