#ifndef POSTGRESDB_H
#define POSTGRESDB_H

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <libpq-fe.h>

/* table. user_cores */
typedef struct {
    uint64_t id;
    time_t created_at;
    time_t updated_at;
    uint64_t user_uid;
    char *email;
    uint8_t email_confirmed;
    char *password;
} user_core_t;

typedef struct {
    uint64_t id;
    time_t created_at;
    uint64_t user_uid;
    bool online;
    time_t last_online_date;
    char *avatar;            
    char *name;
    char *username;
    bool username_visible; 
    char *email;             
    bool email_visible;    
    bool email_confirmed;  
    char *phone;             
    bool phone_visible;    
    char *overview;           
    char *access_token;     
} user_info_t;

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

/* SQLs */
/* Create user_core */
int db_user_core_create(PGconn *conn, user_core_t *user);
/* Get user_core by user_uid */
user_core_t* db_user_core_by_uid(PGconn *conn, uint64_t user_uid);

#endif