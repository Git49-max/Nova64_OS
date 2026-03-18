#include "../include/lexer.h"
#include "../include/error.h"
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

LexerState saveLexerState() {
    return { source, pos, line, col };
}

void restoreLexerState(const LexerState& s) {
    source = s.source;
    pos    = s.pos;
    line   = s.line;
    col    = s.col;
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

// FIX #1: advance() agora é seguro — nunca lê além do fim do buffer.
static char advance() {
    if (pos >= source.size()) return '\0';
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
        } else if (c == '/' && pos + 1 < source.size() && source[pos+1] == '*'){
            while (peek() != '*' && source[pos+1] != '/' && pos < source.size()) advance();
        }
        else {
            break;
        }
    }
}


static TokenType identifyKeyword(const std::string& word) {
    if (word == "int")       return TOKEN_INT;
    if (word == "float")     return TOKEN_FLOAT;
    if (word == "string")    return TOKEN_STRING;
    if (word == "char")      return TOKEN_CHAR;
    if (word == "void")      return TOKEN_VOID;
    if (word == "long")      return TOKEN_LONG;
    if (word == "double")    return TOKEN_DOUBLE;
    if (word == "if")        return TOKEN_IF;
    if (word == "else")      return TOKEN_ELSE;
    if (word == "then")      return TOKEN_THEN;
    if (word == "for")       return TOKEN_FOR;
    if (word == "while")     return TOKEN_WHILE;
    if (word == "return")    return TOKEN_RETURN;
    if (word == "struct")    return TOKEN_STRUCT;
    if (word == "namespace") return TOKEN_NAMESPACE;
    if (word == "include")   return TOKEN_INCLUDE;
    if (word == "asm")       return TOKEN_ASM;
    if (word == "ir")        return TOKEN_IR;
    return TOKEN_IDENT;
}

Token nextToken() {
    skipWhiteSpace();

    if (pos >= source.size())
        return {TOKEN_EOF, "", line, col};

    // FIX #2: c é lido via peek() — signed, mas os isdigit/isalpha abaixo
    // recebem sempre o cast correto para (unsigned char).
    char c = peek();
    int tokLine = line;
    int tokCol  = col;

    // FIX #2 aplicado: todos os is*() recebem (unsigned char) para evitar UB
    // com caracteres cujo valor de byte é > 127.
    if (std::isdigit((unsigned char)c)) {
        std::string num;
        bool isFloat  = false;
        bool dotSeen  = false;   // FIX #3: impede "3.14.15" ser aceito como float válido

        while (pos < source.size()) {
            char ch = peek();
            if (std::isdigit((unsigned char)ch)) {
                num += advance();
            } else if (ch == '.' && !dotSeen) {
                // Só aceita o primeiro ponto
                dotSeen  = true;
                isFloat  = true;
                num += advance();
            } else {
                break;
            }
        }
        return {isFloat ? TOKEN_FLOAT_LIT : TOKEN_INT_LIT, num, tokLine, tokCol};
    }

    if (std::isalpha((unsigned char)c) || c == '_') {
        std::string word;
        while (pos < source.size() &&
               (std::isalnum((unsigned char)peek()) || peek() == '_'))
            word += advance();
        return {identifyKeyword(word), word, tokLine, tokCol};
    }

    if (c == '"') {
        advance();  // consome '"' de abertura
        std::string str;
        while (pos < source.size() && peek() != '"') {
            char sc = advance();
            if (sc == '\\' && pos < source.size()) {
                char esc = advance();
                switch (esc) {
                    case 'n':  str += '\n'; break;
                    case 't':  str += '\t'; break;
                    case 'r':  str += '\r'; break;
                    case '0':  str += '\0'; break;
                    case '\\': str += '\\'; break;
                    case '"':  str += '"';  break;
                    case '\'': str += '\''; break;
                    default:   str += '\\'; str += esc; break;
                }
            } else {
                str += sc;
            }
        }
        // FIX #4: string sem fechar → erro claro em vez de advance() fora dos limites
        if (pos >= source.size()) {
            std::string srcLine = getSourceLine(tokLine);
            reportError("<source>", tokLine, tokCol,
                        "unterminated string literal",
                        srcLine, 1);
        }
        advance();  // consome '"' de fechamento
        return {TOKEN_STRING_LIT, str, tokLine, tokCol};
    }

    // Char literal: 'a', '\n', '\t', '\0', etc.
    if (c == '\'') {
        advance();  // consome '\'' de abertura
        char ch = '\0';
        if (peek() == '\\') {
            advance();  // consome backslash
            char esc = advance();
            switch (esc) {
                case 'n':  ch = '\n'; break;
                case 't':  ch = '\t'; break;
                case 'r':  ch = '\r'; break;
                case '0':  ch = '\0'; break;
                case '\\': ch = '\\'; break;
                case '\'': ch = '\''; break;
                case '"':  ch = '"';  break;
                default:   ch = esc;  break;
            }
        } else {
            ch = advance();
        }
        if (peek() == '\'') advance();  // consome '\'' de fechamento
        return {TOKEN_CHAR_LIT, std::string(1, ch), tokLine, tokCol};
    }

    advance();  // consome o caractere atual antes do switch
    switch (c) {
        case '+': return {TOKEN_PLUS,    "+", tokLine, tokCol};
        case '-': return {TOKEN_MINUS,   "-", tokLine, tokCol};
        case '*': return {TOKEN_STAR,    "*", tokLine, tokCol};
        case '/': return {TOKEN_SLASH,   "/", tokLine, tokCol};
        case '%': return {TOKEN_PERCENT, "%", tokLine, tokCol};
        case '(': return {TOKEN_LPAREN,  "(", tokLine, tokCol};
        case ')': return {TOKEN_RPAREN,  ")", tokLine, tokCol};
        case '{': return {TOKEN_LBRACE,  "{", tokLine, tokCol};
        case '}': return {TOKEN_RBRACE,  "}", tokLine, tokCol};
        case '[': return {TOKEN_LBRACKET,"[", tokLine, tokCol};
        case ']': return {TOKEN_RBRACKET,"]", tokLine, tokCol};
        case ',': return {TOKEN_COMMA,   ",", tokLine, tokCol};
        case ';': return {TOKEN_SEMI,    ";", tokLine, tokCol};
        case ':':
            if (peek() == ':') { advance(); return {TOKEN_COLONCOLON, "::", tokLine, tokCol}; }
            return {TOKEN_COLON, ":", tokLine, tokCol};
        case '.':
            // Checa '...' (ellipsis para variádicos)
            if (peek() == '.' && pos + 1 < source.size() && source[pos+1] == '.') {
                advance(); advance();
                return {TOKEN_ELLIPSIS, "...", tokLine, tokCol};
            }
            return {TOKEN_DOT, ".", tokLine, tokCol};
        case '#': return {TOKEN_HASH, "#", tokLine, tokCol};
        case '=':
            if (peek() == '=') { advance(); return {TOKEN_EQ, "==", tokLine, tokCol}; }
            return {TOKEN_ASSIGN, "=", tokLine, tokCol};
        case '!':
            if (peek() == '=') { advance(); return {TOKEN_NEQ, "!=", tokLine, tokCol}; }
            return {TOKEN_NOT, "!", tokLine, tokCol};

        // FIX #5: '&' e '|' sozinhos emitem erro explícito em vez de TOKEN_UNKNOWN silencioso
        case '&':
            if (peek() == '&') { advance(); return {TOKEN_AND, "&&", tokLine, tokCol}; }
            {
                std::string srcLine = getSourceLine(tokLine);
                reportError("<source>", tokLine, tokCol,
                    "bitwise operator '&' is not supported — did you mean '&&'?",
                    srcLine, 1);
            }
            break;
        case '|':
            if (peek() == '|') { advance(); return {TOKEN_OR, "||", tokLine, tokCol}; }
            {
                std::string srcLine = getSourceLine(tokLine);
                reportError("<source>", tokLine, tokCol,
                    "bitwise operator '|' is not supported — did you mean '||'?",
                    srcLine, 1);
            }
            break;

        case '<': {
            // Tenta ler <header.nh> — caminho de include do sistema
            size_t savePos2  = pos;
            int    saveLine2 = line;
            int    saveCol2  = col;
            std::string hdr;
            while (peek() != '>' && peek() != '\n' && pos < source.size())
                hdr += advance();
            if (peek() == '>' && !hdr.empty() &&
                (std::isalpha((unsigned char)hdr[0]) || hdr[0] == '_' || hdr[0] == '.')) {
                advance();  // consome '>'
                return {TOKEN_HEADER_PATH, hdr, tokLine, tokCol};
            }
            // Não era header path — restaura e emite TOKEN_LT / TOKEN_LEQ
            pos  = savePos2;
            line = saveLine2;
            col  = saveCol2;
            if (peek() == '=') { advance(); return {TOKEN_LEQ, "<=", tokLine, tokCol}; }
            return {TOKEN_LT, "<", tokLine, tokCol};
        }
        case '>':
            if (peek() == '=') { advance(); return {TOKEN_GEQ, ">=", tokLine, tokCol}; }
            return {TOKEN_GT, ">", tokLine, tokCol};
    }

    return {TOKEN_UNKNOWN, std::string(1, c), tokLine, tokCol};
}

// FIX #7: peekToken usa saveLexerState/restoreLexerState em vez de salvar
// só pos/line/col, garantindo que qualquer mudança futura em 'source' também
// seja corretamente revertida.
Token peekToken() {
    LexerState saved = saveLexerState();
    Token t = nextToken();
    restoreLexerState(saved);
    return t;
}