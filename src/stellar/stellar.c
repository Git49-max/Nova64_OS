#include <stellar/stellar.h>

#ifdef STELLAR_HOST
#include <stdio.h>
#else
// Precisarás de uma função para imprimir floats no teu Kernel
extern void print_int(long long, int, int, int); 
extern int cursor_x;
extern int cursor_y;
extern void print_double(double, int, int, int, int);
#endif

void stellar_init(StellarVM* vm, uint8_t* code, double* stack) {
    vm->code = code;
    vm->stack = stack;
    vm->pc = 0;
    vm->sp = -1;
    vm->running = 0;
    for(int i = 0; i < 26; i++) vm->variables[i] = 0.0;
}

void stellar_run(StellarVM* vm) {
    vm->running = 1;
    while (vm->running) {
        uint8_t opcode = vm->code[vm->pc++];
        switch (opcode) {
            case OP_PUSH: {
                // Lê 8 bytes do binário e interpreta como double
                double val = *(double*)&vm->code[vm->pc];
                vm->pc += 8;
                vm->stack[++vm->sp] = val;
                break;
            }
            case OP_ADD: {
                double b = vm->stack[vm->sp--];
                double a = vm->stack[vm->sp--];
                vm->stack[++vm->sp] = a + b;
                break;
            }
            case OP_SUB: {
                double b = vm->stack[vm->sp--];
                double a = vm->stack[vm->sp--];
                vm->stack[++vm->sp] = a - b;
                break;
            }
            case OP_MUL: {
                double b = vm->stack[vm->sp--];
                double a = vm->stack[vm->sp--];
                vm->stack[++vm->sp] = a * b;
                break;
            }
            case OP_DIV: {
                double b = vm->stack[vm->sp--];
                double a = vm->stack[vm->sp--];
                vm->stack[++vm->sp] = a / b;
                break;
            }
            case OP_STORE: {
                uint8_t var_idx = vm->code[vm->pc++];
                vm->variables[var_idx] = vm->stack[vm->sp--];
                break;
            }
            case OP_LOAD: {
                uint8_t var_idx = vm->code[vm->pc++];
                vm->stack[++vm->sp] = vm->variables[var_idx];
                break;
            }
            case OP_PRINT: {
                #ifdef STELLAR_HOST
                printf("%g\n", vm->stack[vm->sp]);
                #else
                print_double(vm->stack[vm->sp], 4, 0x0F, 0, cursor_y);
                if (vm->code[vm->pc] != OP_HALT) {
                    cursor_y++;
                }
                #endif
                break;
            }
            case OP_HALT:
                vm->running = 0;
                break;
        }
    }
}