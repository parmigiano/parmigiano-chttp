#ifndef REDIS_SESSION_H
#define REDIS_SESSION_H

#include <stdint.h>

typedef struct {
    uint64_t user_uid;
    uint64_t expires_at;
} session_t;

char *session_create(const session_t *sess);

session_t *session_get(const char *session_id);

int session_refresh(const char *session_id);

int session_delete(const char *session_id);

#endif