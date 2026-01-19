#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdbool.h>

void trim_space(char *str);

bool is_valid(const char *str);

void to_lower(char *str);

int env_init(const char* filename);

#endif