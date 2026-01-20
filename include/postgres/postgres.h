#ifndef POSTGRES_H
#define POSTGRES_H

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <libpq-fe.h>

typedef enum {
    DB_OK = 0,
    DB_DUPLICATE = 1,
    DB_ERROR = -1,
    DB_TIMEOUT = -2
} db_result_t;

/* Connection to database */
PGconn* db_conn(void);

/* Disconnect from database */
void db_close(PGconn* conn);

/* Started migrations in tables */
void run_migrations(PGconn* conn);

/* One row SELECT result */
typedef struct {
    char **columns;
    int n_columns;
} db_row_t;

// many SELECT result
typedef struct {
    db_row_t *rows;
    int n_rows;
} db_result_set_t;

/* Exec INSERT/UPDATE/DELETE */
db_result_t execute_sql(PGconn *conn, const char *query, const char **params, int n_params);

/* Exec SELECT and return many lines -> out_result */
db_result_t execute_select(PGconn *conn, const char *query, const char **params, int n_params, db_result_set_t **out_result);

/* free memory SELECT */
void free_result_set(db_result_set_t *rs);

#endif