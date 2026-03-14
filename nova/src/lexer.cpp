#include "../include/lexer.h"
#include <cctype>
#include <sstream>

static std::string source;
static size_t pos = 0;
static int line = 1;
static int col  = 1;

void initLexer(const std::string& src) {
    source = src;
    pos  = 0;
    line = 1;
    col  = 1;
}

// Retorna uma linha específica do fonte (para mensagens de erro)
std::string getSourceLine(int lineNumber) {
    int cur = 1;
    std::string result;
    for (size_t i = 0; i < source.size(); i++) {
        if (cur == lineNumber) {
            if (source[i] == '\n') break;
            result += source[i];
        }
        if (source[i] == '\n') cur++;
    }
    return result;
}

static char peek() {
    if (pos >= source.size()) return '\0';
    return source[pos];
}

static char advance() {
    char c = source[pos++];
    if (c == '\n') { line++; col = 1; }
    else col++;
    return c;
}

static void skipWhiteSpace() {
    while (pos < source.size()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && pos + 1 < source.size() && source[pos+1] == '/') {
            while (peek() != '\n' && pos < source.size()) advance();
        } else {
            break;
        }
    }
}

static TokenType identifyKeyword(const std::string& word) {
    if (word == "int")    return TOKEN_INT;
    if (word == "float")  return TOKEN_FLOAT;
    if (word == "string") return TOKEN_STRING;
    if (word == "void")   return TOKEN_VOID;
    if (word == "if")     return TOKEN_IF;
    if (word == "else")   return TOKEN_ELSE;
    if (word == "then")   return TOKEN_THEN;
    if (word == "for")    return TOKEN_FOR;
    if (word == "while")  return TOKEN_WHILE;
    if (word == "return") return TOKEN_RETURN;
    if (word == "print")  return TOKEN_PRINT;
    if (word == "struct") return TOKEN_STRUCT;
    return TOKEN_IDENT;
}

Token nextToken() {
    skipWhiteSpace();

    if (pos >= source.size())
        return {TOKEN_EOF, "", line, col};

    char c = peek();
    int tokLine = line;
    int tokCol  = col;

    if (std::isdigit(c)) {
        std::string num;
        bool isFloat = false;
        while (std::isdigit(peek()) || peek() == '.') {
            if (peek() == '.') isFloat = true;
            num += advance();
        }
        return {isFloat ? TOKEN_FLOAT_LIT : TOKEN_INT_LIT, num, tokLine, tokCol};
    }

    if (std::isalpha(c) || c == '_') {
        std::string word;
        while (std::isalnum(peek()) || peek() == '_')
            word += advance();
        return {identifyKeyword(word), word, tokLine, tokCol};
    }

    if (c == '"') {
        advance();
        std::string str;
        while (peek() != '"' && pos < source.size())
            str += advance();
        advance();
        return {TOKEN_STRING_LIT, str, tokLine, tokCol};
    }

    advance();
    switch (c) {
        case '+': return {TOKEN_PLUS,   "+", tokLine, tokCol};
        case '-': return {TOKEN_MINUS,  "-", tokLine, tokCol};
        case '*': return {TOKEN_STAR,   "*", tokLine, tokCol};
        case '/': return {TOKEN_SLASH,  "/", tokLine, tokCol};
        case '%': return {TOKEN_PERCENT, "%", tokLine, tokCol};
        case '(': return {TOKEN_LPAREN, "(", tokLine, tokCol};
        case ')': return {TOKEN_RPAREN, ")", tokLine, tokCol};
        case '{': return {TOKEN_LBRACE, "{", tokLine, tokCol};
        case '}': return {TOKEN_RBRACE, "}", tokLine, tokCol};
        case '[': return {TOKEN_LBRACKET, "[", tokLine, tokCol};
        case ']': return {TOKEN_RBRACKET, "]", tokLine, tokCol};
        case ',': return {TOKEN_COMMA,  ",", tokLine, tokCol};
        case ';': return {TOKEN_SEMI,   ";", tokLine, tokCol};
        case '.': return {TOKEN_DOT,    ".", tokLine, tokCol};
        case '=':
            if (peek() == '=') { advance(); return {TOKEN_EQ,  "==", tokLine, tokCol}; }
            return {TOKEN_ASSIGN, "=", tokLine, tokCol};
        case '!':
            if (peek() == '=') { advance(); return {TOKEN_NEQ, "!=", tokLine, tokCol}; }
            return {TOKEN_NOT, "!", tokLine, tokCol};
        case '&':
            if (peek() == '&') { advance(); return {TOKEN_AND, "&&", tokLine, tokCol}; }
            break;
        case '|':
            if (peek() == '|') { advance(); return {TOKEN_OR,  "||", tokLine, tokCol}; }
            break;
        case '<':
            if (peek() == '=') { advance(); return {TOKEN_LEQ, "<=", tokLine, tokCol}; }
            return {TOKEN_LT, "<", tokLine, tokCol};
        case '>':
            if (peek() == '=') { advance(); return {TOKEN_GEQ, ">=", tokLine, tokCol}; }
            return {TOKEN_GT, ">", tokLine, tokCol};
    }

    return {TOKEN_UNKNOWN, std::string(1, c), tokLine, tokCol};
}

Token peekToken() {
    size_t savedPos  = pos;
    int    savedLine = line;
    int    savedCol  = col;
    Token t = nextToken();
    pos  = savedPos;
    line = savedLine;
    col  = savedCol;
    return t;
}