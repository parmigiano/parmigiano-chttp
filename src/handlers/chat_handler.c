#include "handlers.h"
#include "postgres/postgres_users.h"

#include <libchttpx/libchttpx.h>

void chat_i_handler(chttpx_request_t* req, chttpx_response_t* res)
{
    user_info_t* identity = (user_info_t*)req->context;
    *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", cHTTPX_i18n_t("error.no-active-mail-confirmation", "ru"));
}