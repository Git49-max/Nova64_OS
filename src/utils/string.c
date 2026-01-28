#include "utils/string.h"

int strlen(char *s) {
    int i = 0;
    while (s[i] != '\0') i++;
    return i;
}

int strcmp(char *s1, char *s2) {
    int i = 0;
    while (s1[i] == s2[i]) {
        if (s1[i] == '\0') return 0;
        i++;
    }
    return s1[i] - s2[i];
}

void strcpy(char *dest, char *src) {
    int i = 0;
    while ((dest[i] = src[i]) != '\0') i++;
}

String string_create(char *s) {
    String str;
    strcpy(str.data, s);
    str.length = strlen(s);
    return str;
}

void string_append(String *str, char c) {
    if (str->length < 254) {
        str->data[str->length] = c;
        str->length++;
        str->data[str->length] = '\0';
    }
}

void string_clear(String *str) {
    str->data[0] = '\0';
    str->length = 0;
}