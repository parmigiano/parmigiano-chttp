#include "utilities.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

int send_email(const char* to, const char* subject, const char* body)
{
    CURL* curl;
    CURLcode res = CURLE_OK;

    const char* smtp_server = getenv("SMTP_ADDR");
    const char* smtp_email = getenv("SMTP_EMAIL");
    const char* smtp_pass = getenv("SMTP_PASSWORD");
    const char* smtp_port = getenv("SMTP_PORT");

    if (!smtp_server || !smtp_email || !smtp_pass || !smtp_port)
    {
        fprintf(stderr, "SMTP environment variables not set\n");
        return 1;
    }

    char url[245];
    snprintf(url, sizeof(url), "smtp://%s/%s", smtp_server, smtp_port);

    char payload[2048];
    snprintf(payload, sizeof(payload),
             "To: %s\r\n"
             "From: %s\r\n"
             "Subject: %s\r\n"
             "\r\n"
             "%s\r\n",
             to, smtp_email, subject, body);

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_email);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_pass);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, smtp_email);

        struct curl_slist* recipients = NULL;
        recipients = curl_slist_append(recipients, to);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_READDATA, payload);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

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