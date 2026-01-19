#include "handlers.h"

#include "utilities.h"

#include <stdbool.h>

typedef struct
{
    char* name;
    char* username;
    char* email;
    char* password;
} user_create_t;

chttpx_response_t auth_login_handler(chttpx_request_t* req)
{
    return cHTTPX_ResJson(200, "{\"message\": \"%s\"}", "welcome");
}

chttpx_response_t auth_create_handler(chttpx_request_t* req)
{
    user_create_t payload;

    chttpx_validation_t fields[] = {
        chttpx_validation_str("name", true, 0, 25, &payload.name),
        chttpx_validation_str("username", true, 3, 25, &payload.username),
        chttpx_validation_str("email", true, 0, 255, &payload.email),
        chttpx_validation_str("password", true, 8, 255, &payload.password),
    };

    if (!cHTTPX_Parse(req, fields, (sizeof(fields) / sizeof(fields[0]))))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);
    }

    /* Trim spaces */
    trim_space(payload.name);
    trim_space(payload.username);
    trim_space(payload.email);
    trim_space(payload.password);

    /* Check symbols */
    is_valid(payload.name);
    is_valid(payload.username);

    /* To lower */
    to_lower(payload.email);
    to_lower(payload.password);

    if (!cHTTPX_Validate(req, fields, (sizeof(fields) / sizeof(fields[0]))))
    {
        return cHTTPX_ResJson(cHTTPX_StatusBadRequest, "{\"error\": \"%s\"}", req->error_msg);
    }
}