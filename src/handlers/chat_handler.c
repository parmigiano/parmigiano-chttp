#include "handlers.h"
#include "postgres/postgres_users.h"

#include <libchttpx/libchttpx.h>

chttpx_response_t chat_i_handler(chttpx_request_t* req)
{
    user_info_t* identity = (user_info_t*)req->context;
}