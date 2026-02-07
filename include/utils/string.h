#ifndef STRING_H
#define STRING_H

typedef struct {
    char data[256];
    int length;
} String;

#include "utils/types.h"

int strlen(char *s);
int strcmp(char *s1, char *s2);
void strcpy(char *dest, char *src);
void strcat(char *dest, char *src);

String string_create(char *s);
void string_append(String *str, char c);
void string_clear(String *str);
int strncmp(const char* s1, const char* s2, uint64_t n);
double atof(const char* s);
#endif