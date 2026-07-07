#include "logger.h"
#include "middlewarex.h"

#include <stdlib.h>
#include <libchttpx/libchttpx.h>

chttpx_middleware_result_t body_entry_middleware(chttpx_request_t* req, chttpx_response_t* res) {
    logger_info("Body (%zu bytes): %.*s\n", req->body_size, (int)req->body_size, (const char*)req->body);

    return next;
}