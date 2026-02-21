#include "postgres/postgres.h"
#include "postgres/postgres_messages.h"

db_result_t db_message_create(PGconn* conn, message_t* msg, uint64_t* out_message_id)
{
    const char* query = "INSERT INTO messages (chat_id, sender_uid, content, content_type) VALUES ($1, $2, $3, $4) RETURNING id;";

    static char chat_id_str[32];
    static char sender_uid_str[32];

    snprintf(chat_id_str, sizeof(chat_id_str), "%ld", msg->chat_id);
    snprintf(sender_uid_str, sizeof(sender_uid_str), "%ld", msg->sender_uid);

    const char* params[4] = {chat_id_str, sender_uid_str, msg->content, msg->content_type};

    time_t start = time(NULL);
    int timeout_sec = 5;

    while (1)
    {
        PGresult* res = PQexecParams(conn, query, 4, NULL, params, NULL, NULL, 0);
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
            *out_message_id = strtoull(id_str, NULL, 10);

            PQclear(res);
            return DB_OK;
        }

        if (status == PGRES_FATAL_ERROR)
        {
            fprintf(stderr, "DB error: %s\n", PQresultErrorMessage(res));
            PQclear(res);
            return DB_ERROR;
        }

        PQclear(res);

        if (difftime(time(NULL), start) > timeout_sec)
            return DB_TIMEOUT;
    }
}

db_result_t db_message_create_status(PGconn* conn, uint64_t message_id, message_status_t* msg_status)
{
    const char* query = "INSERT INTO message_statuses (message_id, receiver_uid) VALUES ($1, $2)";

    static char message_id_str[32];
    static char receiver_uid_str[32];

    snprintf(message_id_str, sizeof(message_id_str), "%ld", message_id);
    snprintf(receiver_uid_str, sizeof(receiver_uid_str), "%ld", msg_status->receiver_uid);

    const char* params[2] = {message_id_str, receiver_uid_str};

    return execute_sql(conn, query, params, 2);
}

db_result_t db_message_create_all(PGconn* conn, message_t* msg, message_status_t* msg_status)
{
    execute_sql(conn, "BEGIN", NULL, 0);

    db_result_t rc;

    uint64_t message_id;
    rc = db_message_create(conn, msg, &message_id);
    if (rc != DB_OK)
        goto rollback;

    rc = db_message_create_status(conn, message_id, msg_status);
    if (rc != DB_OK)
        goto rollback;

    execute_sql(conn, "COMMIT", NULL, 0);

    return DB_OK;

rollback:
    execute_sql(conn, "ROLLBACK", NULL, 0);

    return DB_ERROR;
}