#include "redis/redis.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

redisContext* redis = NULL;

int redis_conn(void)
{
    const char* host_env = getenv("REDIS_HOST");
    const char* port_env = getenv("REDIS_PORT");
    const char* password_env = getenv("REDIS_PASSWORD");

    if (!host_env || !port_env)
    {
        return 0;
    }

    int port = atoi(port_env);
    if (port <= 0)
        return 0;

    struct timeval timeout = {5, 0};
    redis = redisConnectWithTimeout(host_env, port, timeout);
    if (!redis || redis->err)
    {
        if (redis)
            redisFree(redis);
        return 0;
    }

    if (password_env && strlen(password_env) > 0)
    {
        redisReply* r = redisCommand(redis, "AUTH %s", password_env);
        if (!r || r->type == REDIS_REPLY_ERROR)
        {
            if (r)
                freeReplyObject(r);
            redisFree(redis);
            redis = NULL;
            return 0;
        }

        freeReplyObject(r);
    }

    log_info("Successfully connected to the redis!\n");
    return 1;
}