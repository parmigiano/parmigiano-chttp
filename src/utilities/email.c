#include "utilities.h"

#include "log.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct upload_status
{
    const char* data;
    size_t bytes_read;
    size_t data_len;
};

static size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp)
{
    struct upload_status* ctx = (struct upload_status*)userp;

    if (ctx->bytes_read >= ctx->data_len) return 0;

    size_t max = size * nmemb;
    size_t remaining = ctx->data_len - ctx->bytes_read;
    size_t to_copy = remaining < max ? remaining : max;

    memcpy(ptr, ctx->data + ctx->bytes_read, to_copy);
    ctx->bytes_read += to_copy;
    return to_copy;
}

static void rfc2822_date(char* buffer, size_t size)
{
    time_t t = time(NULL);
    struct tm tm_info;
    gmtime_r(&t, &tm_info);
    strftime(buffer, size, "%a, %d %b %Y %H:%M:%S +0000", &tm_info);
}

static void generate_message_id(char* buffer, size_t size, const char* domain)
{
    snprintf(buffer, size, "<%ld.%ld@%s>", time(NULL), random(), domain);
}

int send_email(const char* to, const char* subject, const char* body, const char* cc)
{
    CURL* curl;
    CURLcode res = CURLE_OK;

    const char* smtp_server = getenv("SMTP_ADDR");
    const char* smtp_email = getenv("SMTP_EMAIL");
    const char* smtp_pass = getenv("SMTP_PASSWORD");
    const char* smtp_port = getenv("SMTP_PORT");
    const char* smtp_domain = getenv("SMTP_DOMAIN");

    if (!smtp_server || !smtp_email || !smtp_pass || !smtp_port || !smtp_domain)
    {
        fprintf(stderr, "SMTP environment variables not set\n");
        return 1;
    }

    char date[128];
    char message_id[128];

    rfc2822_date(date, sizeof(date));
    generate_message_id(message_id, sizeof(message_id), smtp_domain);

    char payload[4096];
    snprintf(payload, sizeof(payload),
             "Date: %s\r\n"
             "To: %s\r\n"
             "From: %s\r\n"
             "Cc: %s\r\n"
             "Message-ID: %s\r\n"
             "Subject: %s\r\n"
             "Content-Type: text/html; charset=UTF-8\r\n"
             "\r\n"
             "%s\r\n",
             date, to, smtp_email, cc ? cc : "", message_id, subject, body);

    struct upload_status upload_ctx = {payload, 0, strlen(payload)};

    curl = curl_easy_init();
    if (curl)
    {
        char url[245];
        snprintf(url, sizeof(url), "smtp://%s:%s", smtp_server, smtp_port);

        struct curl_slist* recipients = NULL;
        recipients = curl_slist_append(recipients, to);
        if (cc)
            recipients = curl_slist_append(recipients, cc);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_email);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_pass);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, smtp_email);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            fprintf(stderr, "Email send error: %s\n", curl_easy_strerror(res));
        }
        else
        {
            log_info("Email sent: %s\n", to);
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }

    return (int)res;
}