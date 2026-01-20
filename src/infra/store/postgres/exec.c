#include "postgres/postgres.h"

db_result_t execute_sql(PGconn* conn, const char* query, const char** params, int n_params)
{
    if (!conn || !query)
        return DB_ERROR;

    int timeout_sec = 5;

    time_t start = time(NULL);

    while (1)
    {
        PGresult* result = PQexecParams(conn, query, n_params, NULL, params, NULL, NULL, 0);
        if (!result)
            return DB_ERROR;

        ExecStatusType status = PQresultStatus(result);

        if (status == PGRES_COMMAND_OK)
        {
            PQclear(result);
            return DB_OK;
        }

        if (status == PGRES_FATAL_ERROR)
        {
            const char* err = PQresultErrorMessage(result);

            fprintf(stderr, "Database error: %s\n", err);
            PQclear(result);

            return DB_ERROR;
        }

        PQclear(result);

        if (difftime(time(NULL), start) > timeout_sec)
        {
            return DB_TIMEOUT;
        }
    }
}

db_result_t execute_select(PGconn* conn, const char* query, const char** params, int n_params,
                           db_result_set_t** out_result)
{
    if (!conn || !query || !out_result)
        return DB_ERROR;

    int timeout_sec = 5;

    time_t start = time(NULL);

    while (1)
    {
        PGresult* result = PQexecParams(conn, query, n_params, NULL, params, NULL, NULL, 0);
        if (!result)
            return DB_ERROR;

        ExecStatusType status = PQresultStatus(result);

        if (status == PGRES_TUPLES_OK)
        {
            size_t n_rows = PQntuples(result);
            size_t n_cols = PQnfields(result);

            db_result_set_t* result_set = malloc(sizeof(db_result_set_t));
            result_set->n_rows = n_rows;
            result_set->rows = malloc(sizeof(db_row_t) * n_rows);

            for (size_t i = 0; i < n_rows; i++)
            {
                result_set->rows[i].n_columns = n_cols;
                result_set->rows[i].columns = malloc(sizeof(char*) * n_cols);

                for (size_t j = 0; j < n_cols; j++)
                {
                    const char* val = PQgetvalue(result, i, j);
                    result_set->rows[i].columns[j] = strdup(val ? val : "");
                }
            }

            PQclear(result);
            *out_result = result_set;

            return DB_OK;
        }

        if (status == PGRES_FATAL_ERROR)
        {
            PQclear(result);
            return DB_ERROR;
        }

        PQclear(result);

        if (difftime(time(NULL), start) > timeout_sec)
        {
            return DB_TIMEOUT;
        }
    }
}

void free_result_set(db_result_set_t* rs)
{
    if (!rs)
        return;

    for (int i = 0; i < rs->n_rows; i++)
    {
        for (int j = 0; j < rs->rows[i].n_columns; j++)
        {
            free(rs->rows[i].columns[j]);
        }

        free(rs->rows[i].columns);
    }

    free(rs->rows);
    free(rs);
}