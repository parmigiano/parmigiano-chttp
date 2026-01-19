#include "routes.h"

#include "handlers.h"

void routes(void)
{
    cHTTPX_Route("POST", "/auth/login", auth_login_handler);
    cHTTPX_Route("POST", "/auth/create", auth_create_handler);
}