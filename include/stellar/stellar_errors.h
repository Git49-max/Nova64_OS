#ifndef STELLAR_ERRORS_H
#define STELLAR_ERRORS_H

#ifdef STELLAR_HOST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RED   "\033[1;31m"
#define RESET "\033[0m"
#define BOLD  "\033[1m"
#else
#include "utils/string.h"
#define NULL ((void*)0)
#endif

// Protótipos das funções que você escolheu mover
void error_exit(char* file, char* src, int ptr, char* msg, int len, char* suggest);
static int is_similar(const char* s1, const char* s2);

#endif