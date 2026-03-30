#ifndef POSTGRES_GROUPS_H
#define POSTGRES_GROUPS_H

#include "postgres.h"

/* table. GROUP_CHATS */
/* ------------------ */
typedef struct {
    uint64_t id;
    time_t created_at;
    time_t updated_at;
    uint64_t user_uid;
    char* name;
} chat_group_t;

typedef struct {
    uint64_t id;
    uint64_t user_uid;
    uint64_t group_id;
    uint64_t chat_id;
} chat_group_user_chats_t;
/* ------------------ */
/* table. GROUP_CHATS */

/* Create chat group and return chat id */
db_result_t db_group_chat_create(PGconn* conn, chat_group_t* chat_group, uint64_t* out_chat_group_id);
/* Add chats to group */
db_result_t db_group_chat_add_chats(PGconn* conn, uint64_t user_uid, uint64_t group_id, int* chat_ids, size_t chat_size);

#endif