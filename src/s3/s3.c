#include "s3.h"

#include <string.h>
#include <curl/curl.h>

struct s3_upload_ctx
{
    FILE* file;
};

size_t read_callback(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    struct s3_upload_ctx* ctx = (struct s3_upload_ctx*)userdata;
    return fread(ptr, size, nmemb, ctx->file);
}

char* s3_upload_file(FILE* f, const char* filename, s3_config_t* cfg)
{
    if (!f || !filename || !cfg)
        return NULL;

    CURL* curl = curl_easy_init();
    if (!curl)
        return NULL;

    fseek(f, 0, SEEK_END);
    curl_off_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char url[1024];
    snprintf(url, sizeof(url), "%s/%s/%s", cfg->endpoint, cfg->bucket, filename);

    struct s3_upload_ctx ctx = {.file = f};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, file_size);

    curl_easy_setopt(curl, CURLOPT_USERNAME, cfg->access_key);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, cfg->secret_key);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        return NULL;

    return strdup(url);
}

int s3_delete_file(const char* filename, s3_config_t* cfg)
{
    if (!filename || !cfg)
        return 1;

    CURL* curl = curl_easy_init();
    if (!curl)
        return 1;

    char url[1024];
    snprintf(url, sizeof(url), "%s/%s/%s", cfg->endpoint, cfg->bucket, filename);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_USERNAME, cfg->access_key);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, cfg->secret_key);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? 0 : 1;
}

char* s3_update_file(FILE* f, const char* filename, s3_config_t* cfg)
{
    if (s3_delete_file(filename, cfg) != 0)
        return NULL;

    return s3_upload_file(f, filename, cfg);
}