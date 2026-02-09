#ifndef STELLAR_H
#define STELLAR_H

#include <stdint.h>

#ifdef STELLAR_HOST
#include <stdlib.h>
#else
#include "utils/string.h"
#endif

#define OP_HALT    0x00
#define OP_PUSH    0x01
#define OP_ADD     0x02
#define OP_SUB     0x03
#define OP_MUL     0x04
#define OP_DIV     0x05
#define OP_PRINT   0x06
#define OP_STORE   0x07
#define OP_LOAD    0x08
#define OP_CMP_EQ  0x09 //compare equal
#define OP_CMP_LT  0x0A //compare less than
#define OP_CMP_GT  0x0B //compare greater than
#define OP_JZ      0x0C 
#define OP_JMP     0x0D
#define OP_MOD     0x0E
#define OP_CMP_GTE 0x0F //compare greater than or equal
#define OP_CMP_LTE 0x10 //compare less than or equal
#define OP_CMP_NEQ 0x11 //compare not equal

typedef struct {
    uint8_t* code;
    double* stack;
    double variables[26];
    int pc;
    int sp;
    int running;
} StellarVM;

void vm_init(StellarVM* vm, uint8_t* code, double* stack);
void vm_run(StellarVM* vm);
int  compile(char* filename, char* source, uint8_t* out);


#endif