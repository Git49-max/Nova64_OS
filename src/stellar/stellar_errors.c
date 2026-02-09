#ifdef STELLAR_HOST

#include "stellar/stellar_errors.h"

// Função auxiliar interna para limpar binários corrompidos em caso de erro
static void get_bin_name(char* filename, char* out) {
    strcpy(out, filename);
    char* dot = strrchr(out, '.');
    if (dot) strcpy(dot, ".exe");
    else strcat(out, ".exe");
}

void error_exit(char* file, char* src, int ptr, char* msg, int len, char* suggest) {
    #ifdef STELLAR_HOST
    // Tenta remover o executável se ele foi criado com erro
    char trash[256]; 
    get_bin_name(file, trash); 
    remove(trash);

    int line = 1, l_start = 0;
    for (int i = 0; i < ptr; i++) {
        if (src[i] == '\n') { line++; l_start = i + 1; }
    }
    int col = ptr - l_start + 1;

    printf(BOLD "%s:%d:%d: " RED "error: " RESET BOLD "%s" RESET, file, line, col, msg);
    if (suggest) printf(". Did you mean " BOLD "\"%s\"" RESET "?", suggest);
    printf("\n");

    int l_end = ptr; 
    while (src[l_end] != '\n' && src[l_end] != '\0') l_end++;

    printf("%4d | %.*s\n", line, l_end - l_start, &src[l_start]);
    printf("     | ");
    for (int i = 0; i < (ptr - l_start); i++) printf(" ");
    
    printf(RED); 
    for (int i = 0; i < (len > 0 ? len : 1); i++) printf("^");
    printf(RESET "\n"); 
    
    exit(1);
    #else
    // No Nova64_OS, entra em halt ou loop infinito em caso de erro crítico de compilação
    while(1);
    #endif
}

// Implementação da sugestão "Did you mean"
static int is_similar(const char* s1, const char* s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int matrix[33][33]; // Limite de 32 caracteres para keywords
    
    if (len1 > 32 || len2 > 32) return 0;

    for (int i = 0; i <= len1; i++) matrix[i][0] = i;
    for (int j = 0; j <= len2; j++) matrix[0][j] = j;

    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            int a = matrix[i - 1][j] + 1;
            int b = matrix[i][j - 1] + 1;
            int c = matrix[i - 1][j - 1] + cost;
            matrix[i][j] = (a < b) ? (a < c ? a : c) : (b < c ? b : c);
        }
    }
    return matrix[len1][len2] <= 2; // Retorna verdadeiro se a diferença for pequena
}

#endif