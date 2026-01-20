#include "postgres/postgres.h"
#include "postgres/postgres_users.h"

db_result_t db_user_core_create(PGconn* conn, user_core_t* user)
{
    const char* query = "INSERT INTO user_cores (user_uid, email, password) VALUES ($1, $2, $3)";

    /* convert -> char */
    char user_uid_str[64];
    snprintf(user_uid_str, sizeof(user_uid_str), "%lu", user->user_uid);

    const char* params[3] = {user_uid_str, user->email, user->password};

    db_result_t result = execute_sql(conn, query, params, 3);
    return result;
}

db_result_t db_user_profile_create(PGconn* conn, user_profile_t* user)
{
    const char* query =
        "INSERT INTO user_profiles (user_uid, avatar, name, username) VALUES ($1, $2, $3, $4)";

    /* convert -> char */
    char user_uid_str[64];
    snprintf(user_uid_str, sizeof(user_uid_str), "%lu", user->user_uid);

    const char* params[4] = {user_uid_str, user->avatar, user->name, user->username};

    db_result_t result = execute_sql(conn, query, params, 4);
    return result;
}

db_result_t db_user_profile_access_create(PGconn* conn, user_profile_access_t* user)
{
    const char* query = "INSERT INTO user_profile_accesses (user_uid, email_visible, "
                        "username_visible, phone_visible) VALUES ($1, $2, $3, $4)";

    /* convert -> char */
    char user_uid_str[64];
    snprintf(user_uid_str, sizeof(user_uid_str), "%lu", user->user_uid);

    const char* params[4] = {user_uid_str, user->email_visible ? "true" : "false",
                             user->username_visible ? "true" : "false",
                             user->phone_visible ? "true" : "false"};

    db_result_t result = execute_sql(conn, query, params, 4);
    return result;
}

db_result_t db_user_profile_active_create(PGconn* conn, user_active_t* user)
{
    const char* query = "INSERT INTO user_actives (user_uid, online) VALUES ($1, $2)";

    /* convert -> char */
    char user_uid_str[64];
    snprintf(user_uid_str, sizeof(user_uid_str), "%lu", user->user_uid);

    const char* params[2] = {user_uid_str, user->online ? "true" : "false"};

    db_result_t result = execute_sql(conn, query, params, 2);
    return result;
}

db_result_t db_user_create(PGconn* conn, user_core_t* user_core, user_profile_t* user_profile,
                           user_profile_access_t* user_profile_access, user_active_t* user_active)
{
    if (!conn)
        return DB_ERROR;

    PGresult* result = PQexec(conn, "BEGIN");
    if (!result || PQresultStatus(result) != PGRES_COMMAND_OK)
    {
        if (result)
            PQclear(result);
        return DB_ERROR;
    }
    PQclear(result);

    int rc;

    rc = db_user_core_create(conn, user_core);
    if (rc != DB_OK)
        goto rollback;

    rc = db_user_profile_create(conn, user_profile);
    if (rc != DB_OK)
        goto rollback;

    rc = db_user_profile_access_create(conn, user_profile_access);
    if (rc != DB_OK)
        goto rollback;

    rc = db_user_profile_active_create(conn, user_active);
    if (rc != DB_OK)
        goto rollback;

    result = PQexec(conn, "COMMIT");
    if (!result || PQresultStatus(result) != PGRES_COMMAND_OK)
    {
        if (result)
            PQclear(result);
        goto rollback;
    }

    PQclear(result);
    return DB_OK;

rollback:
    result = PQexec(conn, "ROLLBACK");
    if (result)
        PQclear(result);
    return DB_ERROR;
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