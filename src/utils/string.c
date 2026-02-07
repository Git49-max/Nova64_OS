#include "utils/string.h"
#include "utils/types.h"

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
int strncmp(const char* s1, const char* s2, uint64_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}
double atof(const char* s) {
    double res = 0.0, fact = 1.0;
    int point_seen = 0;
    if (*s == '-') { s++; fact = -1.0; }

    for (; *s; s++) {
        if (*s == '.') { point_seen = 1; continue; }
        int d = *s - '0';
        if (d >= 0 && d <= 9) {
            if (point_seen) fact /= 10.0;
            res = res * 10.0 + (double)d;
        }
    }
    return res * fact;
}