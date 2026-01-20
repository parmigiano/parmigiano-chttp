#ifndef REDIS_H
#define REDIS_H

#include <stdint.h>
#include <hiredis/hiredis.h>

#define REDIS_SESSION_TTL 86400

extern redisContext *redis;

/* Initial redis connect */
int redis_conn(void);

#endif