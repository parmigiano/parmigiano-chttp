#ifndef POSTGRES_CHATS_H
#define POSTGRES_CHATS_H

#include "postgres.h"

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

/* table. CHATS */
/* ------------ */
typedef struct {
    uint64_t id;
    time_t created_at;
    time_t updated_at;
    char* chat_type; /* private ; group ; channel */
    char* title;
} chat_t;

typedef struct {
    uint64_t id;
    time_t created_at;
    uint64_t chat_id;
    uint64_t user_uid;
    char* role; /* owner ; admin ; member */
} chat_member_t;

typedef struct {
    uint64_t id;
    time_t created_at;
    time_t updated_at;
    uint64_t chat_id;
    char* custom_background;
    bool blocked;
    uint64_t who_blocked_uid;
} chat_setting_t;

typedef struct {
    uint64_t id;
    char* name;
    char* username;
    char* avatar;
    uint64_t user_uid;
    char* last_message;
    time_t last_message_date;
    uint16_t unread_message_count;
} chat_preview_t;

typedef struct {
    chat_preview_t* items;
    size_t count;
} chat_preview_LIST_t;

typedef struct {
    uint64_t* user_uids;
    size_t count;
} chat_member_LIST_t;
/* ------------ */
/* table. CHATS */

/* Get my history chats [limit 15 chats] */
chat_preview_LIST_t* db_chat_get_my_history(PGconn* conn, uint64_t user_uid, size_t offset);
/* Get chat members by chat_id */
chat_member_LIST_t* db_chat_get_members_by_chat_id(PGconn* conn, uint64_t chat_id, uint64_t user_uid);
/* Check exists or nah user in chat members by chat_id */
bool db_chat_get_member_exists_by_chat_id(PGconn* conn, uint64_t chat_id, uint64_t user_uid);
/* Get chat settings by chat_id */
chat_setting_t* db_chat_get_setting_by_chat_id(PGconn* conn, uint64_t chat_id);

#endif