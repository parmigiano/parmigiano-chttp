#include "middlewarex.h"
#include "postgres/postgres_users.h"

#include <libchttpx/libchttpx.h>

chttpx_middleware_result_t email_confirmed_middleware(chttpx_request_t* req, chttpx_response_t* res)
{
    if (strstr(req->path, "auth/login") != NULL || strstr(req->path, "auth/create") != NULL)
    {
        return next;
    }

    user_info_t* identity = (user_info_t*)req->context;

    if (!identity || !identity->email_confirm)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusForbidden,
                              "{\"error\": \"пожалуйста, подтвердите свой email\"}");
        return out;
    }

    return next;
}
