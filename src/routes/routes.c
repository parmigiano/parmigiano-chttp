#include "routes.h"

#include "handlers.h"

void routes(void)
{
    cHTTPX_Route("POST", "/api/v2/auth/login", auth_login_handler);
    cHTTPX_Route("POST", "/api/v2/auth/create", auth_create_handler);
    cHTTPX_Route("POST", "/api/v2/auth/verify", auth_verify_handler);
    cHTTPX_Route("GET", "/api/v2/users/me", user_me_handler);
}