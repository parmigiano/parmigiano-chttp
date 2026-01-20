#ifndef UTILITIES_H
#define UTILITIES_H

#include <ctype.h>
#include <stdbool.h>

#define ARGON2_HASH_LEN 256
#define SIMPLE_PASSWORDS_COUNT 80

/* string.c */
void trim_space(char *str);
bool is_valid(const char *str);
void to_lower(char *str);

/* password.c */
bool is_simple_password(const char *password);
char* hash_password(const char *password);
int verify_password(const char *password, const char *hash);

/* env.c */
int env_init(const char* filename);

/* email.c */
int send_email(const char *to, const char *subject, const char *body);

#endif