#pragma once
#include <string>
#include <memory>
#include <vector>

//tipos de dados
enum class DataType { Int, Float, String, Void, Long, LongLong, Double, Char, Custom };

//classe base de nó
struct ASTNode{
    virtual ~ASTNode() = default;
};

using NodePtr = std::unique_ptr<ASTNode>;

// ------- Expressões -------


// 1: Literais
//inteiro
struct IntLitNode : ASTNode{
    int value;
    IntLitNode(int v) : value(v) {}
};

// long long literal — para números que não cabem em int32 (> 2147483647)
struct LongLitNode : ASTNode{
    long long value;
    LongLitNode(long long v) : value(v) {}
};

//float — valor guardado como string para preservar precisão total
struct FloatLitNode : ASTNode{
    std::string raw; // ex: "3.14159265358979"
    FloatLitNode(const std::string& v) : raw(v) {}
};

//string
struct StringLitNode: ASTNode{
    std::string value;
    StringLitNode(const std::string& v) : value(v) {}
};

// char literal: 'a', '\n', etc. — armazenado como int (código ASCII)
struct CharLitNode : ASTNode {
    char value;
    CharLitNode(char v) : value(v) {}
};

//2. variaveis
struct VarName : ASTNode{
    std::string name;
    VarName(const std::string& v) : name(v) {}
};

// Operação binária
struct BinaryOpNode : ASTNode {
    std::string op;
    NodePtr left, right;
    int line, col;
    BinaryOpNode(const std::string& op, NodePtr l, NodePtr r, int ln = 0, int c = 0)
        : op(op), left(std::move(l)), right(std::move(r)), line(ln), col(c) {}
};

// Operação unária: !expr
struct UnaryOpNode : ASTNode {
    std::string op;
    NodePtr operand;
    int line, col;
    UnaryOpNode(const std::string& o, NodePtr e, int l, int c)
        : op(o), operand(std::move(e)), line(l), col(c) {}
};

// Chamada de função
struct CallNode : ASTNode {
    std::string name;
    std::vector<NodePtr> args;
    int line, col;
    CallNode(const std::string& n, std::vector<NodePtr> a, int l, int c)
        : name(n), args(std::move(a)), line(l), col(c) {}
};

// Cast explícito: (int)x, (float)x, (double)expr
struct CastNode : ASTNode {
    DataType targetType;
    NodePtr expr;
    int line, col;
    CastNode(DataType t, NodePtr e, int l, int c)
        : targetType(t), expr(std::move(e)), line(l), col(c) {}
};

// ─── Statements ───────────────────────────────────────

// Declaração de variável
struct VarDeclNode : ASTNode {
    DataType type;
    std::string name;
    NodePtr init; // valor inicial (pode ser nullptr)
    int line, col;
    VarDeclNode(DataType t, const std::string& n, NodePtr i, int l = 0, int c = 0)
        : type(t), name(n), init(std::move(i)), line(l), col(c) {}
};
// Referência a variável: a, b, x
struct VarNode : ASTNode {
    std::string name;
    int line, col;
    VarNode(const std::string& n, int l, int c) : name(n), line(l), col(c) {}
};
// Return: 
struct ReturnNode : ASTNode {
    NodePtr expr;
    ReturnNode(NodePtr e) : expr(std::move(e)) {}
};

// Print
struct PrintNode : ASTNode {
    NodePtr expr;
    PrintNode(NodePtr e) : expr(std::move(e)) {}
};

// If/else: if(cond) then { ... } else { ... }
struct IfNode : ASTNode {
    NodePtr condition;
    std::vector<NodePtr> thenBlock;
    std::vector<NodePtr> elseBlock;
    IfNode(NodePtr c, std::vector<NodePtr> t, std::vector<NodePtr> e)
        : condition(std::move(c)), thenBlock(std::move(t)), elseBlock(std::move(e)) {}
};

// While: while(cond) { ... }
struct WhileNode : ASTNode {
    NodePtr condition;
    std::vector<NodePtr> body;
    WhileNode(NodePtr c, std::vector<NodePtr> b)
        : condition(std::move(c)), body(std::move(b)) {}
};

// For: for(init; cond; step) { ... }
// init: VarDeclNode ou VarAssignNode
// step: VarAssignNode (i++, i--, i += x, i = expr)
struct ForNode : ASTNode {
    NodePtr init;
    NodePtr condition;
    NodePtr step;
    std::vector<NodePtr> body;
    ForNode(NodePtr i, NodePtr c, NodePtr s, std::vector<NodePtr> b)
        : init(std::move(i)), condition(std::move(c)), step(std::move(s)), body(std::move(b)) {}
};

// Atribuição de variável: i = expr  |  i += expr  |  i -= expr  |  i *= expr  |  i /= expr  |  i++  |  i--
// op: "=" | "+=" | "-=" | "*=" | "/=" | "++" | "--"
// Para "++" e "--", expr é nullptr.
struct VarAssignNode : ASTNode {
    std::string name;
    std::string op;
    NodePtr expr; // nullptr para ++ e --
    int line, col;
    VarAssignNode(const std::string& n, const std::string& o, NodePtr e, int l, int c)
        : name(n), op(o), expr(std::move(e)), line(l), col(c) {}
};

// ─── Declaração de função ─────────────────────────────

// Parâmetro de função — pode ser tipo primitivo OU tipo struct
struct ParamNode {
    DataType type;
    std::string name;
    // Se type == DataType::Custom, structTypeName indica o tipo struct
    std::string structTypeName; // ex: "Point"
};

struct FunctionNode : ASTNode {
    DataType returnType;
    std::string returnStructType; // preenchido se returnType == DataType::Custom
    std::string name;
    std::vector<ParamNode> params;
    std::vector<NodePtr> body;
    bool isVariadic; // true se tem ... como último parâmetro
    FunctionNode(DataType rt, const std::string& rst, const std::string& n,
                 std::vector<ParamNode> p, std::vector<NodePtr> b,
                 bool variadic = false)
        : returnType(rt), returnStructType(rst), name(n), params(std::move(p)),
          body(std::move(b)), isVariadic(variadic) {}
    // Construtor legado sem returnStructType
    FunctionNode(DataType rt, const std::string& n,
                 std::vector<ParamNode> p, std::vector<NodePtr> b,
                 bool variadic = false)
        : returnType(rt), returnStructType(""), name(n), params(std::move(p)),
          body(std::move(b)), isVariadic(variadic) {}
};

// Array: int nums[5]; ou int nums[5] = {1,2,3,4,5};
struct ArrayDeclNode : ASTNode {
    DataType type;
    std::string name;
    int size;
    std::vector<NodePtr> init; // pode ser vazio
    std::string structTypeName; // preenchido se type == DataType::Custom (ex: "Point", "ns::Point")
    int line, col;
    ArrayDeclNode(DataType t, const std::string& n, int s,
                  std::vector<NodePtr> i, int l, int c)
        : type(t), name(n), size(s), init(std::move(i)), line(l), col(c) {}
    ArrayDeclNode(DataType t, const std::string& stn, const std::string& n, int s,
                  std::vector<NodePtr> i, int l, int c)
        : type(t), name(n), size(s), init(std::move(i)), structTypeName(stn), line(l), col(c) {}
};

// Atribuição de campo em elemento de array de struct: arr[i].field = expr;
struct ArrayFieldAssignNode : ASTNode {
    std::string arrayName;
    NodePtr index;
    std::string fieldName;
    NodePtr value;
    int line, col;
    ArrayFieldAssignNode(const std::string& a, NodePtr i, const std::string& f,
                         NodePtr v, int l, int c)
        : arrayName(a), index(std::move(i)), fieldName(f), value(std::move(v)), line(l), col(c) {}
};

// Atribuição de struct em elemento de array: arr[i] = func(...) ou arr[i] = outraVar
struct ArrayStructAssignNode : ASTNode {
    std::string arrayName;
    NodePtr index;
    std::string funcName;        // nome da função que retorna struct (ou "@copy:varName")
    std::vector<NodePtr> args;   // args da chamada (vazio se @copy)
    int line, col;
    ArrayStructAssignNode(const std::string& a, NodePtr i, const std::string& fn,
                          std::vector<NodePtr> fargs, int l, int c)
        : arrayName(a), index(std::move(i)), funcName(fn), args(std::move(fargs)), line(l), col(c) {}
};

// Leitura: nums[expr]
struct ArrayAccessNode : ASTNode {
    std::string name;
    NodePtr index;
    int line, col;
    ArrayAccessNode(const std::string& n, NodePtr i, int l, int c)
        : name(n), index(std::move(i)), line(l), col(c) {}
};

// Atribuição: nums[expr] = expr;
struct ArrayAssignNode : ASTNode {
    std::string name;
    NodePtr index;
    NodePtr value;
    int line, col;
    ArrayAssignNode(const std::string& n, NodePtr i, NodePtr v, int l, int c)
        : name(n), index(std::move(i)), value(std::move(v)), line(l), col(c) {}
};

// ─── Structs ──────────────────────────────────────────

struct StructField {
    DataType type;
    std::string name;
};

// Metodo dentro de struct
struct StructMethod {
    DataType returnType;
    std::string returnStructType; // preenchido se returnType == DataType::Custom
    std::string name;
    std::vector<ParamNode> params;
    std::vector<NodePtr> body;
};

// Definicao: struct Point { int x; int y; void move(int dx) { ... } }
struct StructDefNode : ASTNode {
    std::string name;
    std::vector<StructField> fields;
    std::vector<StructMethod> methods;
    int line, col;
    StructDefNode(const std::string& n, std::vector<StructField> f,
                  std::vector<StructMethod> m, int l, int c)
        : name(n), fields(std::move(f)), methods(std::move(m)), line(l), col(c) {}
};

// Declaracao de variavel do tipo struct: Point p;
// Também usado para: Point p = outraFuncao();  (via initCall)
struct StructVarDeclNode : ASTNode {
    std::string typeName;
    std::string varName;
    // Inicialização via chamada de função que retorna struct
    // Se não vazio, varName é inicializado pelo retorno da função
    std::string initFuncName;           // nome da função que retorna struct
    std::vector<NodePtr> initFuncArgs;  // args da chamada
    int line, col;
    StructVarDeclNode(const std::string& t, const std::string& v, int l, int c)
        : typeName(t), varName(v), line(l), col(c) {}
    StructVarDeclNode(const std::string& t, const std::string& v,
                      const std::string& fn, std::vector<NodePtr> fa, int l, int c)
        : typeName(t), varName(v), initFuncName(fn), initFuncArgs(std::move(fa)), line(l), col(c) {}
};

// Atribuição de struct completo: p = outraVar; ou p = funcao();
struct StructAssignNode : ASTNode {
    std::string varName;
    // Atribuição por variável: srcVar != ""
    std::string srcVar;
    // Atribuição por chamada de função: funcName != ""
    std::string funcName;
    std::vector<NodePtr> funcArgs;
    int line, col;
    // p = outraVar;
    StructAssignNode(const std::string& v, const std::string& src, int l, int c)
        : varName(v), srcVar(src), line(l), col(c) {}
    // p = funcao(...);
    StructAssignNode(const std::string& v, const std::string& fn,
                     std::vector<NodePtr> fa, int l, int c)
        : varName(v), srcVar(""), funcName(fn), funcArgs(std::move(fa)), line(l), col(c) {}
};

// Acesso de campo: p.x
struct FieldAccessNode : ASTNode {
    std::string varName;
    std::string fieldName;
    int line, col;
    FieldAccessNode(const std::string& v, const std::string& f, int l, int c)
        : varName(v), fieldName(f), line(l), col(c) {}
};

// Atribuicao de campo: p.x = expr;
struct FieldAssignNode : ASTNode {
    std::string varName;
    std::string fieldName;
    NodePtr value;
    int line, col;
    FieldAssignNode(const std::string& v, const std::string& f, NodePtr e, int l, int c)
        : varName(v), fieldName(f), value(std::move(e)), line(l), col(c) {}
};

// Chamada de metodo: p.move(10)
struct MethodCallNode : ASTNode {
    std::string varName;
    std::string methodName;
    std::vector<NodePtr> args;
    int line, col;
    MethodCallNode(const std::string& v, const std::string& m,
                   std::vector<NodePtr> a, int l, int c)
        : varName(v), methodName(m), args(std::move(a)), line(l), col(c) {}
};

// ─── Forward declaration de função (vinda de .nh) ────────────────────────────
// Apenas assinatura — sem corpo. Usado para registrar funções de outros arquivos.
struct FuncDeclNode : ASTNode {
    DataType returnType;
    std::string returnStructType; // preenchido se retorna struct
    std::string name;
    std::vector<ParamNode> params;
    bool isVariadic;
    FuncDeclNode(DataType rt, const std::string& n, std::vector<ParamNode> p,
                 bool variadic = false)
        : returnType(rt), returnStructType(""), name(n), params(std::move(p)),
          isVariadic(variadic) {}
    FuncDeclNode(DataType rt, const std::string& rst, const std::string& n,
                 std::vector<ParamNode> p, bool variadic = false)
        : returnType(rt), returnStructType(rst), name(n), params(std::move(p)),
          isVariadic(variadic) {}
};

// Parâmetro de tipo struct para FuncDeclNode: usado quando o tipo é um IDENT (ex: Point)
// Guardamos o nome do tipo struct como string separada
struct StructParamInfo {
    std::string structTypeName; // ex: "Point"
    std::string paramName;
};

// FuncDeclNode extendido — suporta parâmetros de tipo struct
struct FuncDeclNodeEx : ASTNode {
    DataType returnType;
    std::string returnStructType; // preenchido se returnType == Void e retorna struct
    std::string name;
    std::vector<ParamNode>      primitiveParams; // params de tipo primitivo
    std::vector<StructParamInfo> structParams;   // params de tipo struct
    // Lista ordenada: true = primitivo (índice em primitiveParams), false = struct (índice em structParams)
    std::vector<std::pair<bool,int>> paramOrder;
    FuncDeclNodeEx(DataType rt, const std::string& rst, const std::string& n)
        : returnType(rt), returnStructType(rst), name(n) {}
};

// ─── Inline assembly / IR ─────────────────────────────────────────────────────

// asm { "código" } — inline assembly x86_64 AT&T syntax
// Variáveis Nova podem ser referenciadas com $nome dentro do código.
// O compilador as substitui pelos valores LLVM corretos via constraints.
struct AsmNode : ASTNode {
    std::string code;            // código asm com $var para referências
    std::string constraints;     // constraints GCC-style (geradas automaticamente)
    std::vector<std::string> capturedVars; // nomes das variáveis Nova usadas
    int line, col;
    AsmNode(const std::string& c, const std::string& cst,
            const std::vector<std::string>& cv, int l, int co)
        : code(c), constraints(cst), capturedVars(cv), line(l), col(co) {}
};

// ir { "código LLVM IR" } — LLVM IR inline com captura de variáveis
// Variáveis Nova são referenciadas como $nome no texto IR.
// O compilador substitui $nome pelo %nome_llvm correto em tempo de compilação.
// Formato: ir { call i32 (i8*, ...) @printf(i8* $fmt_ptr, i32 $n) }
struct IrNode : ASTNode {
    std::string code;            // IR com $var para referências a variáveis Nova
    std::vector<std::string> capturedVars; // nomes extraídos automaticamente
    int line, col;
    IrNode(const std::string& c, const std::vector<std::string>& cv, int l, int co)
        : code(c), capturedVars(cv), line(l), col(co) {}
};

// ─── Programa inteiro ─────────────────────────────────

struct ProgramNode {
    std::vector<NodePtr> declarations;
};