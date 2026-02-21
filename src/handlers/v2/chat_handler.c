#include "handlers.h"
#include "postgres/postgres_chats.h"

#include "s3.h"
#include "httpx.h"

#include <string.h>
#include <libchttpx/libchttpx.h>

void chat_get_my_history_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

    const char* offset_query = cHTTPX_Query(req, "offset");

    size_t offset = 0;
    if (offset_query && *offset_query != '\0')
        offset = strtoull(offset_query, NULL, 10);

    chat_preview_LIST_t* chats_preview = NULL;
    chats_preview = db_chat_get_my_history(http_server->conn, ctx->user->user_uid, offset);
    if (!chats_preview || chats_preview->count == 0)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": []}");
        goto cleanup;
    }

    size_t capacity = 8192;
    char* json = malloc(capacity);
    size_t len_json = 0;

    len_json += snprintf(json + len_json, capacity - len_json, "{\"message\": [");

    for (size_t i = 0; i < chats_preview->count; i++)
    {
        chat_preview_t* chat = &chats_preview->items[i];

        char item[2048];

        snprintf(item, sizeof(item),
                 "{"
                 "\"id\":%lu,"
                 "\"name\":\"%s\","
                 "\"username\":\"%s\","
                 "\"avatar\":\"%s\","
                 "\"user_uid\":%lu,"
                 "\"last_message\":\"%s\","
                 "\"last_message_date\":%ld,"
                 "\"unread_message_count\":%u"
                 "}%s",
                 chat->id, chat->name ? chat->name : "", chat->username ? chat->username : "", chat->avatar ? chat->avatar : "", chat->user_uid,
                 chat->last_message ? chat->last_message : "", chat->last_message_date, chat->unread_message_count,
                 (i + 1 < chats_preview->count) ? "," : "");

        size_t need_len = strlen(item);

        if (len_json + need_len + 1 > capacity)
        {
            capacity *= 2;
            json = realloc(json, capacity);
        }

        memcpy(json + len_json, item, need_len);
        len_json += need_len;
    }

    snprintf(json + len_json, capacity - len_json, "]}");

    *res = cHTTPX_ResJson(cHTTPX_StatusOK, "%s", json);

    free(json);

cleanup:
    if (chats_preview)
    {
        for (size_t i = 0; i < chats_preview->count; i++)
        {
            free(chats_preview->items[i].name);
            free(chats_preview->items[i].username);
            free(chats_preview->items[i].avatar);
            free(chats_preview->items[i].last_message);
        }

        free(chats_preview);
        chats_preview = NULL;
    }

    return;
}

void chat_get_by_username_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

    const char* username_param = cHTTPX_Param(req, "username");

    chat_preview_LIST_t* chats_preview = NULL;
    chats_preview = db_chat_get_by_username(http_server->conn, ctx->user->user_uid, username_param);
    if (!chats_preview || chats_preview->count == 0)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": []}");
        goto cleanup;
    }

    size_t capacity = 8192;
    char* json = malloc(capacity);
    size_t len_json = 0;

    len_json += snprintf(json + len_json, capacity - len_json, "{\"message\": [");

    for (size_t i = 0; i < chats_preview->count; i++)
    {
        chat_preview_t* chat = &chats_preview->items[i];

        char item[2048];

        snprintf(item, sizeof(item),
                 "{"
                 "\"id\":%lu,"
                 "\"name\":\"%s\","
                 "\"username\":\"%s\","
                 "\"avatar\":\"%s\","
                 "\"user_uid\":%lu,"
                 "\"last_message\":\"%s\","
                 "\"last_message_date\":%ld,"
                 "\"unread_message_count\":%u"
                 "}%s",
                 chat->id, chat->name ? chat->name : "", chat->username ? chat->username : "", chat->avatar ? chat->avatar : "", chat->user_uid,
                 chat->last_message ? chat->last_message : "", chat->last_message_date, chat->unread_message_count,
                 (i + 1 < chats_preview->count) ? "," : "");

        size_t need_len = strlen(item);

        if (len_json + need_len + 1 > capacity)
        {
            capacity *= 2;
            json = realloc(json, capacity);
        }

        memcpy(json + len_json, item, need_len);
        len_json += need_len;
    }

    snprintf(json + len_json, capacity - len_json, "]}");

    *res = cHTTPX_ResJson(cHTTPX_StatusOK, "%s", json);

    free(json);

cleanup:
    if (chats_preview)
    {
        for (size_t i = 0; i < chats_preview->count; i++)
        {
            free(chats_preview->items[i].name);
            free(chats_preview->items[i].username);
            free(chats_preview->items[i].avatar);
            free(chats_preview->items[i].last_message);
        }

        free(chats_preview);
        chats_preview = NULL;
    }

    return;
}

void chat_get_settings_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

    /* DB. get chat setting */
    chat_setting_t* chat_setting = NULL;

    const char* chat_id_param = cHTTPX_Param(req, "chat_id");

    uint64_t chat_id = 0;
    if (chat_id_param && *chat_id_param != '\0')
        chat_id = strtoull(chat_id_param, NULL, 10);

    bool is_member = db_chat_get_member_exists_by_chat_id(http_server->conn, chat_id, ctx->user->user_uid);
    if (!is_member)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusForbidden, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.not-member-chat", ctx->lang));
        goto cleanup;
    }

    chat_setting = db_chat_get_setting_by_chat_id(http_server->conn, chat_id);
    if (!chat_setting)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusNotFound, "{\"message\": \"NULL\"}");
        goto cleanup;
    }

    *res = cHTTPX_ResJson(cHTTPX_StatusOK,
                          "{"
                          "\"message\": {"
                          "\"id\": %lu,"
                          "\"created_at\": %ld,"
                          "\"updated_at\": %ld,"
                          "\"chat_id\": %lu,"
                          "\"custom_background\": \"%s\","
                          "\"blocked\": %s,"
                          "\"who_blocked_uid\": %lu"
                          "}"
                          "}",
                          chat_setting->id, chat_setting->created_at, chat_setting->updated_at, chat_setting->chat_id,
                          chat_setting->custom_background ? chat_setting->custom_background : "", chat_setting->blocked ? "true" : "false",
                          chat_setting->who_blocked_uid);

cleanup:
    if (chat_setting)
    {
        free(chat_setting->custom_background);
        free(chat_setting);

        chat_setting = NULL;
    }

    return;
}

void chat_upload_custom_bg_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

    /* db. get chat_setting */
    chat_setting_t* chat_setting = NULL;
    /* Initial URL avatar */
    char* url = NULL;

    const char* chat_id_param = cHTTPX_Param(req, "chat_id");

    uint64_t chat_id = 0;
    if (chat_id_param && *chat_id_param != '\0')
        chat_id = strtoull(chat_id_param, NULL, 10);

    bool is_member = db_chat_get_member_exists_by_chat_id(http_server->conn, chat_id, ctx->user->user_uid);
    if (!is_member)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusForbidden, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.not-member-chat", ctx->lang));
        goto cleanup;
    }

    if (req->filename[0] == '\0')
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.no-data-to-process", ctx->lang));
        goto cleanup;
    }

    /* Jpeg/Jpg | Png */
    if (strcmp(req->content_type, cHTTPX_CTYPE_JPEG) != 0 && strcmp(req->content_type, cHTTPX_CTYPE_PNG) != 0)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.forbidden-file-extension", ctx->lang));
        goto cleanup;
    }

    chat_setting = db_chat_get_setting_by_chat_id(http_server->conn, chat_id);
    if (!chat_setting)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusNotFound, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.chat-setting-not-found", ctx->lang));
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

    char s3_bg_chat_key[256];
    snprintf(s3_bg_chat_key, sizeof(s3_bg_chat_key), "bg_chat_id_%ld", chat_id);

    url = s3_upload_file(f, req->filename, req->content_type, s3_bg_chat_key, &s3_config);
    fclose(f);

    if (!url || *url == '\0')
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.save-file", ctx->lang));
        goto cleanup;
    }

    /* Delete old custom bg in S3 */
    if (s3_delete_file(chat_setting->custom_background ? chat_setting->custom_background : "", &s3_config) != 0)
    {
        *res = cHTTPX_ResJson(cHTTPX_StatusConflict, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.s3-cloud", ctx->lang));
        goto cleanup;
    }

    /* update cbackground in database */
    db_result_t chat_db_result = db_chat_upd_cbackground_by_chat_id(http_server->conn, url, chat_id);

    switch (chat_db_result)
    {
    case DB_TIMEOUT:
        *res = cHTTPX_ResJson(cHTTPX_StatusConnectionTimedOut, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.database-connection-timeout", ctx->lang));
        goto cleanup;

    case DB_DUPLICATE:
        *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.repeating-data-request", ctx->lang));
        goto cleanup;

    case DB_ERROR:
        *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.perform-database-operation", ctx->lang));
        goto cleanup;
    }

    *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", url);

cleanup:
    if (chat_setting)
    {
        free(chat_setting->custom_background);
        free(chat_setting);

        chat_setting = NULL;
    }

    if (url)
        free(url);

    if (req->filename[0] != '\0')
        remove(req->filename);

    return;
}

typedef struct
{
    char* title;
    char* description;
    char* chat_type;
} chat_group_create_t;

// void chat_create_group_or_channel_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
// {
//     auth_token_t* ctx = (auth_token_t*)req->context;

//     chat_group_create_t payload = {0};

//     chttpx_validation_t fields[] = {
//         chttpx_validation_string("title", &payload.title, true, 1, 30, VALIDATOR_NONE),
//         chttpx_validation_string("description", &payload.description, false, 1, 255, VALIDATOR_NONE),
//         chttpx_validation_string("chat_type", &payload.chat_type, true, 4, 255, VALIDATOR_NONE),
//     };

//     if (!cHTTPX_Parse(req, fields, (sizeof(fields) / sizeof(fields[0]))))
//         goto errorjson;

//     if (!cHTTPX_Validate(req, fields, (sizeof(fields) / sizeof(fields[0])), ctx->lang))
//         goto errorjson;

//     /* calloc chat */
//     chat_t* chat = calloc(1, sizeof(chat_t));
//     if (!chat)
//     {
//         fprintf(stderr, "calloc failed\n");
//         *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.something-went-wrong", ctx->lang));

//         goto cleanup;
//     }
//     chat->title = strdup(payload.title);
//     chat->chat_type = strdup(payload.chat_type);
//     if (payload.description)
//         chat->description = strdup(payload.description);

//     /* calloc chat setting */
//     chat_setting_t* chat_setting = calloc(1, sizeof(chat_setting_t));
//     if (!chat_setting)
//     {
//         fprintf(stderr, "calloc failed\n");
//         *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.something-went-wrong", ctx->lang));

//         goto cleanup;
//     }

//     /* initial chat members */
//     chat_member_t members[] = {
//         {.user_uid = ctx->user->user_uid, .role = "owner"},
//     };

//     uint64_t chat_id = 0;
//     db_result_t chat_db_result = db_chat_create_all(http_server->conn, chat, members, (sizeof(members) / sizeof(members[0])), chat_setting, &chat_id);

//     /* free memory */
//     free(chat->title);
//     free(chat->chat_type);
//     free(chat->description);
//     free(chat);
//     chat = NULL;

//     free(chat_setting);
//     chat_setting = NULL;

//     switch (chat_db_result)
//     {
//     case DB_TIMEOUT:
//         *res = cHTTPX_ResJson(cHTTPX_StatusConnectionTimedOut, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.database-connection-timeout", ctx->lang));
//         goto cleanup;

//     case DB_DUPLICATE:
//         *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.repeating-data-request", ctx->lang));
//         goto cleanup;

//     case DB_ERROR:
//         *res = cHTTPX_ResJson(cHTTPX_StatusInternalServerError, "{\"error\": \"%s\"}", cHTTPX_i18n_t("error.perform-database-operation", ctx->lang));
//         goto cleanup;
//     }

//     *res = cHTTPX_ResJson(cHTTPX_StatusCreated, "{\"message\": {\"chat_id\": %ld}}", chat_id);

// cleanup:
//     free(payload.title);
//     free(payload.chat_type);

//     if (payload.description)
//         free(payload.description);

//     return;

// errorjson:
//     *res = cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);

//     goto cleanup;
// }