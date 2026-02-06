#ifndef STELLAR_H
#define STELLAR_H

#include <stdint.h>

#define OP_HALT  0x00
#define OP_PUSH  0x01
#define OP_ADD   0x02
#define OP_SUB   0x03
#define OP_MUL   0x04
#define OP_DIV   0x05
#define OP_PRINT 0x06
#define OP_STORE 0x07  // Novo: Salva na variável
#define OP_LOAD  0x08  // Novo: Lê da variável

typedef struct {
    uint8_t* code;
    double* stack;
    double variables[26]; // 26 espaços para a-z
    int pc;
    int sp;
    int running;
} StellarVM;

void stellar_init(StellarVM* vm, uint8_t* code, double* stack);
void stellar_run(StellarVM* vm);
int stellar_compile(char* source, uint8_t* binary_out);

#endif