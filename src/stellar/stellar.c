#include <stellar/stellar.h>
#include <string.h>

#ifdef STELLAR_HOST
    #include <stdio.h>
    #include <stdlib.h> // Para malloc e free
    #include <stdint.h>
#else
    extern int cursor_y;
    extern void print_double(double, int, int, int, int);
#endif

void vm_init(StellarVM* vm, uint8_t* code, double* stack) {
    vm->code = code;
    vm->stack = stack;
    vm->pc = 0;
    vm->sp = -1;
    vm->running = 0;
    for (int i = 0; i < 256; i++) {
        vm->variables[i].d = 0.0;
    }
}

void vm_run(StellarVM* vm) {
    static void* dispatch_table[] = {
        &&do_halt,        // 0x00
        &&do_push,        // 0x01
        &&do_add,         // 0x02
        &&do_sub,         // 0x03
        &&do_mul,         // 0x04
        &&do_div,         // 0x05
        &&do_print,       // 0x06
        &&do_store,       // 0x07
        &&do_load,        // 0x08
        &&do_cmp_eq,      // 0x09
        &&do_cmp_lt,      // 0x0A
        &&do_cmp_gt,      // 0x0B
        &&do_jz,          // 0x0C
        &&do_jmp,         // 0x0D
        &&do_mod,         // 0x0E
        &&do_cmp_gte,     // 0x0F
        &&do_cmp_lte,     // 0x10
        &&do_cmp_neq,     // 0x11
        &&do_else,        // 0x12 
        &&do_else_if,     // 0x13 
        &&do_push_str,    // 0x14
        &&do_print_str,   // 0x15
        &&do_array_make,  // 0x16
        &&do_array_get,   // 0x17
        &&do_array_set,   // 0x18
        &&do_input,       // 0x19
        &&do_inc,         // 0x1A
        &&do_dec,         // 0x1B
        &&do_add_var,     // 0x1C
        &&do_add_const,   // 0x1D
        &&do_sub_var,     // 0x1E
        &&do_sub_const,   // 0x1F
        &&do_mul_var,     // 0x20
        &&do_mul_const,   // 0x21
        &&do_store_i,     // 0x22
        &&do_load_i,      // 0x23
        &&do_inc_i,       // 0x24
        &&do_dec_i,       // 0x25
        &&do_add_var_i,   // 0x26
        &&do_add_const_i, // 0x27
        &&do_sub_var_i,   // 0x28
        &&do_sub_const_i, // 0x29
        &&do_mul_var_i,   // 0x2A
        &&do_mul_const_i  // 0x2B
    };

    #define NEXT_OP() goto *dispatch_table[vm->code[vm->pc++]]

    vm->running = 1;
    NEXT_OP();

    /* --- BASIC OPS --- */

    do_push: {
        double val;
        memcpy(&val, &vm->code[vm->pc], 8);
        vm->pc += 8;
        vm->stack[++vm->sp] = val;
        NEXT_OP();
    }

    do_add: { 
        double b = vm->stack[vm->sp--]; 
        double a = vm->stack[vm->sp--]; 
        vm->stack[++vm->sp] = a + b; 
        NEXT_OP(); 
    }

    do_sub: { 
        double b = vm->stack[vm->sp--]; 
        double a = vm->stack[vm->sp--]; 
        vm->stack[++vm->sp] = a - b; 
        NEXT_OP(); 
    }

    do_mul: { 
        double b = vm->stack[vm->sp--]; 
        double a = vm->stack[vm->sp--]; 
        vm->stack[++vm->sp] = a * b; 
        NEXT_OP(); 
    }

    do_div: { 
        double b = vm->stack[vm->sp--]; 
        double a = vm->stack[vm->sp--]; 
        vm->stack[++vm->sp] = a / b; 
        NEXT_OP(); 
    }

    do_mod: {
        double b = vm->stack[vm->sp--];
        double a = vm->stack[vm->sp--];
        vm->stack[++vm->sp] = (double)((int)a % (int)b);
        NEXT_OP();
    }

    /* --- COMPARISON OPS --- */

    do_cmp_eq: { 
        double b = vm->stack[vm->sp--]; 
        double a = vm->stack[vm->sp--]; 
        vm->stack[++vm->sp] = (a == b) ? 1.0 : 0.0; 
        NEXT_OP(); 
    }

    do_cmp_neq: { 
        double b = vm->stack[vm->sp--]; 
        double a = vm->stack[vm->sp--]; 
        vm->stack[++vm->sp] = (a != b) ? 1.0 : 0.0; 
        NEXT_OP(); 
    }

    do_cmp_lt: { 
        double b = vm->stack[vm->sp--]; 
        double a = vm->stack[vm->sp--]; 
        vm->stack[++vm->sp] = (a < b) ? 1.0 : 0.0; 
        NEXT_OP(); 
    }

    do_cmp_gt: { 
        double b = vm->stack[vm->sp--]; 
        double a = vm->stack[vm->sp--]; 
        vm->stack[++vm->sp] = (a > b) ? 1.0 : 0.0; 
        NEXT_OP(); 
    }

    do_cmp_gte: { 
        double b = vm->stack[vm->sp--]; 
        double a = vm->stack[vm->sp--]; 
        vm->stack[++vm->sp] = (a >= b) ? 1.0 : 0.0; 
        NEXT_OP(); 
    }

    do_cmp_lte: { 
        double b = vm->stack[vm->sp--]; 
        double a = vm->stack[vm->sp--]; 
        vm->stack[++vm->sp] = (a <= b) ? 1.0 : 0.0; 
        NEXT_OP(); 
    }

    /* --- CONTROL FLOW --- */

    do_jz: {
        uint16_t addr = vm->code[vm->pc] | (vm->code[vm->pc + 1] << 8);
        if (vm->stack[vm->sp--] == 0.0) vm->pc = addr; 
        else vm->pc += 2;   
        NEXT_OP();
    }

    do_jmp: {
        uint16_t addr = vm->code[vm->pc] | (vm->code[vm->pc + 1] << 8);
        vm->pc = addr;
        NEXT_OP();
    }

    /* --- DOUBLE REGISTER OPS --- */

    do_store: 
        vm->variables[vm->code[vm->pc++]].d = vm->stack[vm->sp--]; 
        NEXT_OP();

    do_load:  
        vm->stack[++vm->sp] = vm->variables[vm->code[vm->pc++]].d; 
        NEXT_OP();

    do_inc:
        vm->variables[vm->code[vm->pc++]].d += 1.0;
        NEXT_OP();

    do_dec:
        vm->variables[vm->code[vm->pc++]].d -= 1.0;
        NEXT_OP();

    do_add_var: {
        uint8_t d = vm->code[vm->pc++]; 
        uint8_t s = vm->code[vm->pc++];
        vm->variables[d].d += vm->variables[s].d;
        NEXT_OP();
    }

    do_add_const: {
        uint8_t idx = vm->code[vm->pc++]; 
        double v;
        for(int i=0; i<8; i++) ((uint8_t*)&v)[i] = vm->code[vm->pc++];
        vm->variables[idx].d += v;
        NEXT_OP();
    }

    do_sub_var: {
        uint8_t d = vm->code[vm->pc++]; 
        uint8_t s = vm->code[vm->pc++];
        vm->variables[d].d -= vm->variables[s].d;
        NEXT_OP();
    }

    do_sub_const: {
        uint8_t idx = vm->code[vm->pc++]; 
        double v;
        for(int i=0; i<8; i++) ((uint8_t*)&v)[i] = vm->code[vm->pc++];
        vm->variables[idx].d -= v;
        NEXT_OP();
    }

    do_mul_var: {
        uint8_t d = vm->code[vm->pc++]; 
        uint8_t s = vm->code[vm->pc++];
        vm->variables[d].d *= vm->variables[s].d;
        NEXT_OP();
    }

    do_mul_const: {
        uint8_t idx = vm->code[vm->pc++]; 
        double v;
        for(int i=0; i<8; i++) ((uint8_t*)&v)[i] = vm->code[vm->pc++];
        vm->variables[idx].d *= v;
        NEXT_OP();
    }

    /* --- INTEGER REGISTER OPS --- */

    do_load_i:
        vm->stack[++vm->sp] = (double)vm->variables[vm->code[vm->pc++]].i; 
        NEXT_OP();

    do_store_i:
        vm->variables[vm->code[vm->pc++]].i = (long)vm->stack[vm->sp--]; 
        NEXT_OP();

    do_inc_i:
        vm->variables[vm->code[vm->pc++]].i++;
        NEXT_OP();

    do_dec_i:
        vm->variables[vm->code[vm->pc++]].i--;
        NEXT_OP();

    do_add_var_i: {
        uint8_t d = vm->code[vm->pc++]; 
        uint8_t s = vm->code[vm->pc++];
        vm->variables[d].i += vm->variables[s].i;
        NEXT_OP();
    }

    do_add_const_i: {
        uint8_t idx = vm->code[vm->pc++]; 
        long v;
        for(int i=0; i<8; i++) ((uint8_t*)&v)[i] = vm->code[vm->pc++];
        vm->variables[idx].i += v;
        NEXT_OP();
    }

    do_sub_var_i: {
        uint8_t d = vm->code[vm->pc++]; 
        uint8_t s = vm->code[vm->pc++];
        vm->variables[d].i -= vm->variables[s].i;
        NEXT_OP();
    }

    do_sub_const_i: {
        uint8_t idx = vm->code[vm->pc++]; 
        long v;
        for(int i=0; i<8; i++) ((uint8_t*)&v)[i] = vm->code[vm->pc++];
        vm->variables[idx].i -= v;
        NEXT_OP();
    }

    do_mul_var_i: {
        uint8_t d = vm->code[vm->pc++]; 
        uint8_t s = vm->code[vm->pc++];
        vm->variables[d].i *= vm->variables[s].i;
        NEXT_OP();
    }

    do_mul_const_i: {
        uint8_t idx = vm->code[vm->pc++]; 
        long v;
        for(int i=0; i<8; i++) ((uint8_t*)&v)[i] = vm->code[vm->pc++];
        vm->variables[idx].i *= v;
        NEXT_OP();
    }

    /* --- SYSTEM & MEMORY OPS --- */

    do_print:
        #ifdef STELLAR_HOST
            printf("%g\n", vm->stack[vm->sp--]);
        #else
            print_double(vm->stack[vm->sp--], 4, 0x0F, 0, cursor_y++);
        #endif
        NEXT_OP();

    do_push_str: {
        uint16_t addr = vm->code[vm->pc] | (vm->code[vm->pc + 1] << 8);
        vm->pc += 2;
        vm->stack[++vm->sp] = (double)(uintptr_t)addr;
        NEXT_OP();
    }

    do_print_str: {
        int offset = (int)vm->stack[vm->sp--];
        char* s = (char*)&vm->code[offset];
        #ifdef STELLAR_HOST
            printf("%s\n", s);
        #endif
        NEXT_OP();
    }

    do_array_make: {
        int size = (int)vm->stack[vm->sp--];
        double* new_array = (double*)malloc(size * sizeof(double));
        for(int i = 0; i < size; i++) new_array[i] = 0.0;
        vm->stack[++vm->sp] = (double)(uintptr_t)new_array;
        NEXT_OP();
    }

    do_array_set: {
        double value = vm->stack[vm->sp--];
        int index = (int)(vm->stack[vm->sp--] + 0.5); // Arredonda para o inteiro mais próximo
        double* array = (double*)(uintptr_t)vm->stack[vm->sp--];
        if (array) array[index] = value;
        NEXT_OP();
    }

    do_array_get: {
        int index = (int)(vm->stack[vm->sp--] + 0.5);
        double* array = (double*)(uintptr_t)vm->stack[vm->sp--];
        vm->stack[++vm->sp] = array ? array[index] : 0.0;
        NEXT_OP();
    }

    do_input: {
        double val;
        #ifdef STELLAR_HOST
            if (scanf("%lf", &val) != 1) val = 0.0; 
        #endif
        vm->stack[++vm->sp] = val;
        NEXT_OP();
    }

    do_else:
    do_else_if:
        NEXT_OP();

    do_halt:
        vm->running = 0;
        return;
}