#include "../include/ast.h"
#include <cstdlib>
#include "../include/lexer.h"
#include "../include/error.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <map>
#include <vector>
#include <algorithm>

static Token current;
static std::string sourceFile;

static std::set<std::string> includedHeaders;
static std::map<std::string, int> arraySizes;

// Rastreia nomes declarados no escopo atual para "did you mean?"
static std::set<std::string> declaredFunctionNames;
static std::set<std::string> declaredVarNames;

// ── Levenshtein para sugestões "did you mean?" ────────────────────────────────
static int editDistance(const std::string& a, const std::string& b) {
    int m = (int)a.size(), n = (int)b.size();
    std::vector<std::vector<int>> dp(m+1, std::vector<int>(n+1, 0));
    for (int i = 0; i <= m; i++) dp[i][0] = i;
    for (int j = 0; j <= n; j++) dp[0][j] = j;
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= n; j++)
            dp[i][j] = (a[i-1] == b[j-1])
                ? dp[i-1][j-1]
                : 1 + std::min({dp[i-1][j], dp[i][j-1], dp[i-1][j-1]});
    return dp[m][n];
}

static std::string didYouMean(const std::string& name,
                               const std::set<std::string>& candidates) {
    std::string best;
    int bestDist = 3; // threshold: máximo 3 edições
    for (auto& c : candidates) {
        int d = editDistance(name, c);
        if (d < bestDist) { bestDist = d; best = c; }
    }
    if (!best.empty()) return "did you mean '" + best + "'?";
    return "";
}

static std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TOKEN_SEMI:      return "';'";
        case TOKEN_LPAREN:    return "'('";
        case TOKEN_RPAREN:    return "')'";
        case TOKEN_LBRACE:    return "'{'";
        case TOKEN_RBRACE:    return "'}'";
        case TOKEN_LBRACKET:  return "'['";
        case TOKEN_RBRACKET:  return "']'";
        case TOKEN_ASSIGN:    return "'='";
        case TOKEN_COMMA:     return "','";
        case TOKEN_IDENT:     return "identifier";
        case TOKEN_INT_LIT:   return "integer literal";
        case TOKEN_FLOAT_LIT: return "float literal";
        case TOKEN_STRING_LIT:return "string literal";
        case TOKEN_CHAR:      return "'char'";
        case TOKEN_CHAR_LIT:  return "char literal";
        case TOKEN_INT:       return "'int'";
        case TOKEN_FLOAT:     return "'float'";
        case TOKEN_STRING:    return "'string'";
        case TOKEN_VOID:      return "'void'";
        case TOKEN_LONG:      return "'long'";
        case TOKEN_DOUBLE:    return "'double'";
        case TOKEN_IF:        return "'if'";
        case TOKEN_ELSE:      return "'else'";
        case TOKEN_THEN:      return "'then'";
        case TOKEN_WHILE:     return "'while'";
        case TOKEN_FOR:       return "'for'";
        case TOKEN_RETURN:    return "'return'";
        case TOKEN_STRUCT:    return "'struct'";
        case TOKEN_EOF:       return "end of file";
        default:              return "unknown token";
    }
}

static Token eat(TokenType expected) {
    if (current.type != expected) {
        std::string ln = getSourceLine(current.line);
        int tlen = std::max(1, (int)current.value.size());

        if (expected == TOKEN_SEMI) {
            reportError(sourceFile, current.line, current.col,
                        "missing ';' at end of statement",
                        ln, tlen,
                        "add ';' before '" + current.value + "'");
        } else if (expected == TOKEN_THEN) {
            reportError(sourceFile, current.line, current.col,
                        "expected 'then' after if condition, but found '" + current.value + "'",
                        ln, tlen,
                        "Nova syntax: if (condition) then { ... }");
        } else if (expected == TOKEN_LBRACE) {
            reportError(sourceFile, current.line, current.col,
                        "expected '{' to open block, but found '" + current.value + "'",
                        ln, tlen,
                        "every if/for/while/function body must be wrapped in { }");
        } else if (expected == TOKEN_RBRACE) {
            reportError(sourceFile, current.line, current.col,
                        "expected '}' to close block, but found '" + current.value + "'",
                        ln, tlen,
                        "check for a missing '}' above this line");
        } else if (expected == TOKEN_RPAREN) {
            reportError(sourceFile, current.line, current.col,
                        "expected ')' to close expression, but found '" + current.value + "'",
                        ln, tlen,
                        "check for an unmatched '(' earlier");
        } else if (expected == TOKEN_LPAREN) {
            reportError(sourceFile, current.line, current.col,
                        "expected '(' here, but found '" + current.value + "'",
                        ln, tlen);
        } else if (expected == TOKEN_ASSIGN) {
            reportError(sourceFile, current.line, current.col,
                        "expected '=' in assignment, but found '" + current.value + "'",
                        ln, tlen);
        } else if (expected == TOKEN_IDENT) {
            if (current.type == TOKEN_INT || current.type == TOKEN_FLOAT ||
                current.type == TOKEN_STRING || current.type == TOKEN_VOID ||
                current.type == TOKEN_DOUBLE || current.type == TOKEN_LONG ||
                current.type == TOKEN_CHAR) {
                reportError(sourceFile, current.line, current.col,
                            "'" + current.value + "' is a reserved keyword and cannot be used as a name",
                            ln, tlen,
                            "choose a different name for your variable or function");
            } else {
                reportError(sourceFile, current.line, current.col,
                            "expected a name (identifier), but found '" + current.value + "'",
                            ln, tlen);
            }
        } else if (expected == TOKEN_RBRACKET) {
            reportError(sourceFile, current.line, current.col,
                        "expected ']' to close array index, but found '" + current.value + "'",
                        ln, tlen,
                        "check for a missing ']' in the array access");
        } else {
            reportError(sourceFile, current.line, current.col,
                        "expected " + tokenTypeName(expected) +
                        ", but found '" + current.value + "'",
                        ln, tlen);
        }
    }
    Token t = current;
    current = nextToken();
    return t;
}

// Quando retorna DataType::Custom, o caller deve ler lastCustomTypeName para saber o nome do struct.
static std::string lastCustomTypeName;

static DataType parseDataType() {
    if (current.type == TOKEN_INT)    { current = nextToken(); return DataType::Int; }
    if (current.type == TOKEN_FLOAT)  { current = nextToken(); return DataType::Float; }
    if (current.type == TOKEN_STRING) { current = nextToken(); return DataType::String; }
    if (current.type == TOKEN_CHAR)   { current = nextToken(); return DataType::Char; }
    if (current.type == TOKEN_VOID)   { current = nextToken(); return DataType::Void; }
    if (current.type == TOKEN_DOUBLE) { current = nextToken(); return DataType::Double; }
    if (current.type == TOKEN_LONG) {
        current = nextToken();
        if (current.type == TOKEN_LONG) { current = nextToken(); return DataType::LongLong; }
        return DataType::Long;
    }
    // Tipo struct: IDENT que começa com maiúscula ou é um nome de struct declarado
    // O parser não tem acesso ao registro de structs em tempo de parse, então
    // aceita qualquer IDENT como possível tipo struct — o codegen valida.
    // Também aceita namespace::Struct como tipo qualificado.
    if (current.type == TOKEN_IDENT) {
        lastCustomTypeName = current.value;
        current = nextToken();
        // Suporte a tipos qualificados: namespace::Struct
        if (current.type == TOKEN_COLONCOLON) {
            current = nextToken();
            std::string structName = eat(TOKEN_IDENT).value;
            lastCustomTypeName = lastCustomTypeName + "::" + structName;
        }
        return DataType::Custom;
    }
    std::string ln = getSourceLine(current.line);
    static const std::set<std::string> types =
        {"int","float","double","long","char","string","void"};
    std::string hint = didYouMean(current.value, types);
    if (hint.empty())
        hint = "valid types: int, float, double, long, long long, char, string, void, or a struct name";
    reportError(sourceFile, current.line, current.col,
                "'" + current.value + "' is not a valid type",
                ln, std::max(1,(int)current.value.size()), hint);
    return DataType::Int;
}

static long long safeStoll(const std::string& val, int ln, int co) {
    try { return std::stoll(val); }
    catch (const std::out_of_range&) {
        reportError(sourceFile, ln, co,
                    "integer literal '" + val + "' overflows — value is too large",
                    getSourceLine(ln), (int)val.size(),
                    "use 'long' type for large values — max int is 2147483647");
    } catch (const std::invalid_argument&) {
        reportError(sourceFile, ln, co,
                    "malformed integer literal '" + val + "'",
                    getSourceLine(ln), (int)val.size());
    }
    return 0;
}

static int safeStoi(const std::string& val, int ln, int co) {
    try { return std::stoi(val); }
    catch (const std::out_of_range&) {
        reportError(sourceFile, ln, co,
                    "array size '" + val + "' is too large",
                    getSourceLine(ln), (int)val.size(),
                    "array sizes must fit in a 32-bit integer");
    } catch (const std::invalid_argument&) {
        reportError(sourceFile, ln, co,
                    "malformed array size literal '" + val + "'",
                    getSourceLine(ln), (int)val.size());
    }
    return 0;
}

static NodePtr parseStatement();
static std::vector<NodePtr> parseBlock();
static NodePtr parseExpr();
static NodePtr parseAddSub();

static NodePtr parsePrimary() {
    if (current.type == TOKEN_NOT) {
        int tl = current.line, tc = current.col;
        current = nextToken();
        return std::make_unique<UnaryOpNode>("!", parsePrimary(), tl, tc);
    }
    if (current.type == TOKEN_MINUS) {
        int tl = current.line, tc = current.col;
        current = nextToken();
        auto operand = parsePrimary();
        return std::make_unique<BinaryOpNode>("-",
            std::make_unique<IntLitNode>(0), std::move(operand), tl, tc);
    }
    if (current.type == TOKEN_INT_LIT) {
        int tl = current.line, tc = current.col;
        long long v = safeStoll(current.value, tl, tc);
        current = nextToken();
        if (v > 2147483647LL || v < -2147483648LL)
            return std::make_unique<LongLitNode>(v);
        return std::make_unique<IntLitNode>((int)v);
    }
    if (current.type == TOKEN_FLOAT_LIT) {
        std::string raw = current.value; current = nextToken();
        return std::make_unique<FloatLitNode>(raw);
    }
    if (current.type == TOKEN_STRING_LIT) {
        std::string v = current.value; current = nextToken();
        return std::make_unique<StringLitNode>(v);
    }
    if (current.type == TOKEN_CHAR_LIT) {
        char v = current.value.empty() ? '\0' : current.value[0];
        current = nextToken();
        return std::make_unique<CharLitNode>(v);
    }
    if (current.type == TOKEN_IDENT) {
        std::string name = current.value;
        int tokLine = current.line, tokCol = current.col;
        current = nextToken();

        if (current.type == TOKEN_COLONCOLON) {
            current = nextToken();
            std::string funcName = eat(TOKEN_IDENT).value;
            std::string qualName = name + "::" + funcName;
            eat(TOKEN_LPAREN);
            std::vector<NodePtr> args;
            while (current.type != TOKEN_RPAREN) {
                if (current.type == TOKEN_EOF) {
                    reportError(sourceFile, tokLine, tokCol,
                        "unterminated argument list for '" + qualName + "()'",
                        getSourceLine(tokLine), (int)qualName.size(),
                        "add a closing ')'");
                }
                args.push_back(parseExpr());
                if (current.type == TOKEN_COMMA) current = nextToken();
            }
            eat(TOKEN_RPAREN);
            return std::make_unique<CallNode>(qualName, std::move(args), tokLine, tokCol);
        }
        if (current.type == TOKEN_LPAREN) {
            current = nextToken();
            std::vector<NodePtr> args;
            while (current.type != TOKEN_RPAREN) {
                if (current.type == TOKEN_EOF) {
                    reportError(sourceFile, tokLine, tokCol,
                        "unterminated argument list for '" + name + "()'",
                        getSourceLine(tokLine), (int)name.size(), "add a closing ')'");
                }
                args.push_back(parseExpr());
                if (current.type == TOKEN_COMMA) current = nextToken();
            }
            eat(TOKEN_RPAREN);
            return std::make_unique<CallNode>(name, std::move(args), tokLine, tokCol);
        }
        if (current.type == TOKEN_LBRACKET) {
            current = nextToken();
            int idxLine = current.line, idxCol = current.col;
            auto index = parseExpr();
            eat(TOKEN_RBRACKET);
            if (auto* lit = dynamic_cast<IntLitNode*>(index.get())) {
                auto it = arraySizes.find(name);
                if (it != arraySizes.end()) {
                    int idx = lit->value;
                    if (idx < 0) {
                        reportError(sourceFile, idxLine, idxCol,
                            "negative array index " + std::to_string(idx) +
                            " for '" + name + "'",
                            getSourceLine(idxLine),
                            (int)std::to_string(idx).size(),
                            "array indices start at 0");
                    }
                    if (idx >= it->second) {
                        reportError(sourceFile, idxLine, idxCol,
                            "index " + std::to_string(idx) +
                            " is out of bounds for array '" + name +
                            "' (size " + std::to_string(it->second) + ")",
                            getSourceLine(idxLine),
                            (int)std::to_string(idx).size(),
                            "valid indices are 0 to " + std::to_string(it->second - 1));
                    }
                }
            }
            return std::make_unique<ArrayAccessNode>(name, std::move(index), tokLine, tokCol);
        }
        if (current.type == TOKEN_DOT) {
            current = nextToken();
            std::string member = eat(TOKEN_IDENT).value;
            if (current.type == TOKEN_LPAREN) {
                current = nextToken();
                std::vector<NodePtr> args;
                while (current.type != TOKEN_RPAREN) {
                    if (current.type == TOKEN_EOF) {
                        reportError(sourceFile, tokLine, tokCol,
                            "unterminated argument list for '" + name + "." + member + "()'",
                            getSourceLine(tokLine), (int)member.size(), "add a closing ')'");
                    }
                    args.push_back(parseExpr());
                    if (current.type == TOKEN_COMMA) current = nextToken();
                }
                eat(TOKEN_RPAREN);
                return std::make_unique<MethodCallNode>(name, member, std::move(args), tokLine, tokCol);
            }
            return std::make_unique<FieldAccessNode>(name, member, tokLine, tokCol);
        }
        return std::make_unique<VarNode>(name, tokLine, tokCol);
    }
    if (current.type == TOKEN_LPAREN) {
        int castLine = current.line, castCol = current.col;
        Token next = peekToken();
        bool isCast = (next.type == TOKEN_INT   || next.type == TOKEN_FLOAT ||
                       next.type == TOKEN_DOUBLE || next.type == TOKEN_LONG  ||
                       next.type == TOKEN_CHAR   || next.type == TOKEN_STRING);
        if (isCast) {
            current = nextToken();
            DataType targetType = parseDataType();
            eat(TOKEN_RPAREN);
            return std::make_unique<CastNode>(targetType, parsePrimary(), castLine, castCol);
        }
        current = nextToken();
        auto expr = parseExpr();
        eat(TOKEN_RPAREN);
        return expr;
    }

    // ── Erros contextuais de expressão inválida ────────────────────────────
    std::string ln = getSourceLine(current.line);
    int tl = current.line, tc = current.col;
    int tlen = std::max(1, (int)current.value.size());

    if (current.type == TOKEN_EOF)
        reportError(sourceFile, tl, tc,
                    "unexpected end of file — expression is incomplete",
                    ln, 1, "check for a missing value or closing bracket above");
    if (current.type == TOKEN_SEMI)
        reportError(sourceFile, tl, tc,
                    "unexpected ';' — expected an expression here",
                    ln, 1, "remove the extra ';' or provide a value");
    if (current.type == TOKEN_RBRACE)
        reportError(sourceFile, tl, tc,
                    "unexpected '}' — expected an expression here",
                    ln, 1, "check for a missing value or an extra '}'");
    if (current.type == TOKEN_ASSIGN)
        reportError(sourceFile, tl, tc,
                    "unexpected '=' — did you mean '==' for comparison?",
                    ln, 1, "use '==' to compare values; '=' is only for assignment");
    if (current.type == TOKEN_INT    || current.type == TOKEN_FLOAT ||
        current.type == TOKEN_STRING || current.type == TOKEN_VOID  ||
        current.type == TOKEN_DOUBLE || current.type == TOKEN_LONG  ||
        current.type == TOKEN_CHAR)
        reportError(sourceFile, tl, tc,
                    "unexpected type keyword '" + current.value + "' inside expression",
                    ln, tlen,
                    "variable declarations must be at the start of a block, not inside expressions");

    reportError(sourceFile, tl, tc,
                "unexpected '" + current.value + "' — expected a value, variable name, or '('",
                ln, tlen);
    return nullptr;
}

static NodePtr parseTerm() {
    auto left = parsePrimary();
    while (current.type == TOKEN_STAR  ||
           current.type == TOKEN_SLASH ||
           current.type == TOKEN_PERCENT) {
        std::string op = current.value;
        int tl = current.line, tc = current.col;
        current = nextToken();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), parsePrimary(), tl, tc);
    }
    return left;
}

static NodePtr parseAddSub() {
    auto left = parseTerm();
    while (current.type == TOKEN_PLUS || current.type == TOKEN_MINUS) {
        std::string op = current.value;
        int tl = current.line, tc = current.col;
        current = nextToken();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), parseTerm(), tl, tc);
    }
    return left;
}

static NodePtr parseExpr();

static NodePtr parseComparison() {
    auto left = parseAddSub();
    while (current.type == TOKEN_EQ  || current.type == TOKEN_NEQ ||
           current.type == TOKEN_LT  || current.type == TOKEN_GT  ||
           current.type == TOKEN_LEQ || current.type == TOKEN_GEQ) {
        std::string op = current.value;
        int tl = current.line, tc = current.col;
        current = nextToken();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), parseAddSub(), tl, tc);
    }
    return left;
}

static NodePtr parseAnd() {
    auto left = parseComparison();
    while (current.type == TOKEN_AND) {
        int tl = current.line, tc = current.col;
        current = nextToken();
        left = std::make_unique<BinaryOpNode>("&&", std::move(left), parseComparison(), tl, tc);
    }
    return left;
}

static NodePtr parseExpr() {
    auto left = parseAnd();
    while (current.type == TOKEN_OR) {
        int tl = current.line, tc = current.col;
        current = nextToken();
        left = std::make_unique<BinaryOpNode>("||", std::move(left), parseAnd(), tl, tc);
    }
    return left;
}

static std::vector<NodePtr> parseBlock() {
    eat(TOKEN_LBRACE);
    std::vector<NodePtr> stmts;
    while (current.type != TOKEN_RBRACE && current.type != TOKEN_EOF)
        stmts.push_back(parseStatement());
    if (current.type == TOKEN_EOF)
        reportError(sourceFile, current.line, current.col,
                    "reached end of file without closing '}'",
                    getSourceLine(current.line), 1,
                    "add '}' to close the open block");
    eat(TOKEN_RBRACE);
    return stmts;
}

static NodePtr parseStatement() {
    if (current.type == TOKEN_STRUCT)
        current = nextToken(); // 'struct' opcional na declaração local

    if (current.type == TOKEN_IDENT) {
        int tokLine = current.line, tokCol = current.col;
        std::string firstName = current.value;
        current = nextToken();

        // namespace::Something — chamada de função OU declaração de variável/array de struct
        if (current.type == TOKEN_COLONCOLON) {
            current = nextToken();
            std::string secondName = eat(TOKEN_IDENT).value;
            std::string qualName = firstName + "::" + secondName;

            // geo::Point varName  /  geo::Point arr[5]  /  geo::Point varName = ...
            if (current.type == TOKEN_IDENT) {
                std::string varName = current.value;
                current = nextToken();
                // Array de struct: geo::Point arr[5];
                if (current.type == TOKEN_LBRACKET) {
                    current = nextToken();
                    if (current.type != TOKEN_INT_LIT)
                        reportError(sourceFile, current.line, current.col,
                            "array size must be a constant integer",
                            getSourceLine(current.line), std::max(1,(int)current.value.size()),
                            "example: " + qualName + " " + varName + "[10];");
                    int sizeLine = current.line, sizeCol = current.col;
                    int size = safeStoi(eat(TOKEN_INT_LIT).value, sizeLine, sizeCol);
                    if (size <= 0)
                        reportError(sourceFile, sizeLine, sizeCol,
                            "array size must be positive", getSourceLine(sizeLine), 1);
                    eat(TOKEN_RBRACKET);
                    eat(TOKEN_SEMI);
                    arraySizes[varName] = size;
                    return std::make_unique<ArrayDeclNode>(DataType::Custom, qualName, varName, size,
                                                           std::vector<NodePtr>{}, tokLine, tokCol);
                }
                // geo::Point a = func(...);  ou  geo::Point a = outraVar;
                if (current.type == TOKEN_ASSIGN) {
                    current = nextToken();
                    if (current.type == TOKEN_IDENT) {
                        std::string callName = current.value;
                        current = nextToken();
                        if (current.type == TOKEN_LPAREN) {
                            current = nextToken();
                            std::vector<NodePtr> args;
                            while (current.type != TOKEN_RPAREN) {
                                if (current.type == TOKEN_EOF)
                                    reportError(sourceFile, tokLine, tokCol,
                                        "unterminated argument list for '" + callName + "()'",
                                        getSourceLine(tokLine), (int)callName.size(), "add ')'");
                                args.push_back(parseExpr());
                                if (current.type == TOKEN_COMMA) current = nextToken();
                            }
                            eat(TOKEN_RPAREN);
                            eat(TOKEN_SEMI);
                            return std::make_unique<StructVarDeclNode>(
                                qualName, varName, callName, std::move(args), tokLine, tokCol);
                        }
                        eat(TOKEN_SEMI);
                        return std::make_unique<StructVarDeclNode>(
                            qualName, varName, "@copy:" + callName,
                            std::vector<NodePtr>{}, tokLine, tokCol);
                    }
                    reportError(sourceFile, current.line, current.col,
                        "struct variable '" + varName + "' must be initialized with a function call or another struct variable",
                        getSourceLine(current.line), std::max(1,(int)current.value.size()),
                        "example: " + qualName + " p = makePoint(1, 2);");
                }
                // geo::Point a;
                eat(TOKEN_SEMI);
                return std::make_unique<StructVarDeclNode>(qualName, varName, tokLine, tokCol);
            }

            // geo::func(...);  — chamada de função com namespace
            eat(TOKEN_LPAREN);
            std::vector<NodePtr> args;
            while (current.type != TOKEN_RPAREN) {
                if (current.type == TOKEN_EOF) {
                    reportError(sourceFile, tokLine, tokCol,
                        "unterminated argument list for '" + qualName + "()'",
                        getSourceLine(tokLine), (int)qualName.size(), "add ')'");
                }
                args.push_back(parseExpr());
                if (current.type == TOKEN_COMMA) current = nextToken();
            }
            eat(TOKEN_RPAREN);
            eat(TOKEN_SEMI);
            return std::make_unique<CallNode>(qualName, std::move(args), tokLine, tokCol);
        }
        // StructType varName;  ou  StructType varName = func(...);
        // StructType arr[N];  — array de struct
        if (current.type == TOKEN_IDENT) {
            std::string varName = current.value;
            current = nextToken();
            // StructType varName = func(...);
            if (current.type == TOKEN_ASSIGN) {
                current = nextToken();
                if (current.type == TOKEN_IDENT) {
                    std::string callName = current.value;
                    current = nextToken();
                    if (current.type == TOKEN_LPAREN) {
                        current = nextToken();
                        std::vector<NodePtr> args;
                        while (current.type != TOKEN_RPAREN) {
                            if (current.type == TOKEN_EOF)
                                reportError(sourceFile, tokLine, tokCol,
                                    "unterminated argument list for '" + callName + "()'",
                                    getSourceLine(tokLine), (int)callName.size(), "add ')'");
                            args.push_back(parseExpr());
                            if (current.type == TOKEN_COMMA) current = nextToken();
                        }
                        eat(TOKEN_RPAREN);
                        eat(TOKEN_SEMI);
                        return std::make_unique<StructVarDeclNode>(
                            firstName, varName, callName, std::move(args), tokLine, tokCol);
                    }
                    // StructType varName = outraVar;  (cópia de struct)
                    eat(TOKEN_SEMI);
                    // Cria StructVarDeclNode com initFuncName = "@copy:" + callName
                    return std::make_unique<StructVarDeclNode>(
                        firstName, varName, "@copy:" + callName,
                        std::vector<NodePtr>{}, tokLine, tokCol);
                }
                reportError(sourceFile, current.line, current.col,
                    "struct variable '" + varName + "' must be initialized with a function call or another struct variable",
                    getSourceLine(current.line), std::max(1,(int)current.value.size()),
                    "example: Point p = makePoint(1, 2);  or  Point p = other;");
            }
            // StructType arr[N];  — array de struct local
            if (current.type == TOKEN_LBRACKET) {
                current = nextToken();
                if (current.type != TOKEN_INT_LIT)
                    reportError(sourceFile, current.line, current.col,
                        "array size must be a constant integer",
                        getSourceLine(current.line), std::max(1,(int)current.value.size()),
                        "example: " + firstName + " " + varName + "[10];");
                int sizeLine = current.line, sizeCol = current.col;
                int size = safeStoi(eat(TOKEN_INT_LIT).value, sizeLine, sizeCol);
                if (size <= 0)
                    reportError(sourceFile, sizeLine, sizeCol,
                        "array size must be positive", getSourceLine(sizeLine), 1);
                eat(TOKEN_RBRACKET);
                eat(TOKEN_SEMI);
                arraySizes[varName] = size;
                return std::make_unique<ArrayDeclNode>(DataType::Custom, firstName, varName, size,
                                                       std::vector<NodePtr>{}, tokLine, tokCol);
            }
            eat(TOKEN_SEMI);
            return std::make_unique<StructVarDeclNode>(firstName, varName, tokLine, tokCol);
        }
        // Reprocessa como IDENT normal (atribuição, chamada, etc.)
        {
            std::string name = firstName;
            if (current.type == TOKEN_LBRACKET) {
                current = nextToken();
                int idxLine = current.line, idxCol = current.col;
                auto index = parseExpr();
                eat(TOKEN_RBRACKET);
                if (auto* lit = dynamic_cast<IntLitNode*>(index.get())) {
                    auto it = arraySizes.find(name);
                    if (it != arraySizes.end()) {
                        int idx = lit->value;
                        if (idx < 0)
                            reportError(sourceFile, idxLine, idxCol,
                                "negative array index " + std::to_string(idx) + " for '" + name + "'",
                                getSourceLine(idxLine), (int)std::to_string(idx).size(),
                                "array indices start at 0");
                        if (idx >= it->second)
                            reportError(sourceFile, idxLine, idxCol,
                                "index " + std::to_string(idx) +
                                " is out of bounds for array '" + name +
                                "' (size " + std::to_string(it->second) + ")",
                                getSourceLine(idxLine), (int)std::to_string(idx).size(),
                                "valid indices are 0 to " + std::to_string(it->second - 1));
                    }
                }
                // arr[i].field = val;
                if (current.type == TOKEN_DOT) {
                    current = nextToken();
                    std::string field = eat(TOKEN_IDENT).value;
                    if (current.type != TOKEN_ASSIGN)
                        reportError(sourceFile, current.line, current.col,
                            "expected '=' after '" + name + "[index]." + field + "'",
                            getSourceLine(current.line), 1,
                            "syntax: " + name + "[index]." + field + " = value;");
                    eat(TOKEN_ASSIGN);
                    auto val = parseExpr();
                    eat(TOKEN_SEMI);
                    return std::make_unique<ArrayFieldAssignNode>(
                        name, std::move(index), field, std::move(val), tokLine, tokCol);
                }
                if (current.type != TOKEN_ASSIGN) {
                    reportError(sourceFile, current.line, current.col,
                        "expected '=' after array index in assignment",
                        getSourceLine(current.line), 1,
                        "syntax: " + name + "[index] = value;");
                }
                eat(TOKEN_ASSIGN);
                // arr[i] = func(...)  ou  arr[i] = outraVar  — pode ser struct
                if (current.type == TOKEN_IDENT) {
                    LexerState st2 = saveLexerState();
                    Token saved2 = current;
                    std::string rhsName = current.value;
                    current = nextToken();
                    if (current.type == TOKEN_LPAREN) {
                        // arr[i] = func(...);  — função possivelmente retornando struct
                        current = nextToken();
                        std::vector<NodePtr> callArgs;
                        while (current.type != TOKEN_RPAREN) {
                            if (current.type == TOKEN_EOF)
                                reportError(sourceFile, tokLine, tokCol,
                                    "unterminated argument list for '" + rhsName + "()'",
                                    getSourceLine(tokLine), (int)rhsName.size(), "add ')'");
                            callArgs.push_back(parseExpr());
                            if (current.type == TOKEN_COMMA) current = nextToken();
                        }
                        eat(TOKEN_RPAREN);
                        eat(TOKEN_SEMI);
                        return std::make_unique<ArrayStructAssignNode>(
                            name, std::move(index), rhsName, std::move(callArgs), tokLine, tokCol);
                    }
                    if (current.type == TOKEN_SEMI) {
                        // arr[i] = outraVar;  — cópia de struct
                        eat(TOKEN_SEMI);
                        return std::make_unique<ArrayStructAssignNode>(
                            name, std::move(index), "@copy:" + rhsName,
                            std::vector<NodePtr>{}, tokLine, tokCol);
                    }
                    // Não é struct — restaura e parseia normalmente
                    restoreLexerState(st2);
                    current = saved2;
                }
                auto val = parseExpr();
                eat(TOKEN_SEMI);
                return std::make_unique<ArrayAssignNode>(name, std::move(index), std::move(val), tokLine, tokCol);
            }
            if (current.type == TOKEN_DOT) {
                current = nextToken();
                std::string member = eat(TOKEN_IDENT).value;
                if (current.type == TOKEN_LPAREN) {
                    current = nextToken();
                    std::vector<NodePtr> args;
                    while (current.type != TOKEN_RPAREN) {
                        if (current.type == TOKEN_EOF)
                            reportError(sourceFile, tokLine, tokCol,
                                "unterminated argument list for '" + name + "." + member + "()'",
                                getSourceLine(tokLine), (int)member.size(), "add ')'");
                        args.push_back(parseExpr());
                        if (current.type == TOKEN_COMMA) current = nextToken();
                    }
                    eat(TOKEN_RPAREN);
                    eat(TOKEN_SEMI);
                    return std::make_unique<MethodCallNode>(name, member, std::move(args), tokLine, tokCol);
                }
                if (current.type != TOKEN_ASSIGN)
                    reportError(sourceFile, current.line, current.col,
                        "expected '=' after field access '" + name + "." + member + "'",
                        getSourceLine(current.line), 1,
                        "syntax: " + name + "." + member + " = value;");
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
            auto tryCompound = [&](TokenType opTok, const std::string& opStr) -> NodePtr {
                if (current.type == opTok && peekToken().type == TOKEN_ASSIGN) {
                    eat(opTok); eat(TOKEN_ASSIGN);
                    auto val = parseExpr(); eat(TOKEN_SEMI);
                    return std::make_unique<VarAssignNode>(name, opStr, std::move(val), tokLine, tokCol);
                }
                return nullptr;
            };
            if (auto nd = tryCompound(TOKEN_PLUS,  "+=")) return nd;
            if (auto nd = tryCompound(TOKEN_MINUS, "-=")) return nd;
            if (auto nd = tryCompound(TOKEN_STAR,  "*=")) return nd;
            if (auto nd = tryCompound(TOKEN_SLASH, "/=")) return nd;
            if (current.type == TOKEN_ASSIGN) {
                eat(TOKEN_ASSIGN);
                // Verifica se é atribuição de struct: name = func(...) ou name = outraVar
                // Para isso, fazemos lookahead: se o próximo token após IDENT é '(' → função retornando struct
                if (current.type == TOKEN_IDENT) {
                    LexerState st = saveLexerState();
                    Token saved = current;
                    std::string rhsName = current.value;
                    current = nextToken();
                    // Suporte a ns::func(...) no RHS: janela = geo::makeRect(...)
                    if (current.type == TOKEN_COLONCOLON) {
                        current = nextToken();
                        rhsName = rhsName + "::" + eat(TOKEN_IDENT).value;
                    }
                    if (current.type == TOKEN_LPAREN) {
                        current = nextToken();
                        std::vector<NodePtr> args;
                        while (current.type != TOKEN_RPAREN) {
                            if (current.type == TOKEN_EOF)
                                reportError(sourceFile, tokLine, tokCol,
                                    "unterminated argument list for '" + rhsName + "()'",
                                    getSourceLine(tokLine), (int)rhsName.size(), "add ')'");
                            args.push_back(parseExpr());
                            if (current.type == TOKEN_COMMA) current = nextToken();
                        }
                        eat(TOKEN_RPAREN);
                        eat(TOKEN_SEMI);
                        return std::make_unique<StructAssignNode>(
                            name, rhsName, std::move(args), tokLine, tokCol);
                    }
                    // Não é chamada — restaura e parseia como VarAssignNode normal
                    restoreLexerState(st);
                    current = saved;
                }
                auto val = parseExpr(); eat(TOKEN_SEMI);
                return std::make_unique<VarAssignNode>(name, "=", std::move(val), tokLine, tokCol);
            }
            if (current.type == TOKEN_LPAREN) {
                current = nextToken();
                std::vector<NodePtr> args;
                while (current.type != TOKEN_RPAREN) {
                    if (current.type == TOKEN_EOF)
                        reportError(sourceFile, tokLine, tokCol,
                            "unterminated argument list for '" + name + "()'",
                            getSourceLine(tokLine), (int)name.size(), "add ')'");
                    args.push_back(parseExpr());
                    if (current.type == TOKEN_COMMA) current = nextToken();
                }
                eat(TOKEN_RPAREN); eat(TOKEN_SEMI);
                return std::make_unique<CallNode>(name, std::move(args), tokLine, tokCol);
            }
            // Nenhuma operação reconhecida — erro contextual com dica
            {
                std::string ln = getSourceLine(current.line);
                std::string hint;
                if (current.type == TOKEN_EQ)
                    hint = "use '=' for assignment: " + name + " = value;";
                else if (current.type == TOKEN_PLUS || current.type == TOKEN_MINUS ||
                         current.type == TOKEN_STAR  || current.type == TOKEN_SLASH)
                    hint = "did you forget '='? example: " + name + " = " + name + " " + current.value + " expr;";
                else
                    hint = didYouMean(name, declaredVarNames);
                reportError(sourceFile, current.line, current.col,
                            "unexpected '" + current.value + "' after '" + name + "'",
                            ln, std::max(1,(int)current.value.size()), hint);
            }
        }
    }

    // Declaração de variável local (tipos primitivos)
    if (current.type == TOKEN_INT    || current.type == TOKEN_FLOAT ||
        current.type == TOKEN_STRING || current.type == TOKEN_DOUBLE ||
        current.type == TOKEN_LONG   || current.type == TOKEN_CHAR) {
        int tokLine = current.line, tokCol = current.col;
        DataType type = parseDataType();
        std::string name = eat(TOKEN_IDENT).value;
        if (current.type == TOKEN_LBRACKET) {
            current = nextToken();
            if (current.type != TOKEN_INT_LIT) {
                reportError(sourceFile, current.line, current.col,
                            "array size must be a constant integer, but found '" + current.value + "'",
                            getSourceLine(current.line), std::max(1,(int)current.value.size()),
                            "example: int nums[10];  — dynamic sizes are not supported");
            }
            int sizeLine = current.line, sizeCol = current.col;
            int size = safeStoi(eat(TOKEN_INT_LIT).value, sizeLine, sizeCol);
            if (size <= 0)
                reportError(sourceFile, sizeLine, sizeCol,
                            "array size must be positive, but got " + std::to_string(size),
                            getSourceLine(sizeLine), std::max(1,(int)std::to_string(size).size()));
            eat(TOKEN_RBRACKET);
            std::vector<NodePtr> init;
            if (current.type == TOKEN_ASSIGN) {
                current = nextToken();
                eat(TOKEN_LBRACE);
                while (current.type != TOKEN_RBRACE) {
                    if (current.type == TOKEN_EOF)
                        reportError(sourceFile, tokLine, tokCol,
                            "unterminated array initializer for '" + name + "'",
                            getSourceLine(tokLine), (int)name.size(), "add '}'");
                    init.push_back(parseExpr());
                    if (current.type == TOKEN_COMMA) current = nextToken();
                }
                if ((int)init.size() > size)
                    reportError(sourceFile, tokLine, tokCol,
                        "too many initializers for array '" + name + "': "
                        "declared size " + std::to_string(size) +
                        " but got " + std::to_string(init.size()) + " values",
                        getSourceLine(tokLine), (int)name.size(),
                        "remove " + std::to_string((int)init.size() - size) + " extra value(s)");
                eat(TOKEN_RBRACE);
            }
            eat(TOKEN_SEMI);
            arraySizes[name] = size;
            return std::make_unique<ArrayDeclNode>(type, name, size, std::move(init), tokLine, tokCol);
        }
        NodePtr initVal = nullptr;
        if (current.type == TOKEN_ASSIGN) { current = nextToken(); initVal = parseExpr(); }
        eat(TOKEN_SEMI);
        declaredVarNames.insert(name);
        return std::make_unique<VarDeclNode>(type, name, std::move(initVal), tokLine, tokCol);
    }

    if (current.type == TOKEN_RETURN) {
        current = nextToken();
        if (current.type == TOKEN_SEMI) { current = nextToken(); return std::make_unique<ReturnNode>(nullptr); }
        auto expr = parseExpr();
        eat(TOKEN_SEMI);
        return std::make_unique<ReturnNode>(std::move(expr));
    }

    if (current.type == TOKEN_IF) {
        current = nextToken();
        if (current.type != TOKEN_LPAREN)
            reportError(sourceFile, current.line, current.col,
                        "expected '(' after 'if', but found '" + current.value + "'",
                        getSourceLine(current.line), std::max(1,(int)current.value.size()),
                        "syntax: if (condition) then { ... }");
        eat(TOKEN_LPAREN);
        auto cond = parseExpr();
        eat(TOKEN_RPAREN);
        if (current.type != TOKEN_THEN) {
            std::string hint = "Nova requires 'then': if (cond) then { ... }";
            if (current.type == TOKEN_LBRACE)
                hint = "insert 'then' between ')' and '{': if (cond) then { ... }";
            reportError(sourceFile, current.line, current.col,
                        "expected 'then' after if condition, but found '" + current.value + "'",
                        getSourceLine(current.line), std::max(1,(int)current.value.size()), hint);
        }
        eat(TOKEN_THEN);
        auto thenBlock = parseBlock();
        std::vector<NodePtr> elseBlock;
        if (current.type == TOKEN_ELSE) {
            current = nextToken();
            if (current.type == TOKEN_IF) elseBlock.push_back(parseStatement());
            else elseBlock = parseBlock();
        }
        return std::make_unique<IfNode>(std::move(cond), std::move(thenBlock), std::move(elseBlock));
    }

    if (current.type == TOKEN_WHILE) {
        current = nextToken();
        if (current.type != TOKEN_LPAREN)
            reportError(sourceFile, current.line, current.col,
                        "expected '(' after 'while', but found '" + current.value + "'",
                        getSourceLine(current.line), std::max(1,(int)current.value.size()),
                        "syntax: while (condition) { ... }");
        eat(TOKEN_LPAREN);
        auto cond = parseExpr();
        eat(TOKEN_RPAREN);
        return std::make_unique<WhileNode>(std::move(cond), parseBlock());
    }

    if (current.type == TOKEN_FOR) {
        current = nextToken();
        if (current.type != TOKEN_LPAREN)
            reportError(sourceFile, current.line, current.col,
                        "expected '(' after 'for', but found '" + current.value + "'",
                        getSourceLine(current.line), std::max(1,(int)current.value.size()),
                        "syntax: for (init; condition; step) { ... }");
        eat(TOKEN_LPAREN);
        NodePtr init;
        if (current.type == TOKEN_INT  || current.type == TOKEN_FLOAT  ||
            current.type == TOKEN_STRING || current.type == TOKEN_DOUBLE ||
            current.type == TOKEN_LONG   || current.type == TOKEN_CHAR) {
            int tl = current.line, tc = current.col;
            DataType type = parseDataType();
            std::string nm = eat(TOKEN_IDENT).value;
            NodePtr iv = nullptr;
            if (current.type == TOKEN_ASSIGN) { current = nextToken(); iv = parseExpr(); }
            init = std::make_unique<VarDeclNode>(type, nm, std::move(iv), tl, tc);
        } else if (current.type == TOKEN_IDENT) {
            std::string nm = current.value;
            int tl = current.line, tc = current.col;
            current = nextToken(); eat(TOKEN_ASSIGN);
            init = std::make_unique<VarAssignNode>(nm, "=", parseExpr(), tl, tc);
        } else if (current.type != TOKEN_SEMI) {
            reportError(sourceFile, current.line, current.col,
                        "invalid for-loop initializer — expected a variable declaration or assignment",
                        getSourceLine(current.line), std::max(1,(int)current.value.size()),
                        "example: for (int i = 0; i < 10; i++) { ... }");
        }
        eat(TOKEN_SEMI);
        auto cond = parseExpr();
        eat(TOKEN_SEMI);
        NodePtr step;
        {
            if (current.type != TOKEN_IDENT)
                reportError(sourceFile, current.line, current.col,
                            "expected variable name for for-loop step, but found '" + current.value + "'",
                            getSourceLine(current.line), std::max(1,(int)current.value.size()),
                            "valid steps: i++,  i--,  i += 2,  i = i + 1");
            std::string nm = eat(TOKEN_IDENT).value;
            int tl = current.line, tc = current.col;
            if (current.type == TOKEN_PLUS && peekToken().type == TOKEN_PLUS)
                { eat(TOKEN_PLUS); eat(TOKEN_PLUS); step = std::make_unique<VarAssignNode>(nm, "++", nullptr, tl, tc); }
            else if (current.type == TOKEN_MINUS && peekToken().type == TOKEN_MINUS)
                { eat(TOKEN_MINUS); eat(TOKEN_MINUS); step = std::make_unique<VarAssignNode>(nm, "--", nullptr, tl, tc); }
            else {
                auto tryC = [&](TokenType opTok, const std::string& opStr) -> NodePtr {
                    if (current.type == opTok && peekToken().type == TOKEN_ASSIGN) {
                        eat(opTok); eat(TOKEN_ASSIGN);
                        return std::make_unique<VarAssignNode>(nm, opStr, parseExpr(), tl, tc);
                    }
                    return nullptr;
                };
                if      (auto s = tryC(TOKEN_PLUS,  "+=")) step = std::move(s);
                else if (auto s = tryC(TOKEN_MINUS, "-=")) step = std::move(s);
                else if (auto s = tryC(TOKEN_STAR,  "*=")) step = std::move(s);
                else if (auto s = tryC(TOKEN_SLASH, "/=")) step = std::move(s);
                else {
                    if (current.type != TOKEN_ASSIGN)
                        reportError(sourceFile, current.line, current.col,
                                    "invalid for-loop step after '" + nm + "'",
                                    getSourceLine(current.line), 1,
                                    "valid: " + nm + "++,  " + nm + "--,  " + nm + " += expr");
                    eat(TOKEN_ASSIGN);
                    step = std::make_unique<VarAssignNode>(nm, "=", parseExpr(), tl, tc);
                }
            }
        }
        eat(TOKEN_RPAREN);
        return std::make_unique<ForNode>(std::move(init), std::move(cond), std::move(step), parseBlock());
    }

    // ── asm / ir ─────────────────────────────────────────────────────────────
    auto extractVarRefs = [](const std::string& raw) {
        std::vector<std::string> vars; std::set<std::string> seen;
        for (size_t i = 0; i < raw.size(); i++) {
            if (raw[i] == '$' && i+1 < raw.size() &&
                (std::isalpha((unsigned char)raw[i+1]) || raw[i+1] == '_')) {
                std::string nm; size_t j = i+1;
                while (j < raw.size() && (std::isalnum((unsigned char)raw[j]) || raw[j] == '_'))
                    nm += raw[j++];
                if (!seen.count(nm)) { vars.push_back(nm); seen.insert(nm); }
            }
        }
        return vars;
    };

    if (current.type == TOKEN_ASM || current.type == TOKEN_IR) {
        bool isAsm = (current.type == TOKEN_ASM);
        int tl = current.line, tc = current.col;
        current = nextToken();
        if (current.type != TOKEN_LBRACE)
            reportError(sourceFile, current.line, current.col,
                        std::string("expected '{' after '") + (isAsm ? "asm" : "ir") + "'",
                        getSourceLine(current.line), 1,
                        isAsm ? "syntax: asm { \"instruction\" }"
                              : "syntax: ir { llvm_instruction }");
        current = nextToken();
        std::string code; int depth = 1;
        while (current.type != TOKEN_EOF && depth > 0) {
            if (current.type == TOKEN_LBRACE)
                { depth++; code += "{ "; current = nextToken(); }
            else if (current.type == TOKEN_RBRACE) {
                depth--;
                if (depth > 0) { code += "} "; current = nextToken(); }
                else current = nextToken();
            } else if (current.type == TOKEN_STRING_LIT)
                { code += "\"" + current.value + "\" "; current = nextToken(); }
            else if (!isAsm && current.type == TOKEN_PERCENT)
                { code += "% "; current = nextToken(); }
            else { code += current.value + " "; current = nextToken(); }
        }
        if (depth > 0)
            reportError(sourceFile, tl, tc,
                        std::string("unterminated '") + (isAsm ? "asm" : "ir") + "' block — missing '}'",
                        getSourceLine(tl), isAsm ? 3 : 2);
        while (!code.empty() && code.back() == ' ') code.pop_back();
        auto vars = extractVarRefs(code);
        if (isAsm) return std::make_unique<AsmNode>(code, "", vars, tl, tc);
        return std::make_unique<IrNode>(code, vars, tl, tc);
    }

    // ── Erro final: statement inválido ────────────────────────────────────────
    {
        std::string ln = getSourceLine(current.line);
        int tl = current.line, tc = current.col;
        int tlen = std::max(1,(int)current.value.size());

        if (current.type == TOKEN_RBRACE)
            reportError(sourceFile, tl, tc,
                        "unexpected '}' — too many closing braces",
                        ln, 1, "check for an extra '}' or a missing '{'");
        if (current.type == TOKEN_EOF)
            reportError(sourceFile, tl, tc,
                        "unexpected end of file inside function body",
                        ln, 1, "add '}' to close the open function");

        std::string hint;
        if (current.type == TOKEN_IDENT) {
            hint = didYouMean(current.value, declaredVarNames);
            if (hint.empty()) hint = didYouMean(current.value, declaredFunctionNames);
        }
        if (hint.empty())
            hint = "statements must start with a type, variable name, or control keyword (if/for/while/return)";
        reportError(sourceFile, tl, tc,
                    "invalid statement starting with '" + current.value + "'",
                    ln, tlen, hint);
    }
    return nullptr;
}

ProgramNode parseProgram(const std::string& source, const std::string& filename);

// ── Parser de arquivos .nh ─────────────────────────────────────────────────────
static std::vector<NodePtr> parseNhFile(const std::string& nhPath,
                                         const std::string& includedFrom) {
    if (includedHeaders.count(nhPath)) return {};
    includedHeaders.insert(nhPath);

    std::ifstream f(nhPath);
    if (!f) {
        std::cerr << "\n" << BOLD << RED << "error: " << RESET << BOLD
                  << "cannot open header file '" << nhPath << "'\n" << RESET;
        std::cerr << BOLD << "  included from: " << RESET << includedFrom << "\n";
        bool isSystem = (nhPath.find("/usr/local/lib/nova") != std::string::npos) ||
                        (std::getenv("NOVA_STDLIB_PATH") &&
                         nhPath.find(std::getenv("NOVA_STDLIB_PATH")) != std::string::npos);
        if (isSystem) {
            std::cerr << CYAN << "  hint: " << RESET
                      << "system header not found — is NOVA_STDLIB_PATH set correctly?\n";
            std::cerr << CYAN << "  hint: " << RESET
                      << "example: export NOVA_STDLIB_PATH=/usr/local/lib/nova\n";
        } else {
            std::cerr << CYAN << "  hint: " << RESET
                      << "check that the path is relative to '" << includedFrom << "'\n";
            std::cerr << CYAN << "  hint: " << RESET
                      << "header files must have the .nh extension\n";
        }
        std::cerr << "\n";
        exit(1);
    }
    std::stringstream buf; buf << f.rdbuf();
    std::string nhSource = buf.str();

    LexerState savedLexer  = saveLexerState();
    Token savedCurrent     = current;
    std::string savedSrc   = sourceFile;

    initLexer(nhSource);
    sourceFile = nhPath;
    current = nextToken();

    std::string currentNhNamespace;
    std::vector<NodePtr> decls;

    while (current.type != TOKEN_EOF) {
        if (current.type == TOKEN_NAMESPACE) {
            current = nextToken();
            currentNhNamespace = eat(TOKEN_IDENT).value;
            eat(TOKEN_SEMI);
            continue;
        }
        if (current.type == TOKEN_STRUCT) {
            int tl = current.line, tc = current.col;
            current = nextToken();
            std::string sName = eat(TOKEN_IDENT).value;
            // Se há namespace ativo, qualifica o nome do struct: ns::StructName
            std::string qualSName = currentNhNamespace.empty() ? sName : currentNhNamespace + "::" + sName;
            eat(TOKEN_LBRACE);
            std::vector<StructField> fields;
            while (current.type != TOKEN_RBRACE && current.type != TOKEN_EOF) {
                if (current.type == TOKEN_INT   || current.type == TOKEN_FLOAT  ||
                    current.type == TOKEN_STRING || current.type == TOKEN_DOUBLE ||
                    current.type == TOKEN_LONG   || current.type == TOKEN_VOID   ||
                    current.type == TOKEN_CHAR) {
                    DataType ft = parseDataType();
                    std::string fn = eat(TOKEN_IDENT).value;
                    eat(TOKEN_SEMI);
                    fields.push_back({ft, fn});
                } else {
                    reportError(nhPath, current.line, current.col,
                                "struct fields in .nh must be primitive types",
                                getSourceLine(current.line), std::max(1,(int)current.value.size()),
                                "nested structs in .nh are not supported yet");
                }
            }
            if (current.type == TOKEN_EOF)
                reportError(nhPath, current.line, current.col,
                            "unterminated struct '" + sName + "' — missing '}'",
                            getSourceLine(current.line), 1);
            eat(TOKEN_RBRACE);
            if (current.type == TOKEN_SEMI) current = nextToken();
            decls.push_back(std::make_unique<StructDefNode>(
                qualSName, std::move(fields), std::vector<StructMethod>{}, tl, tc));
            continue;
        }
        if (current.type == TOKEN_INT    || current.type == TOKEN_FLOAT  ||
            current.type == TOKEN_STRING || current.type == TOKEN_VOID   ||
            current.type == TOKEN_DOUBLE || current.type == TOKEN_LONG   ||
            current.type == TOKEN_CHAR   || current.type == TOKEN_IDENT) {
            int tl = current.line, tc = current.col;
            // Trata IDENT como possível tipo de retorno de struct
            DataType retType = parseDataType();
            std::string retStructTypeName = (retType == DataType::Custom) ? lastCustomTypeName : "";
            // Se retType == Custom e next não é IDENT, é um erro
            std::string name = eat(TOKEN_IDENT).value;
            if (current.type == TOKEN_LPAREN) {
                current = nextToken();
                std::vector<ParamNode> params;
                bool isVariadic = false;
                while (current.type != TOKEN_RPAREN && current.type != TOKEN_EOF) {
                    if (current.type == TOKEN_ELLIPSIS) { isVariadic = true; current = nextToken(); break; }
                    if (current.type == TOKEN_INT    || current.type == TOKEN_FLOAT  ||
                        current.type == TOKEN_STRING || current.type == TOKEN_VOID   ||
                        current.type == TOKEN_DOUBLE || current.type == TOKEN_LONG   ||
                        current.type == TOKEN_CHAR   || current.type == TOKEN_IDENT) {
                        DataType ptype = parseDataType();
                        std::string pStructType = (ptype == DataType::Custom) ? lastCustomTypeName : "";
                        std::string pname = "_";
                        if (current.type == TOKEN_IDENT) pname = eat(TOKEN_IDENT).value;
                        if (current.type == TOKEN_LBRACKET) {
                            current = nextToken();
                            if (current.type == TOKEN_INT_LIT) current = nextToken();
                            eat(TOKEN_RBRACKET);
                        }
                        if (ptype == DataType::Custom)
                            params.push_back({DataType::Void, "__struct__" + pStructType + "::" + pname, pStructType});
                        else
                            params.push_back({ptype, pname, ""});
                    } else break;
                    if (current.type == TOKEN_COMMA) current = nextToken();
                }
                if (current.type == TOKEN_EOF)
                    reportError(nhPath, tl, tc,
                                "unterminated parameter list for '" + name + "' in header",
                                getSourceLine(tl), (int)name.size(),
                                "add ')' and ';' to complete the declaration");
                eat(TOKEN_RPAREN);
                eat(TOKEN_SEMI);
                std::string qualName = currentNhNamespace.empty() ? name : currentNhNamespace + "::" + name;
                decls.push_back(std::make_unique<FuncDeclNode>(
                    retType, retStructTypeName, qualName, std::move(params), isVariadic));
            } else {
                NodePtr init = nullptr;
                if (current.type == TOKEN_ASSIGN) {
                    current = nextToken();
                    if (current.type == TOKEN_INT_LIT) {
                        long long vl = safeStoll(current.value, current.line, current.col);
                        current = nextToken();
                        init = (vl > 2147483647LL || vl < -2147483648LL)
                            ? (NodePtr)std::make_unique<LongLitNode>(vl)
                            : (NodePtr)std::make_unique<IntLitNode>((int)vl);
                    } else if (current.type == TOKEN_FLOAT_LIT) {
                        init = std::make_unique<FloatLitNode>(current.value); current = nextToken();
                    } else if (current.type == TOKEN_STRING_LIT) {
                        init = std::make_unique<StringLitNode>(current.value); current = nextToken();
                    } else {
                        reportError(nhPath, current.line, current.col,
                                    "only literal values are allowed as initializers in .nh files",
                                    getSourceLine(current.line), std::max(1,(int)current.value.size()),
                                    "expressions and function calls are not permitted here");
                    }
                }
                eat(TOKEN_SEMI);
                std::string qn = currentNhNamespace.empty() ? name : currentNhNamespace + "::" + name;
                decls.push_back(std::make_unique<VarDeclNode>(retType, qn, std::move(init), tl, tc));
            }
            continue;
        }
        reportError(nhPath, current.line, current.col,
                    "unexpected '" + current.value + "' in header file",
                    getSourceLine(current.line), std::max(1,(int)current.value.size()),
                    ".nh files may only contain function signatures, struct definitions, "
                    "and global variable declarations");
    }

    restoreLexerState(savedLexer);
    sourceFile = savedSrc;
    current    = savedCurrent;
    return decls;
}

ProgramNode parseProgram(const std::string& source, const std::string& filename) {
    sourceFile = filename;
    includedHeaders.clear();
    arraySizes.clear();
    declaredFunctionNames.clear();
    declaredVarNames.clear();
    initLexer(source);
    current = nextToken();
    ProgramNode program;

    while (current.type != TOKEN_EOF) {
        if (current.type == TOKEN_HASH) {
            int tl = current.line, tc = current.col;
            current = nextToken();
            if (current.type != TOKEN_INCLUDE)
                reportError(sourceFile, tl, tc,
                            "expected 'include' after '#', but found '" + current.value + "'",
                            getSourceLine(tl), 1,
                            "only '#include' directives are supported");
            current = nextToken();
            std::string headerPath; bool isSystem = false;
            if (current.type == TOKEN_STRING_LIT)
                { headerPath = current.value; current = nextToken(); }
            else if (current.type == TOKEN_HEADER_PATH)
                { headerPath = current.value; isSystem = true; current = nextToken(); }
            else
                reportError(sourceFile, current.line, current.col,
                            "expected a filename after '#include', but found '" + current.value + "'",
                            getSourceLine(current.line), std::max(1,(int)current.value.size()),
                            "use quotes for local: #include \"file.nh\"  "
                            "or angle brackets for stdlib: #include <stdio.nh>");
            std::string fullPath;
            if (isSystem) {
                const char* ev = std::getenv("NOVA_STDLIB_PATH");
                fullPath = (ev ? std::string(ev) : "/usr/local/lib/nova") + "/" + headerPath;
            } else {
                std::string base;
                auto sl = sourceFile.rfind('/');
                if (sl != std::string::npos) base = sourceFile.substr(0, sl + 1);
                fullPath = base + headerPath;
            }
            for (auto& d : parseNhFile(fullPath, sourceFile))
                program.declarations.push_back(std::move(d));
            continue;
        }
        if (current.type == TOKEN_STRUCT) {
            int tokLine = current.line, tokCol = current.col;
            current = nextToken();
            std::string sName = eat(TOKEN_IDENT).value;
            eat(TOKEN_LBRACE);
            std::vector<StructField> fields;
            std::vector<StructMethod> methods;
            while (current.type != TOKEN_RBRACE) {
                if (current.type == TOKEN_EOF)
                    reportError(sourceFile, tokLine, tokCol,
                                "unterminated struct '" + sName + "' — missing '}'",
                                getSourceLine(tokLine), (int)sName.size());
                DataType ft = parseDataType();
                std::string ftStructName = (ft == DataType::Custom) ? lastCustomTypeName : "";
                std::string mName = eat(TOKEN_IDENT).value;
                if (current.type == TOKEN_LPAREN) {
                    current = nextToken();
                    std::vector<ParamNode> params;
                    while (current.type != TOKEN_RPAREN) {
                        if (current.type == TOKEN_EOF)
                            reportError(sourceFile, tokLine, tokCol,
                                "unterminated parameter list for method '" + mName + "'",
                                getSourceLine(tokLine), (int)mName.size());
                        DataType ptype = parseDataType();
                        std::string pStructType = (ptype == DataType::Custom) ? lastCustomTypeName : "";
                        std::string pname = eat(TOKEN_IDENT).value;
                        if (current.type == TOKEN_LBRACKET) {
                            current = nextToken();
                            if (current.type == TOKEN_INT_LIT) current = nextToken();
                            eat(TOKEN_RBRACKET);
                        }
                        params.push_back({ptype, pname, pStructType});
                        if (current.type == TOKEN_COMMA) current = nextToken();
                    }
                    eat(TOKEN_RPAREN);
                    methods.push_back({ft, ftStructName, mName, std::move(params), parseBlock()});
                } else {
                    // Campo — não pode ser Custom (struct aninhado não suportado ainda)
                    if (ft == DataType::Custom)
                        reportError(sourceFile, tokLine, tokCol,
                            "nested struct fields inside struct '" + sName + "' are not supported",
                            getSourceLine(tokLine), (int)sName.size(),
                            "use primitive types for struct fields: int, float, double, char, string");
                    eat(TOKEN_SEMI);
                    fields.push_back({ft, mName});
                }
            }
            eat(TOKEN_RBRACE);
            program.declarations.push_back(
                std::make_unique<StructDefNode>(sName, std::move(fields), std::move(methods), tokLine, tokCol));
            continue;
        }
        if (current.type == TOKEN_IDENT) {
            int tokLine = current.line, tokCol = current.col;
            std::string typeName = current.value;
            current = nextToken();

            // Tipo qualificado: namespace::Struct memberName ...
            if (current.type == TOKEN_COLONCOLON) {
                current = nextToken();
                std::string structPart = eat(TOKEN_IDENT).value;
                typeName = typeName + "::" + structPart;
                // Agora typeName = "ns::Struct" — espera memberName
            }

            if (current.type == TOKEN_IDENT) {
                std::string memberName = current.value;
                current = nextToken();
                // StructType funcName(...) { ... }  — função retornando struct
                if (current.type == TOKEN_LPAREN) {
                    current = nextToken();
                    std::vector<ParamNode> params;
                    bool isVariadic = false;
                    while (current.type != TOKEN_RPAREN) {
                        if (current.type == TOKEN_EOF)
                            reportError(sourceFile, tokLine, tokCol,
                                "unterminated parameter list for function '" + memberName + "'",
                                getSourceLine(tokLine), (int)memberName.size(),
                                "add closing ')' and the function body");
                        if (current.type == TOKEN_ELLIPSIS) { isVariadic = true; current = nextToken(); break; }
                        DataType ptype = parseDataType();
                        std::string pStructType = (ptype == DataType::Custom) ? lastCustomTypeName : "";
                        std::string pname = eat(TOKEN_IDENT).value;
                        if (current.type == TOKEN_LBRACKET) {
                            current = nextToken();
                            if (current.type == TOKEN_INT_LIT) current = nextToken();
                            eat(TOKEN_RBRACKET);
                        }
                        params.push_back({ptype, pname, pStructType});
                        if (current.type == TOKEN_COMMA) current = nextToken();
                    }
                    eat(TOKEN_RPAREN);
                    if (current.type == TOKEN_SEMI)
                        reportError(sourceFile, current.line, current.col,
                                    "function '" + memberName + "' has no body",
                                    getSourceLine(current.line), 1,
                                    "add the function body: { ... }");
                    declaredFunctionNames.insert(memberName);
                    program.declarations.push_back(
                        std::make_unique<FunctionNode>(DataType::Custom, typeName, memberName,
                                                       std::move(params), parseBlock(), isVariadic));
                    continue;
                }
                // StructType arr[N];  — array global de struct
                if (current.type == TOKEN_LBRACKET) {
                    current = nextToken();
                    if (current.type != TOKEN_INT_LIT)
                        reportError(sourceFile, current.line, current.col,
                            "array size must be a constant integer",
                            getSourceLine(current.line), std::max(1,(int)current.value.size()),
                            "example: " + typeName + " " + memberName + "[10];");
                    int sizeLine = current.line, sizeCol = current.col;
                    int size = safeStoi(eat(TOKEN_INT_LIT).value, sizeLine, sizeCol);
                    if (size <= 0)
                        reportError(sourceFile, sizeLine, sizeCol,
                            "global array '" + memberName + "' size must be positive",
                            getSourceLine(sizeLine), 1);
                    eat(TOKEN_RBRACKET);
                    eat(TOKEN_SEMI);
                    arraySizes[memberName] = size;
                    program.declarations.push_back(
                        std::make_unique<ArrayDeclNode>(DataType::Custom, typeName, memberName, size,
                                                        std::vector<NodePtr>{}, tokLine, tokCol));
                    continue;
                }
                // StructType varName;  — variável global de struct
                eat(TOKEN_SEMI);
                program.declarations.push_back(
                    std::make_unique<StructVarDeclNode>(typeName, memberName, tokLine, tokCol));
                continue;
            }
            std::string hint;
            if (current.type == TOKEN_LPAREN)
                hint = "to declare a function, add a return type: int " + typeName + "(...) { ... }";
            else if (current.type == TOKEN_ASSIGN)
                hint = "to declare a variable, add a type: int " + typeName + " = ...;";
            reportError(sourceFile, current.line, current.col,
                        "unexpected '" + current.value + "' after '" + typeName + "' at top level",
                        getSourceLine(current.line), std::max(1,(int)current.value.size()), hint);
        }
        if (current.type == TOKEN_INT   || current.type == TOKEN_FLOAT ||
            current.type == TOKEN_STRING || current.type == TOKEN_VOID ||
            current.type == TOKEN_DOUBLE || current.type == TOKEN_LONG ||
            current.type == TOKEN_CHAR   || current.type == TOKEN_IDENT) {
            // Se é IDENT seguido de IDENT → struct var global OU função retornando struct
            // (já tratado abaixo no bloco TOKEN_IDENT de nível superior)
            // Aqui só entramos se for tipo primitivo
            if (current.type == TOKEN_IDENT) goto try_top_level_ident;
            int tokLine = current.line, tokCol = current.col;
            DataType type = parseDataType();
            std::string retStructType = (type == DataType::Custom) ? lastCustomTypeName : "";
            std::string name = eat(TOKEN_IDENT).value;
            if (current.type == TOKEN_LPAREN) {
                current = nextToken();
                std::vector<ParamNode> params;
                bool isVariadic = false;
                while (current.type != TOKEN_RPAREN) {
                    if (current.type == TOKEN_EOF)
                        reportError(sourceFile, tokLine, tokCol,
                            "unterminated parameter list for function '" + name + "'",
                            getSourceLine(tokLine), (int)name.size(),
                            "add closing ')' and the function body");
                    if (current.type == TOKEN_ELLIPSIS) { isVariadic = true; current = nextToken(); break; }
                    DataType ptype = parseDataType();
                    std::string pStructType = (ptype == DataType::Custom) ? lastCustomTypeName : "";
                    std::string pname = eat(TOKEN_IDENT).value;
                    if (current.type == TOKEN_LBRACKET) {
                        current = nextToken();
                        if (current.type == TOKEN_INT_LIT) current = nextToken();
                        eat(TOKEN_RBRACKET);
                    }
                    params.push_back({ptype, pname, pStructType});
                    if (current.type == TOKEN_COMMA) current = nextToken();
                }
                eat(TOKEN_RPAREN);
                if (current.type == TOKEN_SEMI)
                    reportError(sourceFile, current.line, current.col,
                                "function '" + name + "' has no body",
                                getSourceLine(current.line), 1,
                                "add the function body: { ... }  — "
                                "for forward declarations, use a .nh header file");
                declaredFunctionNames.insert(name);
                program.declarations.push_back(
                    std::make_unique<FunctionNode>(type, retStructType, name,
                                                   std::move(params), parseBlock(), isVariadic));
            } else if (current.type == TOKEN_LBRACKET) {
                current = nextToken();
                int sizeLine = current.line, sizeCol = current.col;
                int size = safeStoi(eat(TOKEN_INT_LIT).value, sizeLine, sizeCol);
                if (size <= 0)
                    reportError(sourceFile, sizeLine, sizeCol,
                                "global array '" + name + "' size must be positive",
                                getSourceLine(sizeLine), 1);
                eat(TOKEN_RBRACKET);
                std::vector<NodePtr> init;
                if (current.type == TOKEN_ASSIGN) {
                    current = nextToken(); eat(TOKEN_LBRACE);
                    while (current.type != TOKEN_RBRACE) {
                        if (current.type == TOKEN_EOF)
                            reportError(sourceFile, tokLine, tokCol,
                                "unterminated initializer for global array '" + name + "'",
                                getSourceLine(tokLine), (int)name.size(), "add '}'");
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
                if (current.type == TOKEN_ASSIGN) { current = nextToken(); init = parseExpr(); }
                eat(TOKEN_SEMI);
                program.declarations.push_back(
                    std::make_unique<VarDeclNode>(type, name, std::move(init), tokLine, tokCol));
            }
            continue;
        }
        {
            try_top_level_ident:
            std::string hint = didYouMean(current.value, declaredFunctionNames);
            if (hint.empty())
                hint = "top-level code must be a function, variable, or struct declaration";
            reportError(sourceFile, current.line, current.col,
                        "unexpected '" + current.value + "' at top level",
                        getSourceLine(current.line), std::max(1,(int)current.value.size()), hint);
        }
    }
    return program;
}