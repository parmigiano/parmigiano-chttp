#include "handlers.h"

chttpx_response_t auth_signin_handler(chttpx_request_t *req)
{
    return cHTTPX_ResJson(200, "{\"message\": \"%s\"}", "welcome");
}