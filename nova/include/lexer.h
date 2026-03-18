#pragma once
#include <string>

enum TokenType{
    TOKEN_INT_LIT,
    TOKEN_FLOAT_LIT,
    TOKEN_STRING_LIT,
    TOKEN_CHAR_LIT,   // 'a', '\n', '\t', etc.
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_CHAR,       // tipo char
    TOKEN_VOID,
    TOKEN_LONG,
    TOKEN_LONGLONG,
    TOKEN_DOUBLE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_THEN,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_RETURN,
    TOKEN_STRUCT,
    TOKEN_NAMESPACE,
    TOKEN_IDENT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_ASSIGN,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LEQ,
    TOKEN_GEQ,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_SEMI,
    TOKEN_COLON,
    TOKEN_COLONCOLON,   // ::
    TOKEN_DOT,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_HASH,
    TOKEN_INCLUDE,
    TOKEN_HEADER_PATH,
    TOKEN_ASM,
    TOKEN_IR,
    TOKEN_ELLIPSIS,   // ... para funções variádicas
    TOKEN_EOF,
    TOKEN_UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int col;   // ← coluna onde o token começa
};

// Estado completo do lexer — usado para salvar/restaurar ao parsear .nh
struct LexerState {
    std::string source;
    size_t pos;
    int line;
    int col;
};

void initLexer(const std::string& src);
LexerState saveLexerState();
void restoreLexerState(const LexerState& s);
Token nextToken();
Token peekToken();
std::string getSourceLine(int lineNumber); // ← retorna a linha do fonte para exibir no erro