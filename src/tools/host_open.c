/*
This file IS NOT the release version of Stellar. The recent update brought cryptografy and obfuscation of bytecodes. For Stellar safety, the version that contains these functionalities will not be released for the public.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stellar/stellar.h>
#include <stellar/stellar_errors.h>

#include "../stellar/stellar.c"
#include "../stellar/stellar_compiler.c"
#include "../stellar/stellar_errors.c"

#define RED    "\033[1;31m"
#define RESET  "\033[0m"
#define BOLD   "\033[1m"

char* file_read_bin(const char* path, int* size_out) {
    FILE* f = fopen(path, "rb");
    if (!f) { printf(RED "Fatal Error: " RESET BOLD "cannot open %s\n", path); exit(1); }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* s = malloc(sz + 1);
    if (!s) exit(1);
    fread(s, 1, sz, f);
    fclose(f);
    if (size_out) *size_out = (int)sz;
    s[sz] = 0;
    return s;
}

void file_write(const char* path, uint8_t* data, int sz) {
    FILE* f = fopen(path, "wb");
    if (!f) { printf(RED "Error: " RESET "could not write to %s\n", path); exit(1); }
    fwrite(data, 1, sz, f);
    fclose(f);
}

int main(int argc, char** argv) {
    if (argc < 2) { 
        printf("Stellar Host\n");
        printf("Usage: stellar <file.ste> [options]\n");
        printf("Options:\n");
        printf("  -o <file>        Set output binary name\n");
        printf("  --run            Force run as binary (skip compilation)\n");
        printf("  --quiet          Compile but do not run the VM\n");
        printf("  --bytecode       Keep the generated binary even if no -o is used\n");
        printf("  -d               Dump bytecode (Hex) to terminal\n");
        printf("  --stack-size <n> Set VM stack size (default 8192)\n");
        return 1; 
    }

    char* filename = argv[1];
    int keep_bytecode = 0;
    int dump_bytecode = 0;
    int quiet_mode = 0;
    int force_run = 0;
    int stack_size = 8192;
    char out_name[256] = "";

    // Argument Parser
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--bytecode") == 0) keep_bytecode = 1;
        else if (strcmp(argv[i], "-d") == 0) dump_bytecode = 1;
        else if (strcmp(argv[i], "--quiet") == 0) quiet_mode = 1;
        else if (strcmp(argv[i], "--run") == 0) force_run = 1;
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) strcpy(out_name, argv[++i]);
        else if (strcmp(argv[i], "--stack-size") == 0 && i + 1 < argc) stack_size = atoi(argv[++i]);
    }

    char* dot = strrchr(filename, '.');
    int is_binary = (force_run || (dot && strcmp(dot, ".exe") == 0));

    uint8_t bin[65536];
    int sz = 0;
    char* src = NULL;

    if (is_binary) {
        int file_sz = 0;
        char* temp_bin = file_read_bin(filename, &file_sz);
        memcpy(bin, temp_bin, file_sz > 65536 ? 65536 : file_sz);
        sz = file_sz;
        free(temp_bin);
    } else {
        src = file_read_bin(filename, NULL);
        memset(bin, 0, 65536);
        sz = compile(filename, src, bin);

        int used_custom_output = (strlen(out_name) > 0);
        if (!used_custom_output) {
            strcpy(out_name, filename);
            char* dot_pos = strrchr(out_name, '.');
            if (dot_pos) strcpy(dot_pos, ".exe"); else strcat(out_name, ".exe");
        }

        file_write(out_name, bin, sz);
    }

        if (dump_bytecode) {
        printf(BOLD "\n--- Bytecode Dump (%d bytes) ---\n" RESET, sz);
        for (int i = 0; i < sz; i++) {
            printf("%02X ", bin[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n------------------------------\n");
    }

    if (!quiet_mode) {
        StellarVM vm;
        double* stack = malloc(stack_size * sizeof(double));
        if (!stack) { printf("Error: Could not allocate stack\n"); return 1; }

        vm_init(&vm, bin, stack);
        vm_run(&vm);

        if (!is_binary) {
            int used_custom_output = (strlen(out_name) > 0);
            if (!used_custom_output && !keep_bytecode) {
                remove(out_name);
            }
        }
        free(stack);
    } else if (!is_binary) {
        printf("Successfully compiled to: %s\n", out_name);
    }

    if (src) free(src);
    return 0;
}