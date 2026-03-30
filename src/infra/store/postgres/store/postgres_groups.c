#include "postgres/postgres.h"
#include "postgres/postgres_groups.h"

#include "logger.h"

/* Create chat group and return chat id */
db_result_t db_group_chat_create(PGconn* conn, chat_group_t* chat_group, uint64_t* out_chat_group_id)
{
    const char* query = "INSERT INTO chat_groups (user_uid, name) VALUES ($1, $2) RETURNING id";

    /* convert -> char */
    static char user_uid_str[32];
    snprintf(user_uid_str, sizeof(user_uid_str), "%ld", chat_group->user_uid);

    const char* params[2] = {user_uid_str, chat_group->name};

    time_t start = time(NULL);
    int timeout_sec = 5;

    while (1)
    {
        PGresult* res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
        if (!res)
            return DB_ERROR;

        ExecStatusType status = PQresultStatus(res);

        if (status == PGRES_TUPLES_OK)
        {
            if (PQntuples(res) != 1)
            {
                PQclear(res);
                return DB_ERROR;
            }

            const char* id_str = PQgetvalue(res, 0, 0);
            *out_chat_group_id = strtoull(id_str, NULL, 10);

            PQclear(res);
            return DB_OK;
        }

        if (status == PGRES_FATAL_ERROR)
        {
            logger_error("db_group_chat_create: failed to exec sql: %s", PQresultErrorMessage(res));
            PQclear(res);
            return DB_ERROR;
        }

        PQclear(res);

        if (difftime(time(NULL), start) > timeout_sec)
            return DB_TIMEOUT;
    }
}

db_result_t db_group_chat_add_chats(PGconn* conn, uint64_t user_uid, uint64_t group_id, int* chat_ids, size_t chat_size)
{
    const char* query = "INSERT INTO chat_group_user_chats (user_uid, group_id, chat_id) "
                        "SELECT $1, $2, UNNEST($3::bigint[]) "
                        "ON CONFLICT DO NOTHING";

    /* convert -> char */
    static char user_uid_str[32];
    snprintf(user_uid_str, sizeof(user_uid_str), "%ld", user_uid);

    static char group_id_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%ld", group_id);

    char chats_array[2048];
    size_t offset = 0;

    chats_array[offset++] = '{';

    for (size_t i = 0; i < chat_size; i++)
    {
        int written = snprintf(chats_array + offset, sizeof(chats_array) - offset, "%s%lu", (i == 0 ? "" : ","), chat_ids[i]);

        if (written < 0 || (size_t)written >= sizeof(chats_array) - offset)
            return DB_ERROR;

        offset += written;
    }

    chats_array[offset++] = '}';
    chats_array[offset] = '\0';

    const char* params[3] = {user_uid_str, group_id_str, chats_array};

    db_result_t result = execute_sql(conn, query, params, 3);
    if (result != DB_OK)
    {
        logger_error("db_group_chat_add_chats user_uid={%lu}: failed to exec sql: %s", user_uid, PQerrorMessage(conn));
    }

    return result;
}