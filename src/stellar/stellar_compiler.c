#include <stellar/stellar.h>
#include <stellar/stellar_errors.h>

#ifdef STELLAR_HOST
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stddef.h>
    #include <ctype.h> 
    #define RED   "\033[1;31m"
    #define RESET "\033[0m"
    #define BOLD  "\033[1m"
#else
    #include "utils/string.h"
    #define NULL ((void*)0)
    extern int cursor_y;
#endif

// --- SYMBOL TABLE ---
typedef struct {
    char name[32];
    int id;
} Symbol;

static Symbol symbol_table[256];
static int symbol_count = 0;

static int find_variable(char* name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) return symbol_table[i].id;
    }
    return -1;
}

static int add_variable(char* name) {
    if (symbol_count >= 256) return -1;
    strncpy(symbol_table[symbol_count].name, name, 31);
    symbol_table[symbol_count].id = symbol_count;
    return symbol_count++;
}

static int match(char* src, int ptr, char* word) {
    int i = 0;
    while (word[i] != '\0') {
        if (src[ptr + i] != word[i]) return 0;
        i++;
    }
    char next = src[ptr + i];
    if (isalnum(next) || next == '_') return 0;
    return i;
}

static void patch16(uint8_t* out, int pos, uint16_t val) {
    out[pos] = val & 0xFF;
    out[pos + 1] = (val >> 8) & 0xFF;
}

int compile(char* filename, char* source, uint8_t* out) {
    int s_ptr = 0, b_ptr = 0;
    uint8_t p_add_sub = 0, p_mul_div = 0, p_cmp = 0;
    int var_store = -1; 
    int expect_print = 0;
    int force_semicolon_check = 0; 
    int paren_depth = 0;
    
    symbol_count = 0;

    int if_stack[16], if_top = -1;
    int while_start[16], while_exit[16], while_top = -1;
    int scope_stack[32], scope_top = -1;
    int expecting_cond_stack[32] = {0}; 
    
    // Pilha para gerenciar os JMPs de saída da estrutura IF/ELSE completa
    int exit_jmp_stack[32], exit_jmp_top = -1;

    while (source[s_ptr] != '\0') {
        char c = source[s_ptr];

        if (isspace(c)) { s_ptr++; continue; }

        if (c == ';') {
            if (p_mul_div) { out[b_ptr++] = p_mul_div; p_mul_div = 0; }
            if (p_add_sub) { out[b_ptr++] = p_add_sub; p_add_sub = 0; }
            if (p_cmp)     { out[b_ptr++] = p_cmp; p_cmp = 0; }
            
            if (var_store != -1) {
                out[b_ptr++] = OP_STORE; 
                out[b_ptr++] = (uint8_t)var_store;
                var_store = -1;
            }
            force_semicolon_check = 0;
            s_ptr++;
            continue;
        }

        if (force_semicolon_check) {
             error_exit(filename, source, s_ptr, "Expected ';' after previous instruction", 1, NULL);
        }

        if (c == '/' && source[s_ptr+1] == '/') {
            while (source[s_ptr] != '\n' && source[s_ptr] != '\0') s_ptr++;
            continue;
        }

        // --- IF / ELSE IF / ELSE ---
        int len;
        if ((len = match(source, s_ptr, "if"))) {
            if (var_store != -1) error_exit(filename, source, s_ptr, "Missing ';' before 'if'", len, NULL);
            scope_stack[++scope_top] = 0; 
            expecting_cond_stack[scope_top] = 1;
            s_ptr += len; continue;
        }

        if ((len = match(source, s_ptr, "else"))) {
            if (var_store != -1) error_exit(filename, source, s_ptr, "Missing ';' before 'else'", len, NULL);
            
            // Antes de entrar no else, o bloco anterior deve pular para o fim de tudo
            out[b_ptr++] = OP_JMP;
            exit_jmp_stack[++exit_jmp_top] = b_ptr;
            out[b_ptr++] = 0; out[b_ptr++] = 0;

            // O JZ do bloco anterior agora aponta para este else
            patch16(out, if_stack[if_top--], (uint16_t)b_ptr);

            s_ptr += len;
            while (isspace(source[s_ptr])) s_ptr++;

            if ((len = match(source, s_ptr, "if"))) {
                scope_stack[++scope_top] = 0; // Trata como novo IF dentro da cadeia
                expecting_cond_stack[scope_top] = 1;
                s_ptr += len;
            } else {
                scope_stack[++scope_top] = 2; // ELSE puro
            }
            continue;
        }

        // --- Comparações ---
        if (c == '=' && source[s_ptr+1] == '=') {
            if (p_mul_div) { out[b_ptr++] = p_mul_div; p_mul_div = 0; }
            if (p_add_sub) { out[b_ptr++] = p_add_sub; p_add_sub = 0; }
            p_cmp = OP_CMP_EQ; s_ptr += 2; continue;
        }
        if (c == '!' && source[s_ptr+1] == '=') {
            if (p_mul_div) { out[b_ptr++] = p_mul_div; p_mul_div = 0; }
            if (p_add_sub) { out[b_ptr++] = p_add_sub; p_add_sub = 0; }
            p_cmp = OP_CMP_NEQ; s_ptr += 2; continue;
        }
        if (c == '<' || c == '>') {
            if (p_mul_div) { out[b_ptr++] = p_mul_div; p_mul_div = 0; }
            if (p_add_sub) { out[b_ptr++] = p_add_sub; p_add_sub = 0; }
            char op = c;
            s_ptr++;
            if (source[s_ptr] == '=') {
                p_cmp = (op == '<') ? OP_CMP_LTE : OP_CMP_GTE;
                s_ptr++;
            } else {
                p_cmp = (op == '<') ? OP_CMP_LT : OP_CMP_GT;
            }
            continue;
        }

        if ((len = match(source, s_ptr, "while"))) {
            if (var_store != -1) error_exit(filename, source, s_ptr, "Missing ';' before 'while'", len, NULL);
            while_start[++while_top] = b_ptr;
            scope_stack[++scope_top] = 1; 
            expecting_cond_stack[scope_top] = 1;
            s_ptr += len; continue;
        }

        if ((len = match(source, s_ptr, "let"))) {
            if (var_store != -1) error_exit(filename, source, s_ptr, "Missing ';' before 'let'", len, NULL);
            s_ptr += len; while (isspace(source[s_ptr])) s_ptr++;
            
            char var_name[32] = {0}; int p = 0;
            while (isalnum(source[s_ptr]) || source[s_ptr] == '_') {
                if(p < 31) var_name[p++] = source[s_ptr++]; else s_ptr++;
            }
            
            if (find_variable(var_name) == -1) {
                if (add_variable(var_name) == -1) 
                    error_exit(filename, source, s_ptr, "Too many variables (limit 256)", 1, NULL);
            }
            
            while (isspace(source[s_ptr])) s_ptr++;
            if (source[s_ptr] == '=') { var_store = find_variable(var_name); s_ptr++; }
            continue;
        }

        if ((len = match(source, s_ptr, "print"))) { 
            if (var_store != -1) error_exit(filename, source, s_ptr, "Missing ';' before 'print'", len, NULL);
            expect_print = 1; s_ptr += len; continue; 
        }

        if (c == '(' ) { paren_depth++; s_ptr++; continue; }
        if (c == '{' ) { if (var_store != -1) error_exit(filename, source, s_ptr, "Missing ';' before '{'", 1, NULL); s_ptr++; continue; }

        if (c == ')') {
            paren_depth--;
            if (p_mul_div) { out[b_ptr++] = p_mul_div; p_mul_div = 0; }
            if (p_add_sub) { out[b_ptr++] = p_add_sub; p_add_sub = 0; }
            if (p_cmp)     { out[b_ptr++] = p_cmp; p_cmp = 0; }
            
            if (paren_depth == 0 && scope_top >= 0 && expecting_cond_stack[scope_top]) {
                out[b_ptr++] = OP_JZ;
                int patch_pos = b_ptr; out[b_ptr++] = 0; out[b_ptr++] = 0;
                if (scope_stack[scope_top] == 0) if_stack[++if_top] = patch_pos;
                else while_exit[while_top] = patch_pos;
                expecting_cond_stack[scope_top] = 0;
            }
            if (expect_print) { out[b_ptr++] = OP_PRINT; expect_print = 0; force_semicolon_check = 1; }
            s_ptr++; continue;
        }

        if (c == '}') {
            if (var_store != -1) error_exit(filename, source, s_ptr, "Missing ';' before '}'", 1, NULL);
            if (scope_top < 0) error_exit(filename, source, s_ptr, "Unmatched '}'", 1, NULL);
            
            int type = scope_stack[scope_top--];
            if (type == 1) { // WHILE
                out[b_ptr++] = OP_JMP;
                patch16(out, b_ptr, (uint16_t)while_start[while_top]); b_ptr += 2;
                patch16(out, while_exit[while_top--], (uint16_t)b_ptr);
            } 
            else { // IF ou ELSE
                int next = s_ptr + 1;
                while (isspace(source[next])) next++;
                if (match(source, next, "else") == 0) {
                    if (type == 0) patch16(out, if_stack[if_top--], (uint16_t)b_ptr);
                    while (exit_jmp_top >= 0) {
                        patch16(out, exit_jmp_stack[exit_jmp_top--], (uint16_t)b_ptr);
                    }
                }
            }
            s_ptr++; continue;
        }

        if (isdigit(c)) {
            char buf[32]; int p = 0;
            while (isdigit(source[s_ptr]) || source[s_ptr] == '.') buf[p++] = source[s_ptr++];
            buf[p] = '\0';
            out[b_ptr++] = OP_PUSH;
            double v = atof(buf);
            for (int i = 0; i < 8; i++) out[b_ptr++] = ((uint8_t*)&v)[i];
            if (p_mul_div) { out[b_ptr++] = p_mul_div; p_mul_div = 0; }
            continue;
        }

        if (isalpha(c)) {
            int start = s_ptr;
            while (isalnum(source[s_ptr]) || source[s_ptr] == '_') s_ptr++;
            int w_len = s_ptr - start;
            int chk = s_ptr;
            while (isspace(source[chk])) chk++;

            char word[32] = {0};
            strncpy(word, &source[start], (w_len < 31 ? w_len : 31));

            // --- PRESERVAÇÃO: Unknown Function Call + Sugestão ---
            if (source[chk] == '(') {
                char* suggestion = NULL;
                const char* valid_functions[] = { "print", NULL };
                for (int i = 0; valid_functions[i] != NULL; i++) {
                    if (is_similar(word, valid_functions[i])) {
                        suggestion = (char*)valid_functions[i];
                        break;
                    }
                }
                error_exit(filename, source, start, "Unknown function call", w_len, suggestion);
            }

            int idx = find_variable(word);
            if (idx == -1) error_exit(filename, source, start, "Variable not declared", w_len, NULL);
            
            if (source[chk] == '=' && source[chk+1] != '=') {
                if (var_store != -1) error_exit(filename, source, start, "Missing ';' before assignment", w_len, NULL);
                var_store = idx; s_ptr = chk + 1;
            } 
            else if (source[chk] == '+' && source[chk+1] == '=') {
                out[b_ptr++] = OP_LOAD; out[b_ptr++] = (uint8_t)idx;
                p_add_sub = OP_ADD; var_store = idx; s_ptr = chk + 2;
            } else if (source[chk] == '-' && source[chk+1] == '=') {
                out[b_ptr++] = OP_LOAD; out[b_ptr++] = (uint8_t)idx;
                p_add_sub = OP_SUB; var_store = idx; s_ptr = chk + 2;
            } else if (source[chk] == '*' && source[chk+1] == '=') {
                out[b_ptr++] = OP_LOAD; out[b_ptr++] = (uint8_t)idx;
                p_mul_div = OP_MUL; var_store = idx; s_ptr = chk + 2;
            } 
            else {
                out[b_ptr++] = OP_LOAD; out[b_ptr++] = (uint8_t)idx;
                if (p_mul_div) { out[b_ptr++] = p_mul_div; p_mul_div = 0; }
            }
            continue;
        }

        if (c == '+' || c == '-') {
            if (p_add_sub) out[b_ptr++] = p_add_sub;
            p_add_sub = (c == '+') ? OP_ADD : OP_SUB;
            s_ptr++; continue;
        }
    if (c == '*' || c == '/' || c == '%') {
        if (p_mul_div) out[b_ptr++] = p_mul_div; 
        p_mul_div = (c == '*') ? OP_MUL : (c == '/' ? OP_DIV : OP_MOD);
        s_ptr++; 
        continue;
    }
        error_exit(filename, source, s_ptr, "Unexpected token", 1, NULL);
    }
    if (var_store != -1) error_exit(filename, source, s_ptr, "Expected ';' at end of file", 1, NULL);
    out[b_ptr++] = OP_HALT; 
    return b_ptr;
}