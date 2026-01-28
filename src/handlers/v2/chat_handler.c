#include "handlers.h"
#include "postgres/postgres_chats.h"

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

    chat_preview_LIST_t* chats_preview = db_chat_get_my_history(http_server->conn, ctx->user->user_uid, offset);
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
    }

    return;
}

void chat_get_settings_handler_v2(chttpx_request_t* req, chttpx_response_t* res)
{
    auth_token_t* ctx = (auth_token_t*)req->context;

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

    chat_setting_t* chat_setting = db_chat_get_setting_by_chat_id(http_server->conn, chat_id);
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

    if (chat_setting)
    {
        free(chat_setting->custom_background);
        free(chat_setting);

        chat_setting = NULL;
    }

    return;
}