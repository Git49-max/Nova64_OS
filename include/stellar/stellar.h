#ifndef STELLAR_H
#define STELLAR_H

#include <stdint.h>

#ifdef STELLAR_HOST
#include <stdlib.h>
#else
#include "utils/string.h"
#endif

// --- Opcodes Utils ---

#define OP_HALT             0x00
#define OP_PUSH             0x01
#define OP_ADD              0x02
#define OP_SUB              0x03
#define OP_MUL              0x04
#define OP_DIV              0x05
#define OP_PRINT            0x06
#define OP_STORE            0x07
#define OP_LOAD             0x08
#define OP_CMP_EQ           0x09 // compare equal
#define OP_CMP_LT           0x0A // compare less than
#define OP_CMP_GT           0x0B // compare greater than
#define OP_JZ               0x0C 
#define OP_JMP              0x0D
#define OP_MOD              0x0E
#define OP_CMP_GTE          0x0F // compare greater than or equal
#define OP_CMP_LTE          0x10 // compare less than or equal
#define OP_CMP_NEQ          0x11 // compare not equal
#define OP_ELSE             0x12
#define OP_ELSE_IF          0x13
#define OP_PUSH_STR         0x14
#define OP_PRINT_STR        0x15
#define OP_ARRAY_MAKE       0x16
#define OP_ARRAY_GET        0x17
#define OP_ARRAY_SET        0x18
#define OP_INPUT            0x19

// --- Opcodes Double ---
#define OP_INC              0x1A 
#define OP_DEC              0x1B
#define OP_ADD_VAR          0x1C
#define OP_ADD_CONST        0x1D
#define OP_SUB_VAR          0x1E
#define OP_SUB_CONST        0x1F
#define OP_MUL_VAR          0x20
#define OP_MUL_CONST        0x21

// --- Opcodes Integer ---
#define OP_STORE_I          0x22
#define OP_LOAD_I           0x23
#define OP_INC_I            0x24
#define OP_DEC_I            0x25
#define OP_ADD_VAR_I        0x26
#define OP_ADD_CONST_I      0x27
#define OP_SUB_VAR_I        0x28
#define OP_SUB_CONST_I      0x29
#define OP_MUL_VAR_I        0x2A
#define OP_MUL_CONST_I      0x2B

typedef union {
    double d;
    long i;
    void* ptr;
} Register;

typedef struct {
    uint8_t* code;
    double* stack;
    Register variables[256];
    int pc;
    int sp;
    int running;
} StellarVM;

void vm_init(StellarVM* vm, uint8_t* code, double* stack);
void vm_run(StellarVM* vm);
int  compile(char* filename, char* source, uint8_t* out);


#endif