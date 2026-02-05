#ifndef STELLAR_H
#define STELLAR_H

#include "utils/types.h"

typedef enum {
    OP_HALT  = 0x00,  
    OP_PUSH  = 0x01,  
    OP_ADD   = 0x02,  
    OP_PRINT = 0x03   
} OpCode;

typedef struct {
    uint8_t* code;      
    uint64_t* stack;    
    uint32_t pc;        
    uint32_t sp;        
    int running;      
} StellarVM;

#endif