#pragma once
#include <string>
#include <memory>
#include <vector>

//tipos de dados
enum class DataType { Int, Float, String, Void };

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

//float
struct FloatLitNode : ASTNode{
    float value;
    FloatLitNode(float v) : value(v) {}
};

//string
struct StringLitNode: ASTNode{
    std::string value;
    StringLitNode(const std::string& v) : value(v) {}
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

struct ParamNode {
    DataType type;
    std::string name;
};

struct FunctionNode : ASTNode {
    DataType returnType;
    std::string name;
    std::vector<ParamNode> params;
    std::vector<NodePtr> body;
    FunctionNode(DataType rt, const std::string& n,
                 std::vector<ParamNode> p, std::vector<NodePtr> b)
        : returnType(rt), name(n), params(std::move(p)), body(std::move(b)) {}
};

// Array: int nums[5]; ou int nums[5] = {1,2,3,4,5};
struct ArrayDeclNode : ASTNode {
    DataType type;
    std::string name;
    int size;
    std::vector<NodePtr> init; // pode ser vazio
    int line, col;
    ArrayDeclNode(DataType t, const std::string& n, int s,
                  std::vector<NodePtr> i, int l, int c)
        : type(t), name(n), size(s), init(std::move(i)), line(l), col(c) {}
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
struct StructVarDeclNode : ASTNode {
    std::string typeName;
    std::string varName;
    int line, col;
    StructVarDeclNode(const std::string& t, const std::string& v, int l, int c)
        : typeName(t), varName(v), line(l), col(c) {}
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

// ─── Programa inteiro ─────────────────────────────────

struct ProgramNode {
    std::vector<NodePtr> declarations;
};