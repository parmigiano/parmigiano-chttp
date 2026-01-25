#include "handlers.h"

#include "s3.h"
#include "httpx.h"

void user_me_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
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
                          ctx->user->id, ctx->user->created_at, ctx->user->user_uid, ctx->user->avatar ? ctx->user->avatar : "", ctx->user->name ? ctx->user->name : "",
                          ctx->user->username ? ctx->user->username : "", ctx->user->username_visible ? "true" : "false",
                          ctx->user->email ? ctx->user->email : "", ctx->user->email_visible ? "true" : "false",
                          ctx->user->email_confirm ? "true" : "false", ctx->user->phone ? ctx->user->phone : "",
                          ctx->user->phone_visible ? "true" : "false", ctx->user->overview ? ctx->user->overview : "");

cleanup:
    if (req->context)
    {
        auth_token_t* ctx = (auth_token_t*)req->context;

        if (ctx->user)
        {
            db_user_info_free(ctx->user);
            ctx->user = NULL;
        }

        if (ctx->lang)
            free(ctx->lang);
        free(ctx);

        req->context = NULL;
    }

    return;
}

void user_upload_avatar_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

    if (req->filename[0] == '\0')
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.no-data-to-process", ctx->lang));
        goto cleanup;
    }

    FILE* f = fopen(req->filename, "rb");
    if (!f)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.open-temporary-file", ctx->lang));
        goto cleanup;
    }

    s3_config_t s3_config = {
        .endpoint = getenv("S3_ENDPOINT"),
        .bucket = getenv("S3_BUCKET"),
        .access_key = getenv("S3_ACCESS_KEY"),
        .secret_key = getenv("S3_SECRET_KEY"),
        .region = getenv("S3_REGION"),
    };

    /* save to s3 storage */
    char* url = s3_upload_file(f, req->filename, &s3_config);
    fclose(f);

    if (!url)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.save-file", ctx->lang));
        goto cleanup;
    }

    /* update in database */
    db_result_t user_upd_result = db_user_profile_upd_avatar_by_uid(http_server->conn, ctx->user->user_uid, url);

    switch (user_upd_result)
    {
    case DB_TIMEOUT:
        *res = cHTTPX_ResJson(cHTTPX_StatusConnectionTimedOut, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.database-connection-timeout", ctx->lang));
        goto cleanup;

    case DB_DUPLICATE:
        *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.repeating-data-request", ctx->lang));
        goto cleanup;

    case DB_ERROR:
        *res = cHTTPX_ResJson(cHTTPX_StatusConflict, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.perform-database-operation", ctx->lang));
        goto cleanup;
    }

    *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", url);

cleanup:
    if (req->context)
    {
        auth_token_t* ctx = (auth_token_t*)req->context;

        if (ctx->user)
        {
            db_user_info_free(ctx->user);
            ctx->user = NULL;
        }

        if (ctx->lang)
            free(ctx->lang);
        free(ctx);

        req->context = NULL;
    }

    if (url)
        free(url);

    if (req->filename[0] != '\0')
        remove(req->filename);

    return;
}

void user_get_profile_handler_v2(chttpx_request_t *req, chttpx_response_t *res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

    /* Get from params -> user_uid */
    const char* user_uid_param = cHTTPX_Param(req, "user_uid");
    if (!user_uid_param)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusNotFound, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.user-not-found", ctx->lang));
        goto cleanup;
    }

    int user_uid = atoi(user_uid_param);
    if (user_uid < 0)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusNotFound, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.user-not-found", ctx->lang));
        goto cleanup;
    }

    user_info_t* user = db_user_info_get_by_uid(http_server->conn, (uint64_t)user_uid);
    if (!user)
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
                          user->id, user->created_at, user->user_uid, user->avatar ? user->avatar : "", user->name ? user->name : "",
                          user->username ? user->username : "", user->username_visible ? "true" : "false",
                          user->email ? user->email : "", user->email_visible ? "true" : "false",
                          user->email_confirm ? "true" : "false", user->phone ? user->phone : "",
                          user->phone_visible ? "true" : "false", user->overview ? user->overview : "");

cleanup:
    if (req->context)
    {
        auth_token_t* ctx = (auth_token_t*)req->context;

        if (ctx->user)
        {
            db_user_info_free(ctx->user);
            ctx->user = NULL;
        }

        if (ctx->lang) free(ctx->lang);
        free(ctx);

        req->context = NULL;
    }

    if (user)
    {
        db_user_info_free(user);
        user = NULL;
    }

    return;
}