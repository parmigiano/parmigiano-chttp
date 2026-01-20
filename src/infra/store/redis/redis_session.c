#include "redis/redis.h"
#include "encryption.h"
#include "redis/redis_session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

static char* session_key(const char* key)
{
    char* buffer = malloc(strlen(key) + 9);
    sprintf(buffer, "session:%s", key);
    return buffer;
}

static char* session_to_json(const session_t* sess)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "user_uid", sess->user_uid);
    cJSON_AddNumberToObject(root, "expires_at", sess->expires_at);

    char* json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static int json_to_session(const char* json, session_t* sess)
{
    cJSON* root = cJSON_Parse(json);
    if (!root)
        return 0;

    sess->user_uid = cJSON_GetObjectItem(root, "user_uid")->valuedouble;
    sess->expires_at = cJSON_GetObjectItem(root, "expires_at")->valuedouble;

    cJSON_Delete(root);
    return 1;
}

char* session_create(const session_t* sess)
{
    char* json = session_to_json(sess);
    if (!json)
        return NULL;

    char* key = NULL;
    char* session_id = NULL;

    session_id = encrypt(json);
    key = session_key(session_id);

    redisReply* r = redisCommand(redis, "EXISTS %s", key);
    if (!r)
        goto error;

    if (r->integer == 1)
    {
        freeReplyObject(r);
        redisCommand(redis, "EXPIRE %s %d", REDIS_SESSION_TTL);
        free(key);
        return session_id;
    }

    freeReplyObject(r);
    redisCommand(redis, "SET %s %s EX %d", key, json, REDIS_SESSION_TTL);

    free(json);
    free(key);

    return session_id;

error:
    free(key);
    free(session_id);
    return NULL;
}

session_t* session_get(const char* session_id)
{
    char* key = session_key(session_id);

    redisReply* r = redisCommand(redis, "GET %s", key);
    free(key);

    if (!r || r->type == REDIS_REPLY_NIL)
    {
        if (r)
            freeReplyObject(r);
        return NULL;
    }

    session_t* sess = malloc(sizeof(session_t));
    if (!json_to_session(r->str, sess))
    {
        free(sess);
        sess = NULL;
    }

    freeReplyObject(r);
    return sess;
}

int session_refresh(const char* session_id)
{
    char* key = session_key(session_id);
    redisReply* r = redisCommand(redis, "EXPIRE %s %d", key, REDIS_SESSION_TTL);
    free(key);

    int ok = r && r->integer == 1;
    if (r)
        freeReplyObject(r);
    return ok;
}

int session_delete(const char* session_id)
{
    char* key = session_key(session_id);
    redisReply* r = redisCommand(redis, "DEL %s", key);
    free(key);

    int ok = r && r->integer == 1;
    if (r)
        freeReplyObject(r);
    return ok;
}