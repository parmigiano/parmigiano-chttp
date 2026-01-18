#include "postgresdb.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define FOLDER_SQL_MIGRATIONS "src/infra/store/postgres/migrations"

static char* read_file_migrate(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* data = malloc(size + 1);
    fread(data, 1, size, f);
    data[size] = 0;

    fclose(f);
    return data;
}

void run_migrations(PGconn* conn)
{
    PGresult* res;

    PQexec(conn, "CREATE TABLE IF NOT EXISTS schema_migrations ("
                 "version INT PRIMARY KEY, "
                 "applied_at TIMESTAMP DEFAULT now()"
                 ")");
    PQclear(res);

    DIR* dir = opendir(FOLDER_SQL_MIGRATIONS);
    if (!dir)
    {
        perror("opendir migrations");
        return;
    }

    struct dirent* ent;

    while ((ent = readdir(dir)))
    {
        if (!strstr(ent->d_name, ".sql"))
        {
            continue;
        }

        int version = atoi(ent->d_name);
        if (version == 0)
        {
            continue;
        }

        char check_sql[256];
        snprintf(check_sql, sizeof(check_sql), "SELECT 1 FROM schema_migrations WHERE version = %d",
                 version);

        res = PQexec(conn, check_sql);
        int applied = PQntuples(res) > 0;
        PQclear(res);

        if (applied)
        {
            continue;
        }

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", FOLDER_SQL_MIGRATIONS, ent->d_name);

        char* sql = read_file_migrate(path);
        if (!sql)
        {
            fprintf(stderr, "Cannot read %s\n", path);
            continue;
        }

        log_info("Applying migration %s\n", ent->d_name);

        PQexec(conn, "BEGIN");

        res = PQexec(conn, sql);
        free(sql);

        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr, "Migration failed: %s\n", PQerrorMessage(conn));
            PQclear(res);
            PQexec(conn, "ROLLBACK");
            closedir(dir);
            exit(1);
        }
        PQclear(res);

        char insert_sql[256];
        snprintf(insert_sql, sizeof(insert_sql),
                 "INSERT INTO schema_migrations(version) VALUES(%d)", version);

        res = PQexec(conn, insert_sql);
        PQclear(res);

        PQexec(conn, "COMMIT");
    }

    closedir(dir);
}