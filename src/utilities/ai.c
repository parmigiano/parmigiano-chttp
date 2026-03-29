#include "utilities.h"

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>

#include <cjson/cJSON.h>

#include <libchttpx/libchttpx.h>

typedef struct Request
{
    char* text;
    void (*callback)(const char* result, void* arg);
    void* arg;
    struct Request* next;
} Request;

typedef struct
{
    Request* head;
    Request* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} RequestQueue;

static RequestQueue queue;

static void init_ai_queue()
{
    queue.head = queue.tail = NULL;

    pthread_mutex_init(&queue.mutex, NULL);
    pthread_cond_init(&queue.cond, NULL);
}

void enqueue_ai_request(const char* text, void (*callback)(const char* result, void* arg), void* arg)
{
    Request* req = malloc(sizeof(Request));
    req->text = strdup(text);
    req->callback = callback;
    req->arg = arg;
    req->next = NULL;

    pthread_mutex_lock(&queue.mutex);

    if (queue.tail)
    {
        queue.tail->next = req;
        queue.tail = req;
    }
    else
    {
        queue.head = queue.tail = req;
    }

    pthread_cond_signal(&queue.cond);
    pthread_mutex_unlock(&queue.mutex);
}

static Request* dequeue_ai_request()
{
    pthread_mutex_lock(&queue.mutex);
    while (queue.head == NULL)
    {
        pthread_cond_wait(&queue.cond, &queue.mutex);
    }

    Request* req = queue.head;
    queue.head = req->next;
    if (!queue.head)
        queue.tail = NULL;

    pthread_mutex_unlock(&queue.mutex);

    return req;
}

static void* worker_thread(void* arg)
{
    (void)arg;

    while (1)
    {
        Request* req = dequeue_ai_request();
        char* result = call_ai_text(req->text);
        req->callback(result, req->arg);

        free(result);
        free(req->text);
        free(req);
    }

    return NULL;
}

void start_ai_worker()
{
    static int started = 0;
    if (started)
        return;
    started = 1;

    init_ai_queue();
    pthread_t tid;
    pthread_create(&tid, NULL, worker_thread, NULL);
    pthread_detach(tid);
}

void callback_ai_send_response(const char* result, void* arg)
{
    chttpx_response_t* res = (chttpx_response_t*)arg;
    *res = cHTTPX_ResJson(cHTTPX_StatusOK, "{\"message\": \"%s\"}", result);
}

char* get_ollama_response(const char* json_str)
{
    cJSON* root = cJSON_Parse(json_str);
    if (!root) return NULL;

    cJSON* response = cJSON_GetObjectItem(root, "response");
    if (!cJSON_IsString(response))
    {
        cJSON_Delete(root);
        return NULL;
    }

    char* result = strdup(response->valuestring);

    cJSON_Delete(root);
    return result;
}

char* call_ai_text(const char* prompt)
{
    CURL* curl;
    CURLcode res;

    struct Memory chunk;
    chunk.response = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (!curl)
        return NULL;

    const char* ollama_phi_host = getenv("OLLAMA_PHI_HOST");
    if (!ollama_phi_host)
        ollama_phi_host = "http://localhost:11434";

    char url[256];
    snprintf(url, sizeof(url), "%s/api/generate", ollama_phi_host);

    curl_easy_setopt(curl, CURLOPT_URL, url);

    char json[1024];
    snprintf(json, sizeof(json),
             "{"
             "\"model\": \"phi3:latest\","
             "\"prompt\": \"%s\","
             "\"stream\": false"
             "}",
             escape_json_string(prompt));

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        logger_error("call_ai_text: curl error: %s", curl_easy_strerror(res));
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
        return NULL;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return chunk.response;
}