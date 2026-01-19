#include "postgresdb.h"

int db_user_core_create(PGconn* conn, user_core_t* user)
{
    const char* query = "INSERT INTO user_cores (user_uid, email, password) VALUES ($1, $2, $3)";

    /* convert -> char */
    char user_uid_str[64];
    snprintf(user_uid_str, sizeof(user_uid_str), "%lu", user->user_uid);

    const char* params[3] = {user_uid_str, user->email, user->password};

    db_result_t result = execute_sql(conn, query, params, 3);
    return result;
}

user_core_t* db_user_core_by_uid(PGconn* conn, uint64_t user_uid)
{
    db_result_set_t* rs = NULL;

    const char* query = "SELECT id, user_uid, email, email_confirmed, password FROM user_cores "
                        "WHERE user_uid = $1 LIMIT 1";

    /* convert -> char */
    char user_uid_str[64];
    snprintf(user_uid_str, sizeof(user_uid_str), "%lu", user_uid);

    const char* params[1] = {user_uid_str};

    db_result_t result = execute_select(conn, query, params, 1, &rs);

    if (result != DB_OK || !rs || rs->n_rows == 0)
    {
        free_result_set(rs);
        return NULL;
    }

    user_core_t* user = malloc(sizeof(user_core_t));
    memset(user, 0, sizeof(user_core_t));

    user->id = strtoull(rs->rows[0].columns[0], NULL, 10);
    user->user_uid = strtoull(rs->rows[0].columns[1], NULL, 10);
    user->email = strdup(rs->rows[0].columns[2]);
    user->email_confirmed = (uint8_t)atoi(rs->rows[0].columns[3]);
    user->password = strdup(rs->rows[0].columns[4]);

    free_result_set(rs);

    return user;
}