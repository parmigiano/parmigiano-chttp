#include "postgresdb.h"
#include "middlewarex.h"

#include <libchttpx/libchttpx.h>

chttpx_middleware_result_t email_confirmed_middleware(chttpx_request_t *req, chttpx_response_t *res)
{
    user_info_t *identity = (user_info_t *)req->context;

    if (!identity || !identity->email_confirmed)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusForbidden, "{\"error\": \"пожалуйста, подтвердите свой email\"}");
        return out;
    }

    return next;
}
