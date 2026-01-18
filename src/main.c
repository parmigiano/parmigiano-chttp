#include "httpx.h"
#include "env.h"

int main()
{
    /* ENV.. */
    env_init(".env"); /* .env.production */

    /* Initial http server */
    http_init();
}