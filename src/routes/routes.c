#include "routes.h"

#include "handlers.h"

void routes(void)
{
    chttpx_router_t v2 = cHTTPX_RoutePathPrefix("/api/v2");

    /* Auth routes */
    cHTTPX_RegisterRoute(&v2, "POST", "/auth/login", auth_login_handler);
    cHTTPX_RegisterRoute(&v2, "POST", "/auth/create", auth_create_handler);
    cHTTPX_RegisterRoute(&v2, "POST", "/auth/verify", auth_verify_handler);
    /* User routes */
    cHTTPX_RegisterRoute(&v2, "GET", "/users/me", user_me_handler);
    cHTTPX_RegisterRoute(&v2, "PATCH", "/users/me/avatar", user_upload_avatar_handler);
}