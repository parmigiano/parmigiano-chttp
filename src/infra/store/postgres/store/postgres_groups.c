#include "postgres/postgres.h"
#include "postgres/postgres_groups.h"

#include "logger.h"

/* Create chat group and return chat id */
db_result_t db_chat_group_create(PGconn* conn, chat_group_t* chat_group, uint64_t* out_chat_group_id)
{
    const char* query = "INSERT INTO chat_groups (user_uid, name) VALUES ($1, $2) RETURNING id;";

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
            logger_error("db_chat_group_create: failed to exec sql: %s", PQresultErrorMessage(res));
            PQclear(res);
            return DB_ERROR;
        }

        PQclear(res);

        if (difftime(time(NULL), start) > timeout_sec)
            return DB_TIMEOUT;
    }
}