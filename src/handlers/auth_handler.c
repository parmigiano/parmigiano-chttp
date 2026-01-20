#include "handlers.h"

#include "httpx.h"
#include "utilities.h"
#include "redis/redis.h"
#include "redis/redis_session.h"
#include "postgres/postgres_users.h"

#include <time.h>
#include <stdbool.h>
#include <openssl/rand.h>

typedef struct
{
    char* name;
    char* username;
    char* email;
    char* password;
} user_create_t;

chttpx_response_t auth_login_handler(chttpx_request_t* req)
{
    return cHTTPX_ResJson(200, "{\"message\": \"%s\"}", "welcome");
}

chttpx_response_t auth_create_handler(chttpx_request_t* req)
{
    user_create_t payload;

    chttpx_validation_t fields[] = {
        chttpx_validation_str("name", true, 0, 24, &payload.name),
        chttpx_validation_str("username", true, 3, 24, &payload.username),
        chttpx_validation_str("email", true, 0, 254, &payload.email),
        chttpx_validation_str("password", true, 8, 254, &payload.password),
    };

    if (!cHTTPX_Parse(req, fields, (sizeof(fields) / sizeof(fields[0]))))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);
    }

    /* Trim spaces */
    trim_space(payload.name);
    trim_space(payload.username);
    trim_space(payload.email);
    trim_space(payload.password);

    /* Check symbols */
    if (!is_valid(payload.name))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest,
                              "{\"error\": \"недопустимые символы в имени\"}");
    }

    if (!is_valid(payload.username))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest,
                              "{\"error\": \"недопустимые символы в имени пользователя\"}");
    }

    /* Check simple passwords */
    if (is_simple_password(payload.password))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest,
                              "{\"error\": \"пароль слишком простой, введите новый\"}");
    }

    /* To lower */
    to_lower(payload.email);
    to_lower(payload.password);

    if (!cHTTPX_Validate(req, fields, (sizeof(fields) / sizeof(fields[0]))))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);
    }

    uint64_t uid;
    if (RAND_bytes((unsigned char*)&uid, sizeof(uid)) != 1)
    {
        fprintf(stderr, "Failed to generate UID\n");
        return cHTTPX_ResJson(
            cHTTPX_StatusInternalServerError,
            "{\"error\": \"ошибка генерации уникального значения пользователя\"}");
    }
    uid = (uid % 9000000000ULL) + 1000000000ULL;

    char* password_hash = hash_password(payload.password);
    if (!password_hash)
    {
        return cHTTPX_ResJson(cHTTPX_StatusInternalServerError,
                              "{\"error\": \"ошибка хеширования пароля\"}");
    }

    /* calloc user_core */
    user_core_t* user_core = calloc(1, sizeof(user_core_t));
    if (!user_core)
    {
        fprintf(stderr, "calloc failed\n");
        exit(1);
    }
    user_core->user_uid = uid;
    user_core->email = strdup(payload.email);
    user_core->password = strdup(password_hash);

    /* calloc user_profile */
    user_profile_t* user_profile = calloc(1, sizeof(user_profile_t));
    if (!user_profile)
    {
        fprintf(stderr, "calloc failed\n");
        exit(1);
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
        exit(1);
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
        exit(1);
    }
    user_active->user_uid = uid;

    db_result_t user_db_result = db_user_create(http_server->conn, user_core, user_profile,
                                                user_profile_access, user_active);

    /* free memory */
    free(user_core->email);
    free(user_core->password);
    free(user_core);

    free(user_profile->name);
    free(user_profile->username);
    free(user_profile);

    free(user_profile_access);
    free(user_active);

    switch (user_db_result)
    {
    case DB_TIMEOUT:
        return cHTTPX_ResJson(cHTTPX_StatusConnectionTimedOut,
                              "{\"error\": \"время ожидания подключения к базе данных истекло\"}");

    case DB_DUPLICATE:
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest,
                              "{\"error\": \"повторяющие данные в запросе\"}");

    case DB_ERROR:
        return cHTTPX_ResJson(cHTTPX_StatusConflict,
                              "{\"error\": \"не удалось выполнить операцию с базой данных\"}");
    }

    session_t session = {.user_uid = uid, .expires_at = time(NULL) + REDIS_SESSION_TTL};

    char *session_id = session_create(&session);
    if (!session_id)
    {
        return cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"ошибка создании сессии\"}");
    }

    return cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", session_id);
}