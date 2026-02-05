#define _HOST_TEST_
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../stellar/stellar_compiler.c"
#include "../stellar/stellar.c"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: ./stellar arquivo.ste\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) return 1;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *src = malloc(size + 1);
    fread(src, 1, size, f);
    src[size] = '\0';
    fclose(f);

    uint8_t bin[1024];
    int bin_size = stellar_compile(src, bin);

    // Salva o .exe com nome limpo
    char out_name[256];
    strncpy(out_name, argv[1], 250);
    char* dot = strrchr(out_name, '.');
    if(dot) *dot = '\0';
    strcat(out_name, ".exe");

    FILE *f_out = fopen(out_name, "wb");
    fwrite(bin, 1, bin_size, f_out);
    fclose(f_out);

    // Roda a VM
    double stack[1024];
    StellarVM vm;
    stellar_init(&vm, bin, stack);
    stellar_run(&vm);

    free(src);
    return 0;
}