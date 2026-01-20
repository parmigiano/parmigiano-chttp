#include "postgres/postgres.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>

PGconn* db_conn(void)
{
    char connstr[256];
    snprintf(connstr, sizeof(connstr), "host=%s dbname=%s user=%s password=%s", getenv("DB_HOST"),
             getenv("DB_NAME"), getenv("DB_USER"), getenv("DB_PASSWORD"));

    PGconn* conn = PQconnectdb(connstr);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "DB error: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }

    log_info("Successfully connected to the database!\n");

    return conn;
}

void db_close(PGconn* conn)
{
    PQfinish(conn);
}