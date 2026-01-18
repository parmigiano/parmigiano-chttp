#include "routes.h"

#include "handlers.h"

void routes(void)
{
    cHTTPX_Route("GET", "/auth/signin", auth_signin_handler);
}