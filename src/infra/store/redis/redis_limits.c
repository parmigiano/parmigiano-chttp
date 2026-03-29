#include "redis/redis.h"

#include "redis/redis_limits.h"

#include <time.h>
#include <stdlib.h>
#include <hiredis/hiredis.h>

int redis_check_limit_email_and_increment(const char* email, int max_per_day)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char date[16];
    snprintf(date, sizeof(date), "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    char key[128];
    snprintf(key, sizeof(key), "user:%s:requests:%s", email, date);

    redisReply* reply = redisCommand(redis, "GET %s", key);
    int count = 0;
    if (reply->type == REDIS_REPLY_STRING)
    {
        count = atoi(reply->str);
    }
    freeReplyObject(reply);

    if (count >= max_per_day)
    {
        return 0;
    }

    reply = redisCommand(redis, "INCR %s", key);
    freeReplyObject(reply);

    reply = redisCommand(redis, "EXPIRE %s %d", key, 24 * 60 * 60 - (tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec));
    freeReplyObject(reply);

    return 1;
}