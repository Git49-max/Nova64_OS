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

typedef enum { TY_DOUBLE, TY_INT } VarType;

// --- SYMBOL TABLE ---
typedef struct {
    char name[32];
    int id;
    VarType type; 
} Symbol;

static Symbol symbol_table[256];
static int symbol_count = 0;

static int find_variable(char* name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) return symbol_table[i].id;
    }
    return -1;
}

static int add_variable(char* name, VarType type) {
    if (symbol_count >= 256) return -1;
    strncpy(symbol_table[symbol_count].name, name, 31);
    symbol_table[symbol_count].id = symbol_count;
    symbol_table[symbol_count].type = type; 
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
            
            if (var_store == -2) {
                // Se era um a[i] = valor, agora o valor já está na stack
                out[b_ptr++] = OP_ARRAY_SET;
                var_store = -1;
            } else if (var_store != -1) {
                    out[b_ptr++] = (symbol_table[var_store].type == TY_INT) ? OP_STORE_I : OP_STORE;
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

        if (c == '"') {
            s_ptr++;
            int start_str = s_ptr;
            while (source[s_ptr] != '"' && source[s_ptr] != '\0') s_ptr++;
            int str_len = s_ptr - start_str;

            // 1. Emite um JMP para pular o texto (3 bytes: Opcode + Addr16)
            out[b_ptr++] = OP_JMP;
            int jmp_patch = b_ptr; 
            out[b_ptr++] = 0; out[b_ptr++] = 0;

            // 2. Aqui começa a string real
            uint16_t str_addr = (uint16_t)b_ptr;
            for (int i = 0; i < str_len; i++) out[b_ptr++] = source[start_str + i];
            out[b_ptr++] = '\0';

            // 3. Patch do JMP: Agora o JMP aponta para DEPOIS do \0
            patch16(out, jmp_patch, (uint16_t)b_ptr);

            // 4. Agora sim, empilha o endereço que a gente guardou
            out[b_ptr++] = OP_PUSH; // Usamos o PUSH normal de 8 bytes (double)
            double addr_val = (double)str_addr;
            for (int i = 0; i < 8; i++) out[b_ptr++] = ((uint8_t*)&addr_val)[i];

            if (expect_print) {
                expect_print = 2; // Sinaliza PRINT_STR
            }
            s_ptr++; continue;
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

       // --- DECLARAÇÃO: let (double) ou int (inteiro) ---
        int is_int = -1;
        if ((len = match(source, s_ptr, "let"))) { is_int = 0; }
        else if ((len = match(source, s_ptr, "int"))) { is_int = 1; }

        if (is_int != -1) {
            if (var_store != -1) error_exit(filename, source, s_ptr, "Missing ';' before declaration", len, NULL);
            s_ptr += len; 
            while (isspace(source[s_ptr])) s_ptr++;
            
            char var_name[32] = {0}; int p = 0;
            while (isalnum(source[s_ptr]) || source[s_ptr] == '_') {
                if(p < 31) var_name[p++] = source[s_ptr++]; else s_ptr++;
            }
            
            int current_var_idx = find_variable(var_name);
            if (current_var_idx == -1) {
                // Adiciona a variável passando o tipo: TY_INT ou TY_DOUBLE
                current_var_idx = add_variable(var_name, is_int ? TY_INT : TY_DOUBLE);
                if (current_var_idx == -1) 
                    error_exit(filename, source, s_ptr, "Too many variables (limit 256)", 1, NULL);
            }
            
            while (isspace(source[s_ptr])) s_ptr++;
            
            if (source[s_ptr] == '=') {
                s_ptr++; // Pula o '='
                while (isspace(source[s_ptr])) s_ptr++;

                if (source[s_ptr] == '[') {
                    s_ptr++; // Pula o '['
                    while (isspace(source[s_ptr])) s_ptr++;

                    // Lendo o tamanho do array
                    if (isdigit(source[s_ptr])) {
                        char num_buf[16] = {0}; int np = 0;
                        while(isdigit(source[s_ptr]) || source[s_ptr] == '.') num_buf[np++] = source[s_ptr++];
                        double size = atof(num_buf);
                        out[b_ptr++] = OP_PUSH;
                        for (int i = 0; i < 8; i++) out[b_ptr++] = ((uint8_t*)&size)[i];
                    } else {
                        error_exit(filename, source, s_ptr, "Array size must be a number for now", 1, NULL);
                    }

                    while (isspace(source[s_ptr])) s_ptr++;
                    if (source[s_ptr] != ']') error_exit(filename, source, s_ptr, "Expected ']'", 1, NULL);
                    s_ptr++;

                    out[b_ptr++] = OP_ARRAY_MAKE;
                    
                    // Arrays sempre usam o STORE de double por enquanto para guardar o ponteiro
                    out[b_ptr++] = OP_STORE;
                    out[b_ptr++] = (uint8_t)current_var_idx;
                    
                    force_semicolon_check = 1;
                } else {
                    // Atribuição normal (o valor será armazenado no ponto e vírgula)
                    var_store = current_var_idx;
                }
            }
            continue;
        }

        if ((len = match(source, s_ptr, "print"))) { 
            if (var_store != -1) error_exit(filename, source, s_ptr, "Missing ';' before 'print'", len, NULL);
            expect_print = 1; s_ptr += len; continue; 
        }
        // --- SUPORTE A INPUT ---
        if ((len = match(source, s_ptr, "input"))) {
            s_ptr += len;
            while (isspace(source[s_ptr])) s_ptr++;
            
            if (source[s_ptr] != '(') error_exit(filename, source, s_ptr, "Expected '(' after input", 1, NULL);
            s_ptr++;
            while (isspace(source[s_ptr])) s_ptr++;

            // Captura o nome da variável alvo
            char var_name[32] = {0}; int p = 0;
            while (isalnum(source[s_ptr]) || source[s_ptr] == '_') {
                if(p < 31) var_name[p++] = source[s_ptr++]; else s_ptr++;
            }

            int idx = find_variable(var_name);
            if (idx == -1) error_exit(filename, source, s_ptr, "Variable not declared for input", p, NULL);

            

            while (isspace(source[s_ptr])) s_ptr++;
            if (source[s_ptr] != ')') error_exit(filename, source, s_ptr, "Expected ')' after variable name", 1, NULL);
            s_ptr++;

            // Geração do Bytecode
            out[b_ptr++] = OP_INPUT;  // Coloca o valor lido na stack
            out[b_ptr++] = OP_STORE;  // Tira da stack e salva na variável
            out[b_ptr++] = (uint8_t)idx;

            force_semicolon_check = 1;
            continue;
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
            if (expect_print) {
                if (expect_print == 2) {
                    out[b_ptr++] = OP_PRINT_STR;
                } else {
                    out[b_ptr++] = OP_PRINT;
                }
                expect_print = 0; 
                force_semicolon_check = 1; 
            }
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
                const char* valid_functions[] = { "print", "input", NULL };
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

            if (source[chk] == '+' && source[chk+1] == '+') {
                out[b_ptr++] = (symbol_table[idx].type == TY_INT) ? OP_INC_I : OP_INC;
                out[b_ptr++] = (uint8_t)idx;
                s_ptr = chk + 2; // Pula os dois '+'
                // Precisamos garantir que o usuário coloque ';' depois, ou tratar como instrução finalizada
                force_semicolon_check = 1; 
                continue;
            }

            if (source[chk] == '-' && source[chk+1] == '-') {
                out[b_ptr++] = (symbol_table[idx].type == TY_INT) ? OP_DEC_I : OP_DEC;
                out[b_ptr++] = (uint8_t)idx;
                s_ptr = chk + 2; 
                force_semicolon_check = 1; 
                continue;
            }
            
            // --- TRATAMENTO DE ARRAYS ---
            if (source[chk] == '[') {
            s_ptr = chk + 1;
            while (isspace(source[s_ptr])) s_ptr++;

            // 1. Carrega o endereço base (ponteiro) do array
            // Arrays são sempre armazenados como ponteiros (double/uintptr_t)
            out[b_ptr++] = OP_LOAD;
            out[b_ptr++] = (uint8_t)idx;

            // 2. Compila o índice (Número ou outra Variável)
            if (isdigit(source[s_ptr])) {
                char num_buf[16] = {0}; int np = 0;
                while(isdigit(source[s_ptr]) || source[s_ptr] == '.') num_buf[np++] = source[s_ptr++];
                double idx_val = atof(num_buf);
                out[b_ptr++] = OP_PUSH;
                for (int i = 0; i < 8; i++) out[b_ptr++] = ((uint8_t*)&idx_val)[i];
            } else if (isalpha(source[s_ptr])) {
                char idx_var_name[32] = {0}; int p = 0;
                while (isalnum(source[s_ptr]) || source[s_ptr] == '_') idx_var_name[p++] = source[s_ptr++];
                int idx_var = find_variable(idx_var_name);
                if (idx_var == -1) error_exit(filename, source, s_ptr, "Index variable not declared", p, NULL);
                
                // --- CORREÇÃO AQUI ---
                // Se o índice for uma variável 'int', usamos LOAD_I para converter para double na stack
                out[b_ptr++] = (symbol_table[idx_var].type == TY_INT) ? OP_LOAD_I : OP_LOAD;
                out[b_ptr++] = (uint8_t)idx_var;
            }

            while (isspace(source[s_ptr])) s_ptr++;
            if (source[s_ptr] != ']') error_exit(filename, source, s_ptr, "Expected ']'", 1, NULL);
            s_ptr++;
            
            while (isspace(source[s_ptr])) s_ptr++;

            // 3. Decidir se é Leitura (GET) ou Escrita (SET)
            if (source[s_ptr] == '=' && source[s_ptr+1] != '=') {
                // Atribuição: a[i] = valor;
                s_ptr++; // pula '='
                var_store = -2; // Sinalizador para o ';' emitir OP_ARRAY_SET
                // O valor a ser guardado será compilado nas próximas iterações do loop principal
            } else {
                // Leitura: x = a[i]; ou print a[i];
                out[b_ptr++] = OP_ARRAY_GET;
                
                // Se houver operação pendente (ex: a[i] * 2), aplica agora
                if (p_mul_div) { 
                    out[b_ptr++] = p_mul_div; 
                    p_mul_div = 0; 
                }
                if (p_add_sub) {
                    out[b_ptr++] = p_add_sub;
                    p_add_sub = 0;
                }
            }
            continue;
        } 
            // --- ATRIBUIÇÕES COMPOSTAS E SIMPLES ---
            else if (source[chk] == '=' && source[chk+1] != '=') {
                if (var_store != -1) error_exit(filename, source, start, "Missing ';' before assignment", w_len, NULL);
                var_store = idx; s_ptr = chk + 1;
            } 
            else if (source[chk] == '+' && source[chk+1] == '=') {
                s_ptr = chk + 2;
                while (isspace(source[s_ptr])) s_ptr++;

                // Se o que vem depois for um número (ex: j += 2)
                if (isdigit(source[s_ptr])) {
                    char val_str[32];
                    int v_ptr = 0;
                    while (isdigit(source[s_ptr]) || source[s_ptr] == '.') {
                        val_str[v_ptr++] = source[s_ptr++];
                    }
                    val_str[v_ptr] = '\0';
                    double val = atof(val_str);

                    if (symbol_table[idx].type == TY_INT) {
                        out[b_ptr++] = OP_ADD_CONST_I;
                        out[b_ptr++] = (uint8_t)idx;
                        long ival = (long)val; // Converte para long para a VM ler como .i
                        for(int i=0; i<8; i++) out[b_ptr++] = ((uint8_t*)&ival)[i];
                    } else {
                        out[b_ptr++] = OP_ADD_CONST;
                        out[b_ptr++] = (uint8_t)idx;
                        for(int i=0; i<8; i++) out[b_ptr++] = ((uint8_t*)&val)[i];
                    }
                    
                    force_semicolon_check = 1;
                    continue;
                }

                // Verifica se é uma variável simples: j += i;
                if (isalpha(source[s_ptr])) {
                    int start_src = s_ptr;
                    while (isalnum(source[s_ptr]) || source[s_ptr] == '_') s_ptr++;
                    int src_len = s_ptr - start_src;
                    
                    char src_name[32] = {0};
                    strncpy(src_name, &source[start_src], src_len);
                    int src_idx = find_variable(src_name);

                    if (src_idx != -1) {
                        if (symbol_table[idx].type == TY_INT && symbol_table[src_idx].type == TY_INT) {
                            out[b_ptr++] = OP_ADD_VAR_I;
                        } else {
                            out[b_ptr++] = OP_ADD_VAR;
                        }
                        out[b_ptr++] = (uint8_t)idx;     // Destino (j)
                        out[b_ptr++] = (uint8_t)src_idx; // Origem (i)
                        force_semicolon_check = 1;
                        continue;
                    }
                    s_ptr = start_src; 
                }
                
                // Modo padrão (Fallback para stack se não for otimizável)
                out[b_ptr++] = (symbol_table[idx].type == TY_INT) ? OP_LOAD_I : OP_LOAD;
                out[b_ptr++] = (uint8_t)idx;
                p_add_sub = OP_ADD; var_store = idx;
            } 
            else if (source[chk] == '-' && source[chk+1] == '=') {
                s_ptr = chk + 2;
                while (isspace(source[s_ptr])) s_ptr++;

                if (isdigit(source[s_ptr])) {
                    char val_str[32];
                    int v_ptr = 0;
                    while (isdigit(source[s_ptr]) || source[s_ptr] == '.') val_str[v_ptr++] = source[s_ptr++];
                    val_str[v_ptr] = '\0';
                    double val = atof(val_str);

                    if (symbol_table[idx].type == TY_INT) {
                        out[b_ptr++] = OP_SUB_CONST_I;
                        out[b_ptr++] = (uint8_t)idx;
                        long ival = (long)val;
                        for(int i=0; i<8; i++) out[b_ptr++] = ((uint8_t*)&ival)[i];
                    } else {
                        out[b_ptr++] = OP_SUB_CONST;
                        out[b_ptr++] = (uint8_t)idx;
                        for(int i=0; i<8; i++) out[b_ptr++] = ((uint8_t*)&val)[i];
                    }
                    force_semicolon_check = 1;
                    continue;
                }

                if (isalpha(source[s_ptr])) {
                    int start_src = s_ptr;
                    while (isalnum(source[s_ptr]) || source[s_ptr] == '_') s_ptr++;
                    int src_len = s_ptr - start_src;
                    char src_name[32] = {0};
                    strncpy(src_name, &source[start_src], src_len);
                    int src_idx = find_variable(src_name);

                    if (src_idx != -1) {
                        if (symbol_table[idx].type == TY_INT && symbol_table[src_idx].type == TY_INT) {
                            out[b_ptr++] = OP_SUB_VAR_I;
                        } else {
                            out[b_ptr++] = OP_SUB_VAR;
                        }
                        out[b_ptr++] = (uint8_t)idx;
                        out[b_ptr++] = (uint8_t)src_idx;
                        force_semicolon_check = 1;
                        continue;
                    }
                    s_ptr = start_src; 
                }
                
                out[b_ptr++] = (symbol_table[idx].type == TY_INT) ? OP_LOAD_I : OP_LOAD;
                out[b_ptr++] = (uint8_t)idx;
                p_add_sub = OP_SUB; var_store = idx;
            } 
            else if (source[chk] == '*' && source[chk+1] == '=') {
                s_ptr = chk + 2;
                while (isspace(source[s_ptr])) s_ptr++;

                if (isdigit(source[s_ptr])) {
                    char val_str[32];
                    int v_ptr = 0;
                    while (isdigit(source[s_ptr]) || source[s_ptr] == '.') val_str[v_ptr++] = source[s_ptr++];
                    val_str[v_ptr] = '\0';
                    double val = atof(val_str);

                    if (symbol_table[idx].type == TY_INT) {
                        out[b_ptr++] = OP_MUL_CONST_I;
                        out[b_ptr++] = (uint8_t)idx;
                        long ival = (long)val;
                        for(int i=0; i<8; i++) out[b_ptr++] = ((uint8_t*)&ival)[i];
                    } else {
                        out[b_ptr++] = OP_MUL_CONST;
                        out[b_ptr++] = (uint8_t)idx;
                        for(int i=0; i<8; i++) out[b_ptr++] = ((uint8_t*)&val)[i];
                    }
                    force_semicolon_check = 1;
                    continue;
                }

                if (isalpha(source[s_ptr])) {
                    int start_src = s_ptr;
                    while (isalnum(source[s_ptr]) || source[s_ptr] == '_') s_ptr++;
                    int src_len = s_ptr - start_src;
                    char src_name[32] = {0};
                    strncpy(src_name, &source[start_src], src_len);
                    int src_idx = find_variable(src_name);

                    if (src_idx != -1) {
                        if (symbol_table[idx].type == TY_INT && symbol_table[src_idx].type == TY_INT) {
                            out[b_ptr++] = OP_MUL_VAR_I;
                        } else {
                            out[b_ptr++] = OP_MUL_VAR;
                        }
                        out[b_ptr++] = (uint8_t)idx;
                        out[b_ptr++] = (uint8_t)src_idx;
                        force_semicolon_check = 1;
                        continue;
                    }
                    s_ptr = start_src; 
                }
                
                out[b_ptr++] = (symbol_table[idx].type == TY_INT) ? OP_LOAD_I : OP_LOAD;
                out[b_ptr++] = (uint8_t)idx;
                p_mul_div = OP_MUL; var_store = idx;
            }
            // --- VARIÁVEL SIMPLES (LEITURA) ---
            else {
                // 1. Carrega a variável com o tipo correto
                out[b_ptr++] = (symbol_table[idx].type == TY_INT) ? OP_LOAD_I : OP_LOAD;
                out[b_ptr++] = (uint8_t)idx;
                
                // 2. Se havia uma multiplicação/divisão pendente ANTES desta variável, aplica AGORA
                if (p_mul_div) { 
                    out[b_ptr++] = p_mul_div; 
                    p_mul_div = 0; 
                }
                
                s_ptr = chk;
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