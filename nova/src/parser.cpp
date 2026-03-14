#include "../include/ast.h"
#include "../include/lexer.h"
#include "../include/error.h"
#include <iostream>
#include <map>

static Token current;
static std::string sourceFile;

// Mapa de tamanho de arrays declarados: nome -> tamanho
// Usado para bound checking em tempo de compilação
static std::map<std::string, int> arraySizes;

static std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TOKEN_SEMI:     return "';'";
        case TOKEN_LPAREN:   return "'('";
        case TOKEN_RPAREN:   return "')'";
        case TOKEN_LBRACE:   return "'{'";
        case TOKEN_RBRACE:   return "'}'";
        case TOKEN_LBRACKET: return "'['";
        case TOKEN_RBRACKET: return "']'";
        case TOKEN_ASSIGN:   return "'='";
        case TOKEN_COMMA:    return "','";
        case TOKEN_IDENT:    return "identificador";
        case TOKEN_INT_LIT:  return "literal inteiro";
        case TOKEN_FLOAT_LIT:return "literal float";
        case TOKEN_STRING_LIT:return "literal string";
        case TOKEN_INT:      return "'int'";
        case TOKEN_FLOAT:    return "'float'";
        case TOKEN_STRING:   return "'string'";
        case TOKEN_VOID:     return "'void'";
        case TOKEN_IF:       return "'if'";
        case TOKEN_ELSE:     return "'else'";
        case TOKEN_THEN:     return "'then'";
        case TOKEN_WHILE:    return "'while'";
        case TOKEN_FOR:      return "'for'";
        case TOKEN_RETURN:   return "'return'";
        case TOKEN_PRINT:    return "'print'";
        case TOKEN_STRUCT:   return "'struct'";
        case TOKEN_EOF:      return "fim de arquivo";
        default:             return "token desconhecido";
    }
}

static Token eat(TokenType expected) {
    if (current.type != expected) {
        std::string line = getSourceLine(current.line);
        if(tokenTypeName(expected) == "';'"){
            std::string msg = "expected ';' after previous instruction";
            reportError(sourceFile, current.line, current.col,
                    msg, line, (int)current.value.size());
        } else {
        std::string msg = "expected " + tokenTypeName(expected) +
                          ", but found '" + current.value + "'";
        reportError(sourceFile, current.line, current.col,
                    msg, line, (int)current.value.size());
        }
    }
    Token t = current;
    current = nextToken();
    return t;
}

static DataType parseDataType() {
    if (current.type == TOKEN_INT)    { current = nextToken(); return DataType::Int; }
    if (current.type == TOKEN_FLOAT)  { current = nextToken(); return DataType::Float; }
    if (current.type == TOKEN_STRING) { current = nextToken(); return DataType::String; }
    if (current.type == TOKEN_VOID)   { current = nextToken(); return DataType::Void; }
    std::string line = getSourceLine(current.line);
    reportError(sourceFile, current.line, current.col,
                "invalid type '" + current.value + "': expected 'int', 'float', 'string', or 'void'",
                line, (int)current.value.size());
    return DataType::Int;
}

static NodePtr parseStatement();
static std::vector<NodePtr> parseBlock();
static NodePtr parseExpr();
static NodePtr parseAddSub();

static NodePtr parsePrimary() {
    // ── Operador unário ! ────────────────────────────────────────────
    if (current.type == TOKEN_NOT) {
        int tl = current.line, tc = current.col;
        current = nextToken();
        auto operand = parsePrimary();
        return std::make_unique<UnaryOpNode>("!", std::move(operand), tl, tc);
    }
    if (current.type == TOKEN_INT_LIT) {
        int v = std::stoi(current.value);
        current = nextToken();
        return std::make_unique<IntLitNode>(v);
    }
    if (current.type == TOKEN_FLOAT_LIT) {
        float v = std::stof(current.value);
        current = nextToken();
        return std::make_unique<FloatLitNode>(v);
    }
    if (current.type == TOKEN_STRING_LIT) {
        std::string v = current.value;
        current = nextToken();
        return std::make_unique<StringLitNode>(v);
    }
    if (current.type == TOKEN_IDENT) {
        std::string name = current.value;
        int tokLine = current.line;
        int tokCol  = current.col;
        current = nextToken();
        if (current.type == TOKEN_LPAREN) {
            current = nextToken();
            std::vector<NodePtr> args;
            while (current.type != TOKEN_RPAREN) {
                args.push_back(parseExpr());
                if (current.type == TOKEN_COMMA)
                    current = nextToken();
            }
            eat(TOKEN_RPAREN);
            return std::make_unique<CallNode>(name, std::move(args), tokLine, tokCol);
        }
        if (current.type == TOKEN_LBRACKET) {
            current = nextToken();
            int idxLine = current.line, idxCol = current.col;
            auto index = parseExpr();
            eat(TOKEN_RBRACKET);
            // Bound check em tempo de compilação (só para literais inteiros)
            if (auto* lit = dynamic_cast<IntLitNode*>(index.get())) {
                auto it = arraySizes.find(name);
                if (it != arraySizes.end()) {
                    int idx = lit->value;
                    if (idx < 0 || idx >= it->second) {
                        std::string ln = getSourceLine(idxLine);
                        reportError(sourceFile, idxLine, idxCol,
                            "index " + std::to_string(idx) + " is out of bounds for array '" +
                            name + "' of size " + std::to_string(it->second) +
                            " (valid range: 0.." + std::to_string(it->second - 1) + ")",
                            ln, (int)std::to_string(idx).size());
                    }
                }
            }
            return std::make_unique<ArrayAccessNode>(name, std::move(index), tokLine, tokCol);
        }
        if (current.type == TOKEN_DOT) {
            current = nextToken();
            std::string member = eat(TOKEN_IDENT).value;
            // p.metodo(args) — method call
            if (current.type == TOKEN_LPAREN) {
                current = nextToken();
                std::vector<NodePtr> args;
                while (current.type != TOKEN_RPAREN) {
                    args.push_back(parseExpr());
                    if (current.type == TOKEN_COMMA) current = nextToken();
                }
                eat(TOKEN_RPAREN);
                return std::make_unique<MethodCallNode>(name, member, std::move(args), tokLine, tokCol);
            }
            // p.campo — field access
            return std::make_unique<FieldAccessNode>(name, member, tokLine, tokCol);
        }
        return std::make_unique<VarNode>(name, tokLine, tokCol);
    }
    if (current.type == TOKEN_LPAREN) {
        current = nextToken();
        auto expr = parseExpr();
        eat(TOKEN_RPAREN);
        return expr;
    }
    std::string line = getSourceLine(current.line);
    reportError(sourceFile, current.line, current.col,
                "invalid expression: '" + current.value + "' cannot start an expression.",
                line, (int)current.value.size());
    return nullptr;
}

static NodePtr parseTerm() {
    auto left = parsePrimary();
    while (current.type == TOKEN_STAR || current.type == TOKEN_SLASH || current.type == TOKEN_PERCENT) {
        std::string op = current.value;
        int tl = current.line, tc = current.col;
        current = nextToken();
        auto right = parsePrimary();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), tl, tc);
    }
    return left;
}

static NodePtr parseAddSub() {
    auto left = parseTerm();
    while (current.type == TOKEN_PLUS || current.type == TOKEN_MINUS) {
        std::string op = current.value;
        int tl = current.line, tc = current.col;
        current = nextToken();
        auto right = parseTerm();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), tl, tc);
    }
    return left;
}

static NodePtr parseExpr();

// Hierarquia de precedência (baixo → alto):
// || → && → comparação → + - → * / → unário → primary

static NodePtr parseComparison() {
    auto left = parseAddSub();
    while (current.type == TOKEN_EQ  || current.type == TOKEN_NEQ ||
           current.type == TOKEN_LT  || current.type == TOKEN_GT  ||
           current.type == TOKEN_LEQ || current.type == TOKEN_GEQ) {
        std::string op = current.value;
        int tl = current.line, tc = current.col;
        current = nextToken();
        auto right = parseAddSub();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right), tl, tc);
    }
    return left;
}

static NodePtr parseAnd() {
    auto left = parseComparison();
    while (current.type == TOKEN_AND) {
        int tl = current.line, tc = current.col;
        current = nextToken();
        auto right = parseComparison();
        left = std::make_unique<BinaryOpNode>("&&", std::move(left), std::move(right), tl, tc);
    }
    return left;
}

static NodePtr parseExpr() {
    auto left = parseAnd();
    while (current.type == TOKEN_OR) {
        int tl = current.line, tc = current.col;
        current = nextToken();
        auto right = parseAnd();
        left = std::make_unique<BinaryOpNode>("||", std::move(left), std::move(right), tl, tc);
    }
    return left;
}

static std::vector<NodePtr> parseBlock() {
    eat(TOKEN_LBRACE);
    std::vector<NodePtr> stmts;
    while (current.type != TOKEN_RBRACE && current.type != TOKEN_EOF)
        stmts.push_back(parseStatement());
    eat(TOKEN_RBRACE);
    return stmts;
}

static NodePtr parseStatement() {
    // ── struct NomeTipo varNome; dentro de funções ───────────────────────
    if (current.type == TOKEN_STRUCT) {
        current = nextToken(); // consome 'struct' (opcional na declaração local)
    }
    // Detecta NomeTipo varNome; onde NomeTipo é um IDENT (tipo de struct)
    // Isso é ambíguo com expressão IDENT, mas só chega aqui se o próximo for outro IDENT
    if (current.type == TOKEN_IDENT) {
        Token saved = current;
        int tokLine = current.line, tokCol = current.col;
        std::string firstName = current.value;
        current = nextToken();
        if (current.type == TOKEN_IDENT) {
            // NomeTipo varNome;
            std::string varName = current.value;
            current = nextToken();
            eat(TOKEN_SEMI);
            return std::make_unique<StructVarDeclNode>(firstName, varName, tokLine, tokCol);
        }
        // Não é declaração de struct — tratar como IDENT normal (atribuição/chamada)
        // Reprocessar: empurrar de volta reconstruindo o fluxo
        // Como não temos unget, vamos duplicar o bloco de TOKEN_IDENT abaixo inline
        {
            std::string name = firstName;
            // current já avançou — simular o que parseStatement faria após ler o IDENT
            if (current.type == TOKEN_LBRACKET) {
                current = nextToken();
                int idxLine = current.line, idxCol = current.col;
                auto index = parseExpr();
                eat(TOKEN_RBRACKET);
                // Bound check
                if (auto* lit = dynamic_cast<IntLitNode*>(index.get())) {
                    auto it = arraySizes.find(name);
                    if (it != arraySizes.end()) {
                        int idx = lit->value;
                        if (idx < 0 || idx >= it->second) {
                            std::string ln = getSourceLine(idxLine);
                            reportError(sourceFile, idxLine, idxCol,
                                "index " + std::to_string(idx) + " is out of bounds for array '" +
                                name + "' of size " + std::to_string(it->second) +
                                " (valid range: 0.." + std::to_string(it->second - 1) + ")",
                                ln, (int)std::to_string(idx).size());
                        }
                    }
                }
                eat(TOKEN_ASSIGN);
                auto val = parseExpr();
                eat(TOKEN_SEMI);
                return std::make_unique<ArrayAssignNode>(name, std::move(index), std::move(val), tokLine, tokCol);
            }
            if (current.type == TOKEN_DOT) {
                current = nextToken();
                std::string member = eat(TOKEN_IDENT).value;
                // p.metodo(args); — method call como statement
                if (current.type == TOKEN_LPAREN) {
                    current = nextToken();
                    std::vector<NodePtr> args;
                    while (current.type != TOKEN_RPAREN) {
                        args.push_back(parseExpr());
                        if (current.type == TOKEN_COMMA) current = nextToken();
                    }
                    eat(TOKEN_RPAREN);
                    eat(TOKEN_SEMI);
                    return std::make_unique<MethodCallNode>(name, member, std::move(args), tokLine, tokCol);
                }
                // p.campo = expr;
                eat(TOKEN_ASSIGN);
                auto val = parseExpr();
                eat(TOKEN_SEMI);
                return std::make_unique<FieldAssignNode>(name, member, std::move(val), tokLine, tokCol);
            }
            if (current.type == TOKEN_PLUS && peekToken().type == TOKEN_PLUS) {
                eat(TOKEN_PLUS); eat(TOKEN_PLUS); eat(TOKEN_SEMI);
                return std::make_unique<VarAssignNode>(name, "++", nullptr, tokLine, tokCol);
            }
            if (current.type == TOKEN_MINUS && peekToken().type == TOKEN_MINUS) {
                eat(TOKEN_MINUS); eat(TOKEN_MINUS); eat(TOKEN_SEMI);
                return std::make_unique<VarAssignNode>(name, "--", nullptr, tokLine, tokCol);
            }
            auto tryCompound2 = [&](TokenType opTok, const std::string& opStr) -> NodePtr {
                if (current.type == opTok && peekToken().type == TOKEN_ASSIGN) {
                    eat(opTok); eat(TOKEN_ASSIGN);
                    auto val = parseExpr();
                    eat(TOKEN_SEMI);
                    return std::make_unique<VarAssignNode>(name, opStr, std::move(val), tokLine, tokCol);
                }
                return nullptr;
            };
            if (auto nd = tryCompound2(TOKEN_PLUS,  "+=")) return nd;
            if (auto nd = tryCompound2(TOKEN_MINUS, "-=")) return nd;
            if (auto nd = tryCompound2(TOKEN_STAR,  "*=")) return nd;
            if (auto nd = tryCompound2(TOKEN_SLASH, "/=")) return nd;
            if (current.type == TOKEN_ASSIGN) {
                eat(TOKEN_ASSIGN);
                auto val = parseExpr();
                eat(TOKEN_SEMI);
                return std::make_unique<VarAssignNode>(name, "=", std::move(val), tokLine, tokCol);
            }
            if (current.type == TOKEN_LPAREN) {
                current = nextToken();
                std::vector<NodePtr> args;
                while (current.type != TOKEN_RPAREN) {
                    args.push_back(parseExpr());
                    if (current.type == TOKEN_COMMA) current = nextToken();
                }
                eat(TOKEN_RPAREN); eat(TOKEN_SEMI);
                return std::make_unique<CallNode>(name, std::move(args), tokLine, tokCol);
            }
            auto varExpr = std::make_unique<VarNode>(name, tokLine, tokCol);
            eat(TOKEN_SEMI);
            return varExpr;
        }
    }

    if (current.type == TOKEN_INT || current.type == TOKEN_FLOAT ||
        current.type == TOKEN_STRING) {
        int tokLine = current.line;
        int tokCol  = current.col;
        DataType type = parseDataType();
        std::string name = eat(TOKEN_IDENT).value;
        // Array local: int nums[5]; ou int nums[5] = {1,2,3,4,5};
        if (current.type == TOKEN_LBRACKET) {
            current = nextToken();
            if (current.type != TOKEN_INT_LIT) {
                std::string ln = getSourceLine(current.line);
                reportError(sourceFile, current.line, current.col,
                            "the array size must be a literal integer, but it encountered '" + current.value + "'",
                            ln, (int)current.value.size());
            }
            int size = std::stoi(eat(TOKEN_INT_LIT).value);
            eat(TOKEN_RBRACKET);
            std::vector<NodePtr> init;
            if (current.type == TOKEN_ASSIGN) {
                current = nextToken();
                eat(TOKEN_LBRACE);
                while (current.type != TOKEN_RBRACE) {
                    init.push_back(parseExpr());
                    if (current.type == TOKEN_COMMA) current = nextToken();
                }
                // Bound check no inicializador
                if ((int)init.size() > size) {
                    std::string ln = getSourceLine(tokLine);
                    reportError(sourceFile, tokLine, tokCol,
                        "array '" + name + "' has size " + std::to_string(size) +
                        " but initializer has " + std::to_string(init.size()) + " elements",
                        ln, (int)name.size());
                }
                eat(TOKEN_RBRACE);
            }
            eat(TOKEN_SEMI);
            arraySizes[name] = size;
            return std::make_unique<ArrayDeclNode>(type, name, size, std::move(init), tokLine, tokCol);
        }
        NodePtr initVal = nullptr;
        if (current.type == TOKEN_ASSIGN) {
            current = nextToken();
            initVal = parseExpr();
        }
        eat(TOKEN_SEMI);
        return std::make_unique<VarDeclNode>(type, name, std::move(initVal), tokLine, tokCol);
    }
    if (current.type == TOKEN_RETURN) {
        current = nextToken();
        // return; sem expressão → void
        if (current.type == TOKEN_SEMI) {
            current = nextToken();
            return std::make_unique<ReturnNode>(nullptr);
        }
        auto expr = parseExpr();
        eat(TOKEN_SEMI);
        return std::make_unique<ReturnNode>(std::move(expr));
    }
    if (current.type == TOKEN_PRINT) {
        current = nextToken();
        eat(TOKEN_LPAREN);
        auto expr = parseExpr();
        eat(TOKEN_RPAREN);
        eat(TOKEN_SEMI);
        return std::make_unique<PrintNode>(std::move(expr));
    }
    if (current.type == TOKEN_IF) {
        current = nextToken();
        eat(TOKEN_LPAREN);
        auto cond = parseExpr();
        eat(TOKEN_RPAREN);
        if (current.type != TOKEN_THEN) {
            std::string ln = getSourceLine(current.line);
            reportError(sourceFile, current.line, current.col,
                        "expected 'then' after the if condition, but instead found '" + current.value + "'",
                        ln, (int)current.value.size());
        }
        eat(TOKEN_THEN);
        auto thenBlock = parseBlock();
        std::vector<NodePtr> elseBlock;
        if (current.type == TOKEN_ELSE) {
            current = nextToken();
            // else if → trata como um único statement no bloco else
            if (current.type == TOKEN_IF) {
                elseBlock.push_back(parseStatement());
            } else {
                elseBlock = parseBlock();
            }
        }
        return std::make_unique<IfNode>(std::move(cond),
                                        std::move(thenBlock),
                                        std::move(elseBlock));
    }
    if (current.type == TOKEN_WHILE) {
        current = nextToken();
        eat(TOKEN_LPAREN);
        auto cond = parseExpr();
        eat(TOKEN_RPAREN);
        auto body = parseBlock();
        return std::make_unique<WhileNode>(std::move(cond), std::move(body));
    }

    if (current.type == TOKEN_FOR) {
        current = nextToken();
        eat(TOKEN_LPAREN);

        // ── init: "int i = 0" ou "i = 0" ──
        NodePtr init;
        if (current.type == TOKEN_INT || current.type == TOKEN_FLOAT || current.type == TOKEN_STRING) {
            int tl = current.line, tc = current.col;
            DataType type = parseDataType();
            std::string name = eat(TOKEN_IDENT).value;
            NodePtr initVal = nullptr;
            if (current.type == TOKEN_ASSIGN) { current = nextToken(); initVal = parseExpr(); }
            init = std::make_unique<VarDeclNode>(type, name, std::move(initVal), tl, tc);
        } else if (current.type == TOKEN_IDENT) {
            std::string name = current.value;
            int tl = current.line, tc = current.col;
            current = nextToken();
            eat(TOKEN_ASSIGN);
            auto val = parseExpr();
            init = std::make_unique<VarAssignNode>(name, "=", std::move(val), tl, tc);
        }
        eat(TOKEN_SEMI);

        // ── cond ──
        auto cond = parseExpr();
        eat(TOKEN_SEMI);

        // ── step: i++  i--  i += x  i -= x  i = expr ──
        NodePtr step;
        {
            std::string name = eat(TOKEN_IDENT).value;
            int tl = current.line, tc = current.col;
            if (current.type == TOKEN_PLUS && peekToken().type == TOKEN_PLUS) {
                eat(TOKEN_PLUS); eat(TOKEN_PLUS);
                step = std::make_unique<VarAssignNode>(name, "++", nullptr, tl, tc);
            } else if (current.type == TOKEN_MINUS && peekToken().type == TOKEN_MINUS) {
                eat(TOKEN_MINUS); eat(TOKEN_MINUS);
                step = std::make_unique<VarAssignNode>(name, "--", nullptr, tl, tc);
            } else {
                auto tryCompound = [&](TokenType opTok, const std::string& opStr) -> NodePtr {
                    if (current.type == opTok && peekToken().type == TOKEN_ASSIGN) {
                        eat(opTok); eat(TOKEN_ASSIGN);
                        auto val = parseExpr();
                        return std::make_unique<VarAssignNode>(name, opStr, std::move(val), tl, tc);
                    }
                    return nullptr;
                };
                if (auto s = tryCompound(TOKEN_PLUS,  "+=")) step = std::move(s);
                else if (auto s = tryCompound(TOKEN_MINUS, "-=")) step = std::move(s);
                else if (auto s = tryCompound(TOKEN_STAR,  "*=")) step = std::move(s);
                else if (auto s = tryCompound(TOKEN_SLASH, "/=")) step = std::move(s);
                else {
                    eat(TOKEN_ASSIGN);
                    auto val = parseExpr();
                    step = std::make_unique<VarAssignNode>(name, "=", std::move(val), tl, tc);
                }
            }
        }
        eat(TOKEN_RPAREN);

        auto body = parseBlock();
        return std::make_unique<ForNode>(std::move(init), std::move(cond), std::move(step), std::move(body));
    }

    auto expr = parseExpr();
    eat(TOKEN_SEMI);
    return expr;
}

ProgramNode parseProgram(const std::string& source, const std::string& filename) {
    sourceFile = filename;
    initLexer(source);
    current = nextToken();
    ProgramNode program;

    // Mapa de structs definidos (nome -> campos), acessível para parseStatement também
    // Declarado aqui como static para ser acessível globalmente neste translation unit
    while (current.type != TOKEN_EOF) {
        // ── struct NomeTipo { ... } ──────────────────────────────────────
        if (current.type == TOKEN_STRUCT) {
            int tokLine = current.line, tokCol = current.col;
            current = nextToken();
            std::string structName = eat(TOKEN_IDENT).value;
            eat(TOKEN_LBRACE);
            std::vector<StructField> fields;
            std::vector<StructMethod> methods;
            while (current.type != TOKEN_RBRACE) {
                // Verifica se é um método (tipo seguido de nome seguido de '(')
                // ou um campo (tipo seguido de nome seguido de ';' ou '[')
                DataType ft = parseDataType();
                std::string memberName = eat(TOKEN_IDENT).value;
                if (current.type == TOKEN_LPAREN) {
                    // É um método
                    current = nextToken();
                    std::vector<ParamNode> params;
                    while (current.type != TOKEN_RPAREN) {
                        DataType ptype = parseDataType();
                        std::string pname = eat(TOKEN_IDENT).value;
                        params.push_back({ptype, pname});
                        if (current.type == TOKEN_COMMA) current = nextToken();
                    }
                    eat(TOKEN_RPAREN);
                    auto body = parseBlock();
                    methods.push_back({ft, memberName, std::move(params), std::move(body)});
                } else {
                    // É um campo normal
                    eat(TOKEN_SEMI);
                    fields.push_back({ft, memberName});
                }
            }
            eat(TOKEN_RBRACE);
            program.declarations.push_back(
                std::make_unique<StructDefNode>(structName, std::move(fields),
                                               std::move(methods), tokLine, tokCol));
            continue;
        }

        // ── NomeTipo varName; (variável de struct) ───────────────────────
        // Detecta se é um IDENT seguido de outro IDENT (tipo struct)
        if (current.type == TOKEN_IDENT) {
            // Verifica se é declaração de variável de struct (NomeTipo varNome;)
            // Fazemos peek: TOKEN_IDENT TOKEN_IDENT
            int tokLine = current.line, tokCol = current.col;
            std::string typeName = current.value;
            current = nextToken();
            if (current.type == TOKEN_IDENT) {
                std::string varName = current.value;
                current = nextToken();
                eat(TOKEN_SEMI);
                program.declarations.push_back(
                    std::make_unique<StructVarDeclNode>(typeName, varName, tokLine, tokCol));
                continue;
            }
            // Não era uma declaração de struct: erro
            std::string srcLine = getSourceLine(current.line);
            reportError(sourceFile, current.line, current.col,
                        "invalid declaration: '" + current.value + "' — expected type ('int', 'float', 'string', 'void') to declare a variable or function.",
                        srcLine, (int)current.value.size());
        }

        if (current.type == TOKEN_INT   || current.type == TOKEN_FLOAT ||
            current.type == TOKEN_STRING || current.type == TOKEN_VOID) {
            int tokLine = current.line;
            int tokCol  = current.col;
            DataType type = parseDataType();
            std::string name = eat(TOKEN_IDENT).value;

            if (current.type == TOKEN_LPAREN) {
                current = nextToken();
                std::vector<ParamNode> params;
                while (current.type != TOKEN_RPAREN) {
                    DataType ptype = parseDataType();
                    std::string pname = eat(TOKEN_IDENT).value;
                    params.push_back({ptype, pname});
                    if (current.type == TOKEN_COMMA)
                        current = nextToken();
                }
                eat(TOKEN_RPAREN);
                auto body = parseBlock();
                program.declarations.push_back(
                    std::make_unique<FunctionNode>(type, name,
                                                   std::move(params),
                                                   std::move(body)));
            } else if (current.type == TOKEN_LBRACKET) {
                current = nextToken();
                int size = std::stoi(eat(TOKEN_INT_LIT).value);
                eat(TOKEN_RBRACKET);
                std::vector<NodePtr> init;
                if (current.type == TOKEN_ASSIGN) {
                    current = nextToken();
                    eat(TOKEN_LBRACE);
                    while (current.type != TOKEN_RBRACE) {
                        init.push_back(parseExpr());
                        if (current.type == TOKEN_COMMA) current = nextToken();
                    }
                    eat(TOKEN_RBRACE);
                }
                eat(TOKEN_SEMI);
                arraySizes[name] = size;
                program.declarations.push_back(
                    std::make_unique<ArrayDeclNode>(type, name, size, std::move(init), tokLine, tokCol));
            } else {
                NodePtr init = nullptr;
                if (current.type == TOKEN_ASSIGN) {
                    current = nextToken();
                    init = parseExpr();
                }
                eat(TOKEN_SEMI);
                program.declarations.push_back(
                    std::make_unique<VarDeclNode>(type, name, std::move(init), tokLine, tokCol));
            }
        } else {
            std::string line = getSourceLine(current.line);
            reportError(sourceFile, current.line, current.col,
                        "invalid declaration: '" + current.value + "' — expected type ('int', 'float', 'string', 'void') to declare a variable or function.",
                        line, (int)current.value.size());
        }
    }
    return program;
}