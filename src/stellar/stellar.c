#include <stellar/stellar.h>

#ifdef STELLAR_HOST
#include <stdio.h>
#else
extern int cursor_y;
extern void print_double(double, int, int, int, int);
#endif

void vm_init(StellarVM* vm, uint8_t* code, double* stack) {
    vm->code = code; vm->stack = stack;
    vm->pc = 0; vm->sp = -1; vm->running = 0;
    for(int i = 0; i < 26; i++) vm->variables[i] = 0.0;
}

void vm_run(StellarVM* vm) {
    vm->running = 1;
    while (vm->running) {
        uint8_t opcode = vm->code[vm->pc++];
        if (vm->sp >= 8190) { 
            printf("Stack Overflow real no PC: %d! sp: %d\n", vm->pc, vm->sp);
            exit(1);
        }
        switch (opcode) {
            case OP_PUSH: {
                double val;
                for(int i=0; i<8; i++) ((uint8_t*)&val)[i] = vm->code[vm->pc++];
                vm->stack[++vm->sp] = val;
                break;
            }
            case OP_ADD: { double b=vm->stack[vm->sp--]; double a=vm->stack[vm->sp--]; vm->stack[++vm->sp] = a+b; break; }
            case OP_SUB: { double b=vm->stack[vm->sp--]; double a=vm->stack[vm->sp--]; vm->stack[++vm->sp] = a-b; break; }
            case OP_MUL: { double b=vm->stack[vm->sp--]; double a=vm->stack[vm->sp--]; vm->stack[++vm->sp] = a*b; break; }
            case OP_DIV: { double b=vm->stack[vm->sp--]; double a=vm->stack[vm->sp--]; vm->stack[++vm->sp] = a/b; break; }
            case OP_CMP_EQ: { double b=vm->stack[vm->sp--]; double a=vm->stack[vm->sp--]; vm->stack[++vm->sp] = (a==b)?1.0:0.0; break; }
            case OP_CMP_NEQ: { double b=vm->stack[vm->sp--]; double a=vm->stack[vm->sp--]; vm->stack[++vm->sp] = (a!=b)?1.0:0.0; break; }
            case OP_CMP_LT: {
                double b = vm->stack[vm->sp--];
                double a = vm->stack[vm->sp--];
                vm->stack[++vm->sp] = (a < b) ? 1.0 : 0.0;
                break;
            }
            case OP_CMP_GT: {
                double b = vm->stack[vm->sp--]; 
                double a = vm->stack[vm->sp--]; 
                vm->stack[++vm->sp] = (a > b) ? 1.0 : 0.0; 
                break;
            }
            case OP_CMP_GTE:{
                double b = vm->stack[vm->sp--]; 
                double a = vm->stack[vm->sp--]; 
                vm->stack[++vm->sp] = (a >= b) ? 1.0 : 0.0; 
                break;
            }
            case OP_CMP_LTE: {
                double b = vm->stack[vm->sp--];
                double a = vm->stack[vm->sp--];
                vm->stack[++vm->sp] = (a <= b) ? 1.0 : 0.0;
                break;
            }
            case OP_JZ: {
                uint16_t addr = vm->code[vm->pc] | (vm->code[vm->pc + 1] << 8);
                if (vm->stack[vm->sp--] == 0.0) {
                    vm->pc = addr; 
                } else {
                    vm->pc += 2;   
                }
                break;
            }
            case OP_JMP: {
                uint16_t addr = vm->code[vm->pc] | (vm->code[vm->pc + 1] << 8);
                vm->pc = addr;
                break;
            }
            case OP_MOD: {
                double b = vm->stack[vm->sp--];
                double a = vm->stack[vm->sp--];
                vm->stack[++vm->sp] = (double)((int)a % (int)b);
                break;
            }
            case OP_STORE: vm->variables[vm->code[vm->pc++]] = vm->stack[vm->sp--]; break;
            case OP_LOAD:  vm->stack[++vm->sp] = vm->variables[vm->code[vm->pc++]]; break;
            case OP_PRINT:
                #ifdef STELLAR_HOST
                printf("%g\n", vm->stack[vm->sp--]);
                #else
                print_double(vm->stack[vm->sp--], 4, 0x0F, 0, cursor_y++);
                #endif
                break;
            case OP_HALT: vm->running = 0; break;
        }
    }
}