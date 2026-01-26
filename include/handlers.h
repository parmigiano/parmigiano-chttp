#ifndef HANDLERS_H
#define HANDLERS_H

#include "postgres/postgres_users.h"

#include <libchttpx/libchttpx.h>

typedef struct {
    user_info_t* user;
    char* lang;
} auth_token_t;

/* Docs handlers */
void swagger_json_handler_v2(chttpx_request_t* req, chttpx_response_t* res);
void swagger_gui_handler_v2(chttpx_request_t* req, chttpx_response_t* res);

/* Auth handlers */
void auth_login_handler_v2(chttpx_request_t *req, chttpx_response_t *res);
void auth_create_handler_v2(chttpx_request_t *req, chttpx_response_t *res);
void auth_verify_handler_v2(chttpx_request_t* req, chttpx_response_t *res);
void auth_delete_handler_v2(chttpx_request_t* req, chttpx_response_t* res);

/* User handlers */
void user_me_handler_v2(chttpx_request_t* req, chttpx_response_t* res);
void user_upload_avatar_handler_v2(chttpx_request_t *req, chttpx_response_t *res);
void user_get_profile_handler_v2(chttpx_request_t *req, chttpx_response_t *res);
void user_update_profile_handler_v2(chttpx_request_t *req, chttpx_response_t *res);

#endif