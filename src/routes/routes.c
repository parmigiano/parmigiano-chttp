#include "routes.h"

#include "handlers.h"

void routes(void)
{
    chttpx_router_t v2 = cHTTPX_RoutePathPrefix("/api/v2");

    /* Swagger routes */
    cHTTPX_RegisterRoute(&v2, "GET", "/doc.api/swagger/json", swagger_json_handler_v2);
    cHTTPX_RegisterRoute(&v2, "GET", "/doc.api/swagger/gui", swagger_gui_handler_v2);
    /* Auth routes */
    cHTTPX_RegisterRoute(&v2, "POST", "/auth/login", auth_login_handler_v2);
    cHTTPX_RegisterRoute(&v2, "POST", "/auth/create", auth_create_handler_v2);
    cHTTPX_RegisterRoute(&v2, "POST", "/auth/verify", auth_verify_handler_v2);
    cHTTPX_RegisterRoute(&v2, "DELETE", "/auth/delete", auth_delete_handler_v2);
    /* User routes */
    cHTTPX_RegisterRoute(&v2, "GET", "/users/me", user_me_handler_v2);
    cHTTPX_RegisterRoute(&v2, "PATCH", "/users/me", user_update_profile_handler_v2);
    cHTTPX_RegisterRoute(&v2, "PATCH", "/users/me/avatar", user_upload_avatar_handler_v2);
    cHTTPX_RegisterRoute(&v2, "GET", "/users/{user_uid}", user_get_profile_handler_v2);
    /* Chat routes */
    /* /chats?offset=0 */
    cHTTPX_RegisterRoute(&v2, "GET", "/chats", chat_get_my_history_handler_v2);
}