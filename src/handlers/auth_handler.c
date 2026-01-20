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
    char* name;
    char* username;
    char* email;
    char* password;
} auth_create_t;

typedef struct
{
    char* email;
    char* password;
} auth_login_t;

typedef struct
{
    char* email;
    int code;
} auth_verify_t;

chttpx_response_t auth_login_handler(chttpx_request_t* req)
{
    auth_login_t payload;

    chttpx_validation_t fields[] = {
        chttpx_validation_str("email", true, 5, 254, &payload.email),
        chttpx_validation_str("password", false, 8, 254, &payload.password),
    };

    if (!cHTTPX_Parse(req, fields, (sizeof(fields) / sizeof(fields[0]))))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);
    }

    if (!cHTTPX_Validate(req, fields, (sizeof(fields) / sizeof(fields[0]))))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);
    }

    /* Trim spaces */
    trim_space(payload.email);
    /* To lower email */
    to_lower(payload.email);

    /* Validation email */
    if (!is_valid_email(payload.email))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest,
                              "{\"error\": \"неверный или несуществующий email\"}");
    }

    user_core_t* user = db_user_core_get_by_email(http_server->conn, payload.email);
    if (!user)
    {
        return cHTTPX_ResJson(cHTTPX_StatusNotFound, "{\"error\": \"пользователь не был найден\"}");
    }

    /* Verify code */
    /* ----------- */
    int codetmp;

    if (redis_verifycode_get(payload.email, &codetmp))
    {
        return cHTTPX_ResJson(cHTTPX_StatusOK,
                              "{\"message\": \"Код подтверждения уже отправлен на вашу почту.\"}");
    }
    /* ----------- */
    /* Verify code */

    uint64_t code;
    if (RAND_bytes((unsigned char*)&code, sizeof(code)) != 1)
    {
        fprintf(stderr, "Failed to generate CODE\n");
        return cHTTPX_ResJson(cHTTPX_StatusInternalServerError,
                              "{\"error\": \"ошибка генерации кода\"}");
    }
    code = (code % 900000) + 100000;

    if (redis_verifycode_create(payload.email, code) != 0)
    {
        return cHTTPX_ResJson(cHTTPX_StatusInternalServerError,
                              "{\"error\": \"ошибка сохранения кода\"}");
    }

    /* Send email */
    /* ---------- */
    const char* html =
        "<body>"
        "<p>Мы получили запрос на использование адреса электронной почты <b>%s</b></p>"
        "<p>Введите код для подтверждения входа. Если вы не запрашивали письмо, просто "
        "проигнорируйте его и не вводите код:</p>"
        "<h2>%d</h2>"
        "<p>Срок действия кода истечет через 24 часа...</p>"
        "<p>P.S. Данное письмо сгенерировано и отправлено автоматически. Пожалуйста, не отвечайте "
        "на него</p>"
        "</body>";

    char buffer[2048];
    snprintf(buffer, sizeof(buffer), html, code);

    send_email(payload.email, "Подтверждения адреса электронной почты ParmigianoChat", buffer);
    /* ---------- */
    /* Send email */

    return cHTTPX_ResJson(cHTTPX_StatusOK,
                          "{\"message\": \"Код подтверждения отправлен на вашу почту.\"}");
}

chttpx_response_t auth_create_handler(chttpx_request_t* req)
{
    auth_create_t payload;

    chttpx_validation_t fields[] = {
        chttpx_validation_str("name", true, 2, 24, &payload.name),
        chttpx_validation_str("username", true, 4, 24, &payload.username),
        chttpx_validation_str("email", true, 5, 254, &payload.email),
        chttpx_validation_str("password", false, 8, 254, &payload.password),
    };

    if (!cHTTPX_Parse(req, fields, (sizeof(fields) / sizeof(fields[0]))))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);
    }

    if (!cHTTPX_Validate(req, fields, (sizeof(fields) / sizeof(fields[0]))))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);
    }

    /* Verify code */
    /* ----------- */
    int codetmp;

    if (redis_verifycode_get(payload.email, &codetmp))
    {
        return cHTTPX_ResJson(cHTTPX_StatusCreated,
                              "{\"message\": \"Код подтверждения уже отправлен на вашу почту.\"}");
    }
    /* ----------- */
    /* Verify code */

    /* Trim spaces */
    trim_space(payload.username);
    trim_space(payload.email);

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

    /* To lower email */
    to_lower(payload.email);

    /* Validation email */
    if (!is_valid_email(payload.email))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest,
                              "{\"error\": \"неверный или несуществующий email\"}");
    }

    char* password_hash = NULL;

    /* Validation password and hash password */
    if (payload.password)
    {
        /* Trim space password */
        trim_space(payload.password);
        /* To lower string password */
        to_lower(payload.password);

        if (is_simple_password(payload.password))
        {
            return cHTTPX_ResJson(cHTTPX_StatusBadRequest,
                                  "{\"error\": \"пароль слишком простой, введите новый\"}");
        }

        password_hash = hash_password(payload.password);
        if (!password_hash)
        {
            return cHTTPX_ResJson(cHTTPX_StatusInternalServerError,
                                  "{\"error\": \"ошибка хеширования пароля\"}");
        }
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

    /* calloc user_core */
    user_core_t* user_core = calloc(1, sizeof(user_core_t));
    if (!user_core)
    {
        fprintf(stderr, "calloc failed\n");
        exit(1);
    }
    user_core->user_uid = uid;
    user_core->email = strdup(payload.email);
    user_core->password = password_hash ? strdup(password_hash) : NULL;

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

    uint64_t code;
    if (RAND_bytes((unsigned char*)&code, sizeof(code)) != 1)
    {
        fprintf(stderr, "Failed to generate CODE\n");
        return cHTTPX_ResJson(cHTTPX_StatusInternalServerError,
                              "{\"error\": \"ошибка генерации кода\"}");
    }
    code = (code % 900000) + 100000;

    if (redis_verifycode_create(payload.email, code) != 0)
    {
        return cHTTPX_ResJson(cHTTPX_StatusInternalServerError,
                              "{\"error\": \"ошибка сохранения кода\"}");
    }

    /* Send email */
    /* ---------- */
    const char* html =
        "<body>"
        "<p>Мы получили запрос на использование адреса электронной почты <b>%s</b></p>"
        "<p>Введите код для подтверждения входа. Если вы не запрашивали письмо, просто "
        "проигнорируйте его и не вводите код:</p>"
        "<h2>%d</h2>"
        "<p>Срок действия кода истечет через 24 часа...</p>"
        "<p>P.S. Данное письмо сгенерировано и отправлено автоматически. Пожалуйста, не отвечайте "
        "на него</p>"
        "</body>";

    char buffer[2048];
    snprintf(buffer, sizeof(buffer), html, code);

    send_email(payload.email, "Подтверждения адреса электронной почты ParmigianoChat", buffer);
    /* ---------- */
    /* Send email */

    return cHTTPX_ResJson(cHTTPX_StatusOK,
                          "{\"message\": \"Код подтверждения отправлен на вашу почту.\"}");
}

chttpx_response_t auth_verify_handler(chttpx_request_t* req)
{
    auth_verify_t payload;

    chttpx_validation_t fields[] = {
        chttpx_validation_str("email", true, 5, 254, &payload.email),
        chttpx_validation_int("code", true, &payload.code),
    };

    if (!cHTTPX_Parse(req, fields, (sizeof(fields) / sizeof(fields[0]))))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);
    }

    if (!cHTTPX_Validate(req, fields, (sizeof(fields) / sizeof(fields[0]))))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);
    }

    /* Verify code */
    /* ----------- */
    int code;

    if (!redis_verifycode_get(payload.email, &code))
    {
        return cHTTPX_ResJson(cHTTPX_StatusConflict,
                              "{\"error\": \"активных запросов для подтверждения не найдено\"}");
    }
    /* ----------- */
    /* Verify code */

    if (payload.code != code)
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest,
                              "{\"error\": \"неверный код подтверждения\"}");
    }

    db_result_t user_confirm_db_result =
        db_user_core_upd_email_confirm(http_server->conn, true, payload.email);

    switch (user_confirm_db_result)
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

    user_core_t* user = db_user_core_get_by_email(http_server->conn, payload.email);
    if (!user)
    {
        return cHTTPX_ResJson(cHTTPX_StatusNotFound, "{\"error\": \"пользователь не был найден\"}");
    }

    session_t session = {.user_uid = user->user_uid, .expires_at = time(NULL) + REDIS_SESSION_TTL};

    char* session_id = redis_session_create(&session);
    if (!session_id)
    {
        return cHTTPX_ResJson(cHTTPX_StatusInternalServerError,
                              "{\"error\": \"ошибка создании сессии\"}");
    }

    return cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", session_id);
}