#include "handlers.h"

void user_me_handler(chttpx_request_t* req, chttpx_response_t* res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

    if (!ctx->user)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusNotFound, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.user-not-found", ctx->lang));
        goto cleanup;
    }

    *res = cHTTPX_ResJson(cHTTPX_StatusOK,
                          "{"
                          "\"message\": {"
                          "\"id\": %lu,"
                          "\"created_at\": %ld,"
                          "\"user_uid\": %lu,"
                          "\"avatar\": \"%s\","
                          "\"name\": \"%s\","
                          "\"username\": \"%s\","
                          "\"username_visible\": %s,"
                          "\"email\": \"%s\","
                          "\"email_visible\": %s,"
                          "\"email_confirm\": %s,"
                          "\"phone\": \"%s\","
                          "\"phone_visible\": %s,"
                          "\"overview\": \"%s\""
                          "}"
                          "}",
                          ctx->user->id, ctx->user->created_at, ctx->user->avatar ? ctx->user->avatar : "", ctx->user->name ? ctx->user->name : "",
                          ctx->user->username ? ctx->user->username : "", ctx->user->username_visible ? "true" : "false",
                          ctx->user->email ? ctx->user->email : "", ctx->user->email_visible ? "true" : "false",
                          ctx->user->email_confirm ? "true" : "false", ctx->user->phone ? ctx->user->phone : "",
                          ctx->user->phone_visible ? "true" : "false", ctx->user->overview ? ctx->user->overview : "");

cleanup:
    if (req->context)
    {
        auth_token_t* ctx = (auth_token_t*)req->context;

        if (ctx->lang) free(ctx->lang);
        free(ctx);

        req->context = NULL;
    }

    return;
}