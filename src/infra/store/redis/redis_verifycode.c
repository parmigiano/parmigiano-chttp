#include "redis/redis.h"
#include "redis/redis_verifycode.h"

static char* verifycode_key(const char* key)
{
    char* buffer = malloc(strlen(key) + 12);
    sprintf(buffer, "verifycode:%s", key);
    return buffer;
}

int redis_verifycode_create(const char* email, int code)
{
    if (!email)
        return 1;

    char* key = verifycode_key(email);
    if (!key)
        return 1;

    redisReply* r = redisCommand(redis, "SETEX %s %d", key, REDIS_VERIFYCODE_TTL, code);
    if (!r)
    {
        free(key);
        return 1;
    }

    freeReplyObject(r);
    free(key);

    return 0;
}

bool redis_verifycode_get(const char* email, int* out_code)
{
    if (!email || !out_code)
        return false;

    char* key = verifycode_key(email);
    if (!key)
        return false;

    redisReply* r = redisCommand(redis, "GET %s", key);
    if (!r)
    {
        free(key);
        return false;
    }

    bool exists = false;
    if (r->type == REDIS_REPLY_STRING)
    {
        *out_code = atoi(r->str);
        exists = true;
    }

    freeReplyObject(r);
    free(key);

    return exists;
}