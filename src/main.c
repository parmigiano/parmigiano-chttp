#include <libchttpx/libchttpx.h>

#include "httpx.h"
#include "utilities.h"

int main()
{
    /* ENV.. */
    env_init(".env"); /* .env.production */

    const char* i18n_locate = getenv("I18N_LOCATE");
    if (!i18n_locate)
    {
        fprintf(stderr, "i18n locate not found\n");
        return 0;
    }

    /* Locale languages */
    cHTTPX_i18n(i18n_locate);

    /* Initial http server */
    http_init();
}