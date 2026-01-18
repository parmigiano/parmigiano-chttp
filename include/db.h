#ifndef DB_H
#define DB_H

#include <libpq-fe.h>

PGconn* db_conn(void);

void db_close(PGconn* conn);

void run_migration(PGconn* conn);

#endif