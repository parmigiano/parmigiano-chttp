#include "handlers.h"

#include "httpx.h"
#include "utilities.h"
#include "redis/redis.h"
#include "redis/redis_session.h"
#include "redis/redis_verifycode.h"
#include "postgres/postgres_users.h"

#include <time.h>
#include <stdbool.h>
#include <openssl/rand.h>

typedef struct
{
    char* email;
    char* password;
} auth_login_t;

typedef struct
{
    char* name;
    char* username;
    char* email;
    char* password;
} auth_create_t;

typedef struct
{
    char* email;
    int code;
} auth_verify_t;

void auth_login_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    /* Context in request */
    auth_token_t* ctx = (auth_token_t*)req->context;

    auth_login_t payload = {0};

    chttpx_validation_t fields[] = {
        chttpx_validation_string("email", &payload.email, true, 5, 254, VALIDATOR_EMAIL),
        chttpx_validation_string("password", &payload.password, false, 8, 16, VALIDATOR_NONE),
    };

    /* DB. get user core */
    user_core_t* user = NULL;

    if (!cHTTPX_Parse(req, fields, (sizeof(fields) / sizeof(fields[0]))))
        goto errorjson;

    if (!cHTTPX_Validate(req, fields, (sizeof(fields) / sizeof(fields[0])), ctx->lang))
        goto errorjson;

    /* Trim spaces */
    trim_space(payload.email);
    /* To lower email */
    to_lower(payload.email);

    user = db_user_core_get_by_email(http_server->conn, payload.email);
    if (!user)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusNotFound, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.user-not-found", ctx->lang));
        goto cleanup;
    }

    /* If user exist password, but not in payload -> 202 */
    if (user->password && user->password[0] != '\0' && !payload.password)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusAccepted, "{\"message\": \"%s\"}", cHTTPX_i18n_t("error.password-required", ctx->lang));
        goto cleanup;
    }

    if (user->password && user->password[0] != '\0' && !verify_password(payload.password, user->password))
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusNotFound, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.user-not-found", ctx->lang));
        goto cleanup;
    }

    /* Verify code */
    /* ----------- */
    int codetmp;

    if (redis_verifycode_get(payload.email, &codetmp))
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", cHTTPX_i18n_t("code-already-sent", ctx->lang));
        goto cleanup;
    }
    /* ----------- */
    /* Verify code */

    uint64_t code;
    if (RAND_bytes((unsigned char*)&code, sizeof(code)) != 1)
    {
        fprintf(stderr, "Failed to generate CODE\n");
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.code-generate-failed", ctx->lang));

        goto cleanup;
    }
    code = (code % 900000) + 100000;

    if (redis_verifycode_create(payload.email, code) != 0)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.code-save-failed", ctx->lang));
        goto cleanup;
    }

    /* Send email */
    /* ---------- */
    const char* html = "<body>"
                       "<p>%s <b>%s</b></p>"
                       "<p>%s</p>"
                       "<h2>%d</h2>"
                       "<p>%s...</p>"
                       "<p>%s</p>"
                       "</body>";

    char buffer[2048];
    snprintf(buffer, sizeof(buffer), html, cHTTPX_i18n_t("email.request-notice", ctx->lang), payload.email,
             cHTTPX_i18n_t("email.code-instruction", ctx->lang), code, cHTTPX_i18n_t("email.code-expire", ctx->lang),
             cHTTPX_i18n_t("email.footer", ctx->lang));

    if (send_email(payload.email, cHTTPX_i18n_t("email.subject", ctx->lang), buffer, NULL) != 0)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s: %s\"}", cHTTPX_i18n_t("error.sending-email", ctx->lang),
                              payload.email);
        goto cleanup;
    }
    /* ---------- */
    /* Send email */

    *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", cHTTPX_i18n_t("code-sent", ctx->lang));

cleanup:
    /* Free payloads */
    free(payload.email);
    if (payload.password)
        free(payload.password);

    if (user)
    {
        free(user->email);
        free(user->password);
        free(user);
        user = NULL;
    }

    if (req->context)
    {
        auth_token_t* ctx = (auth_token_t*)req->context;

        if (ctx->lang)
            free(ctx->lang);
        free(ctx);

        req->context = NULL;
    }

    return;

errorjson:
    *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);

    goto cleanup;
}

void auth_create_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

    auth_create_t payload = {0};

    chttpx_validation_t fields[] = {
        chttpx_validation_string("name", &payload.name, true, 2, 24, VALIDATOR_NONE),
        chttpx_validation_string("username", &payload.username, true, 4, 24, VALIDATOR_NONE),
        chttpx_validation_string("email", &payload.email, true, 5, 254, VALIDATOR_EMAIL),
        chttpx_validation_string("password", &payload.password, false, 8, 16, VALIDATOR_NONE),
    };

    /* DB. get user core */
    user_core_t* user = NULL;

    if (!cHTTPX_Parse(req, fields, (sizeof(fields) / sizeof(fields[0]))))
        goto errorjson;

    if (!cHTTPX_Validate(req, fields, (sizeof(fields) / sizeof(fields[0])), ctx->lang))
        goto errorjson;

    /* Verify code */
    /* ----------- */
    int codetmp;

    if (redis_verifycode_get(payload.email, &codetmp))
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusCreated, "{\"message\": \"%s\"}", cHTTPX_i18n_t("code-already-sent", ctx->lang));
        goto cleanup;
    }
    /* ----------- */
    /* Verify code */

    /* Trim spaces */
    trim_space(payload.username);
    trim_space(payload.email);

    /* Check symbols */
    if (!is_valid(payload.username))
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.invalid-username", ctx->lang));
        goto cleanup;
    }

    /* To lower email */
    to_lower(payload.email);

    /* Check user is exists */
    user = db_user_core_get_by_email(http_server->conn, payload.email);
    if (user)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.user-already-registered", ctx->lang));
        goto cleanup;
    }

    char* password_hash = NULL;

    /* Validation password and hash password */
    if (payload.password)
    {
        /* Trim space password */
        trim_space(payload.password);

        if (is_simple_password(payload.password))
        {
            *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.weak-password", ctx->lang));
            goto cleanup;
        }

        password_hash = hash_password(payload.password);
        if (!password_hash)
        {
            *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.password-hash-failed", ctx->lang));
            goto cleanup;
        }
    }

    uint64_t uid;
    if (RAND_bytes((unsigned char*)&uid, sizeof(uid)) != 1)
    {
        fprintf(stderr, "Failed to generate UID\n");
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.user-uid-generate-failed", ctx->lang));

        goto cleanup;
    }
    uid = (uid % 9000000000ULL) + 1000000000ULL;

    /* calloc user_core */
    user_core_t* user_core = calloc(1, sizeof(user_core_t));
    if (!user_core)
    {
        fprintf(stderr, "calloc failed\n");
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.something-went-wrong", ctx->lang));

        goto cleanup;
    }
    user_core->user_uid = uid;
    user_core->email = strdup(payload.email);
    user_core->password = password_hash ? strdup(password_hash) : NULL;

    /* calloc user_profile */
    user_profile_t* user_profile = calloc(1, sizeof(user_profile_t));
    if (!user_profile)
    {
        fprintf(stderr, "calloc failed\n");
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.something-went-wrong", ctx->lang));

        goto cleanup;
    }
    user_profile->user_uid = uid;
    user_profile->name = strdup(payload.name);
    user_profile->username = strdup(payload.username);
    user_profile->avatar = NULL;

    /* calloc user_profile_access */
    user_profile_access_t* user_profile_access = calloc(1, sizeof(user_profile_access_t));
    if (!user_profile_access)
    {
        fprintf(stderr, "calloc failed\n");
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.something-went-wrong", ctx->lang));

        goto cleanup;
    }
    user_profile_access->user_uid = uid;
    user_profile_access->username_visible = true;
    user_profile_access->email_visible = true;
    user_profile_access->phone_visible = false;

    /* calloc user_active */
    user_active_t* user_active = calloc(1, sizeof(user_active_t));
    if (!user_active)
    {
        fprintf(stderr, "calloc failed\n");
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.something-went-wrong", ctx->lang));

        goto cleanup;
    }
    user_active->user_uid = uid;

    db_result_t user_db_result = db_user_create(http_server->conn, user_core, user_profile, user_profile_access, user_active);

    /* free memory */
    free(user_core->email);
    free(user_core->password);
    free(user_core);
    user_core = NULL;

    free(user_profile->name);
    free(user_profile->username);
    free(user_profile);
    user_profile = NULL;

    free(user_profile_access);
    user_profile_access = NULL;
    free(user_active);
    user_active = NULL;

    switch (user_db_result)
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

    uint64_t code;
    if (RAND_bytes((unsigned char*)&code, sizeof(code)) != 1)
    {
        fprintf(stderr, "Failed to generate CODE\n");
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.code-generate-failed", ctx->lang));

        goto cleanup;
    }
    code = (code % 900000) + 100000;

    /* Send email */
    /* ---------- */
    const char* html = "<body>"
                       "<p>%s <b>%s</b></p>"
                       "<p>%s</p>"
                       "<h2>%d</h2>"
                       "<p>%s...</p>"
                       "<p>%s</p>"
                       "</body>";

    char buffer[2048];
    snprintf(buffer, sizeof(buffer), html, cHTTPX_i18n_t("email.request-notice", ctx->lang), payload.email,
             cHTTPX_i18n_t("email.code-instruction", ctx->lang), code, cHTTPX_i18n_t("email.code-expire", ctx->lang),
             cHTTPX_i18n_t("email.footer", ctx->lang));

    if (send_email(payload.email, cHTTPX_i18n_t("email.subject", ctx->lang), buffer, NULL) != 0)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s: %s\"}", cHTTPX_i18n_t("error.sending-email", ctx->lang),
                              payload.email);
        goto cleanup;
    }
    /* ---------- */
    /* Send email */

    if (redis_verifycode_create(payload.email, code) != 0)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.code-save-failed", ctx->lang));
        goto cleanup;
    }

    *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", cHTTPX_i18n_t("code-sent", ctx->lang));

cleanup:
    /* Free payloads */
    free(payload.name);
    free(payload.username);
    free(payload.email);
    
    if (payload.password)
        free(payload.password);

    if (user)
    {
        free(user->email);
        free(user->password);
        free(user);
        user = NULL;
    }

    if (password_hash)
        free(password_hash);

    if (req->context)
    {
        auth_token_t* ctx = (auth_token_t*)req->context;

        if (ctx->lang)
            free(ctx->lang);
        free(ctx);

        req->context = NULL;
    }

    return;

errorjson:
    *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);

    goto cleanup;
}

void auth_verify_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

    auth_verify_t payload = {0};

    chttpx_validation_t fields[] = {
        chttpx_validation_string("email", &payload.email, true, 5, 254, VALIDATOR_EMAIL),
        chttpx_validation_integer("code", &payload.code, true),
    };

    /* DB. get user core */
    user_core_t* user = NULL;

    if (!cHTTPX_Parse(req, fields, (sizeof(fields) / sizeof(fields[0]))))
        goto errorjson;

    if (!cHTTPX_Validate(req, fields, (sizeof(fields) / sizeof(fields[0])), ctx->lang))
        goto errorjson;

    /* Verify code */
    /* ----------- */
    int code;

    if (!redis_verifycode_get(payload.email, &code))
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusConflict, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.no-active-mail-confirmation", ctx->lang));
        goto cleanup;
    }
    /* ----------- */
    /* Verify code */

    if (payload.code != code)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.invalid-confirmation-code", ctx->lang));
        goto cleanup;
    }

    user = db_user_core_get_by_email(http_server->conn, payload.email);
    if (!user)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusNotFound, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.user-not-found", ctx->lang));
        goto cleanup;
    }

    if (user->email_confirm)
        goto confirmed;

    db_result_t user_confirm_db_result = db_user_core_upd_email_confirm(http_server->conn, true, payload.email);

    switch (user_confirm_db_result)
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

confirmed:
    session_t session = {.user_uid = user->user_uid, .expires_at = time(NULL) + REDIS_SESSION_TTL};

    char* session_id = redis_session_create(&session);
    if (!session_id)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.session-creation-error", ctx->lang));
        goto cleanup;
    }

    *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", session_id);

cleanup:
    /* Free payloads */
    free(payload.email);

    if (user)
    {
        free(user->email);
        free(user->password);
        free(user);
        user = NULL;
    }

    if (req->context)
    {
        auth_token_t* ctx = (auth_token_t*)req->context;

        if (ctx->lang)
            free(ctx->lang);
        free(ctx);

        req->context = NULL;
    }

    if (session_id)
        free(session_id);

    return;

errorjson:
    *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);

    goto cleanup;
}

void auth_delete_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

    db_result_t user_del_db_result = db_user_del_by_uid(http_server->conn, ctx->user->user_uid);

    switch (user_del_db_result)
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

    // DELETE AVATAR S3
    // DELETE FROM SESSION REDIS

    *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", cHTTPX_i18n_t("user-deleted", ctx->lang));

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