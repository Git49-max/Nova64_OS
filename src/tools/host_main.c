#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stellar/stellar.h>
#include <stellar/stellar_errors.h>

#include "../stellar/stellar.c"
#include "../stellar/stellar_compiler.c"
#include "../stellar/stellar_errors.c"

#define RED   "\033[1;31m"
#define RESET "\033[0m"
#define BOLD  "\033[1m"

char* file_read(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { printf(RED "Fatal Error:" RESET BOLD "cannot open %s\n", path); exit(1); }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* s = malloc(sz + 1);
    fread(s, sz, 1, f);
    fclose(f);
    s[sz] = 0;
    return s;
}

void file_write(const char* path, uint8_t* data, int sz) {
    FILE* f = fopen(path, "wb");
    if (!f) exit(1);
    fwrite(data, 1, sz, f);
    fclose(f);
}

int main(int argc, char** argv) {
    if (argc < 2) { printf("Usage: stellar <file.ste>\n"); return 1; }

    char* filename = argv[1];
    char* src = file_read(filename);
    uint8_t bin[4096];

    int sz = compile(filename, src, bin);

    char out_name[256];
    strcpy(out_name, filename);
    char* dot = strrchr(out_name, '.');
    if (dot) strcpy(dot, ".exe"); else strcat(out_name, ".exe");

    file_write(out_name, bin, sz);

    StellarVM vm;
    double stack[8192];
    vm_init(&vm, bin, stack);
    vm_run(&vm);

    remove(out_name); 
    free(src);
    return 0;
}