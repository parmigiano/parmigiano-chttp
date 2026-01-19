#ifndef HANDLERS_H
#define HANDLERS_H

#include <libchttpx/libchttpx.h>

chttpx_response_t auth_login_handler(chttpx_request_t *req);

chttpx_response_t auth_create_handler(chttpx_request_t *req);

#endif