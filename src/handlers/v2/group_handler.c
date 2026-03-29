#include "handlers.h"

typedef struct
{
    char* name;
    chttpx_number_array_t chat_ids;
} group_chats_create_t;

void group_chats_create_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    /* Context in request */
    auth_token_t* ctx = (auth_token_t*)req->context;

    group_chats_create_t payload = {0};

    chttpx_validation_t fields[] = {
        chttpx_validation_string("name", &payload.name, true, 0, 20, VALIDATOR_NONE),
        {.name = "chat_ids", .type = FIELD_NUMBER_ARRAY, .target = &payload.chat_ids},
    };

    if (!cHTTPX_Parse(req, fields, (sizeof(fields) / sizeof(fields[0]))))
        goto errorjson;

    if (!cHTTPX_Validate(req, fields, (sizeof(fields) / sizeof(fields[0])), ctx->lang))
        goto errorjson;

cleanup:
    free(payload.name);

    return;

errorjson:
    *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);

    goto cleanup;
}