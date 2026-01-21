#ifndef HANDLERS_H
#define HANDLERS_H

#include "postgres/postgres_users.h"

#include <libchttpx/libchttpx.h>

typedef struct {
    user_info_t* user;
    const char* lang;
} auth_token_t;

void auth_login_handler(chttpx_request_t *req, chttpx_response_t *res);

void auth_create_handler(chttpx_request_t *req, chttpx_response_t *res);

void auth_verify_handler(chttpx_request_t* req, chttpx_response_t *res);

void chat_i_handler(chttpx_request_t* req, chttpx_response_t *res);

#endif