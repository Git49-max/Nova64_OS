#pragma once
#include <string>
#include <iostream>

// Códigos ANSI para cores
#define RED     "\033[1;31m"
#define BOLD    "\033[1m"
#define RESET   "\033[0m"

// Exibe erro estilo GCC/Clang e encerra o programa
// filename: nome do arquivo fonte
// line, col: posição do erro
// message: descrição do erro
// sourceLine: a linha de código onde ocorreu
// tokenLen: tamanho do token para os ^^^^
inline void reportError(const std::string& filename, int line, int col,
                        const std::string& message, const std::string& sourceLine,
                        int tokenLen = 1) {
    // Ex: test.nova:3:5: error: mensagem
    std::cerr << BOLD << filename << ":" << line << ":" << col << ": "
              << RED  << "error: " << RESET << BOLD << message << RESET << "\n";

    // Ex:   3 | if(a == b) then {
    std::cerr << "  " << line << " | " << sourceLine << "\n";

    // Ex:     |     ^^^^
    std::string padding(std::to_string(line).size() + 3, ' '); // alinha com o |
    std::cerr << padding << "| ";

    // Espaços até a coluna
    for (int i = 1; i < col; i++) std::cerr << ' ';

    // Os ^^^^ em vermelho
    std::cerr << RED;
    for (int i = 0; i < tokenLen; i++) std::cerr << '^';
    std::cerr << RESET << "\n";

    exit(1);
}