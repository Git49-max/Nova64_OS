#ifndef _HOST_TEST_
#define _HOST_TEST_
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Mock das funções de vídeo para o terminal do Linux
void print(char* s, uint8_t color, int x, int y) {
    printf("%s", s);
}

// Use 'unsigned long' para bater com o uint64_t do sistema no printf
void print_int(uint64_t val, uint8_t color, int x, int y) {
    printf("%lu\n", (unsigned long)val);
}

// Incluímos a implementação da VM
#include "../stellar/stellar.c"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: ./stellar_run arquivo.exe\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        printf("Erro ao abrir %s\n", argv[1]);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *bytecode = malloc(size);
    fread(bytecode, 1, size, f);
    fclose(f);

    uint64_t stack[8192]; 
    StellarVM vm;

    stellar_init(&vm, bytecode, stack);
    stellar_run(&vm);

    free(bytecode);
    return 0;
}