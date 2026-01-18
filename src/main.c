#include <libchttpx/libchttpx.h>

#include "httpx.h"
#include "env.h"

int main()
{
    /* ENV.. */
    env_init(".env"); /* .env.production */

    /* Locale languages */
    // cHTTPX_i18n("/home/noneandundefined/Documents/Projects/parmigiano-http/src/infra/locale");

    /* Initial http server */
    http_init();
}