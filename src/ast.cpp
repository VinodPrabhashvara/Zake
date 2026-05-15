#include "zake/ast.hpp"

#include <utility>

namespace zake {

Expr::Expr(SourceLocation location)
    : location_(location) {}

Expr::~Expr() = default;

const SourceLocation& Expr::location() const {
    return location_;
}

LiteralExpr::LiteralExpr(SourceLocation location, Value value)
    : Expr(location), value_(std::move(value)) {}

const Value& LiteralExpr::value() const {
    return value_;
}

VariableExpr::VariableExpr(SourceLocation location, std::string name)
    : Expr(location), name_(std::move(name)) {}

const std::string& VariableExpr::name() const {
    return name_;
}

CallExpr::CallExpr(SourceLocation location, std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> arguments)
    : Expr(location), callee_(std::move(callee)), arguments_(std::move(arguments)) {}

const Expr& CallExpr::callee() const {
    return *callee_;
}

const std::vector<std::unique_ptr<Expr>>& CallExpr::arguments() const {
    return arguments_;
}

UnaryExpr::UnaryExpr(SourceLocation location, TokenType op, std::unique_ptr<Expr> right)
    : Expr(location), op_(op), right_(std::move(right)) {}

TokenType UnaryExpr::op() const {
    return op_;
}

const Expr& UnaryExpr::right() const {
    return *right_;
}

BinaryExpr::BinaryExpr(SourceLocation location, std::unique_ptr<Expr> left, TokenType op, std::unique_ptr<Expr> right)
    : Expr(location), left_(std::move(left)), op_(op), right_(std::move(right)) {}

const Expr& BinaryExpr::left() const {
    return *left_;
}

TokenType BinaryExpr::op() const {
    return op_;
}

const Expr& BinaryExpr::right() const {
    return *right_;
}

GroupingExpr::GroupingExpr(SourceLocation location, std::unique_ptr<Expr> expression)
    : Expr(location), expression_(std::move(expression)) {}

const Expr& GroupingExpr::expression() const {
    return *expression_;
}

Stmt::Stmt(SourceLocation location)
    : location_(location) {}

Stmt::~Stmt() = default;

const SourceLocation& Stmt::location() const {
    return location_;
}

PrintStmt::PrintStmt(SourceLocation location, std::unique_ptr<Expr> expression)
    : Stmt(location), expression_(std::move(expression)) {}

const Expr& PrintStmt::expression() const {
    return *expression_;
}

ExpressionStmt::ExpressionStmt(SourceLocation location, std::unique_ptr<Expr> expression)
    : Stmt(location), expression_(std::move(expression)) {}

const Expr& ExpressionStmt::expression() const {
    return *expression_;
}

LetStmt::LetStmt(SourceLocation location, std::string name, std::unique_ptr<Expr> initializer)
    : Stmt(location), name_(std::move(name)), initializer_(std::move(initializer)) {}

const std::string& LetStmt::name() const {
    return name_;
}

const Expr& LetStmt::initializer() const {
    return *initializer_;
}

FunctionStmt::FunctionStmt(SourceLocation location, std::string name, std::vector<std::string> parameters, std::vector<std::unique_ptr<Stmt>> body)
    : Stmt(location), name_(std::move(name)), parameters_(std::move(parameters)), body_(std::move(body)) {}

const std::string& FunctionStmt::name() const {
    return name_;
}

const std::vector<std::string>& FunctionStmt::parameters() const {
    return parameters_;
}

const std::vector<std::unique_ptr<Stmt>>& FunctionStmt::body() const {
    return body_;
}

ReturnStmt::ReturnStmt(SourceLocation location, std::unique_ptr<Expr> value)
    : Stmt(location), value_(std::move(value)) {}

const Expr& ReturnStmt::value() const {
    return *value_;
}

BlockStmt::BlockStmt(SourceLocation location, std::vector<std::unique_ptr<Stmt>> statements)
    : Stmt(location), statements_(std::move(statements)) {}

const std::vector<std::unique_ptr<Stmt>>& BlockStmt::statements() const {
    return statements_;
}

IfStmt::IfStmt(
    SourceLocation location,
    std::unique_ptr<Expr> condition,
    std::unique_ptr<Stmt> thenBranch,
    std::unique_ptr<Stmt> elseBranch)
    : Stmt(location),
      condition_(std::move(condition)),
      thenBranch_(std::move(thenBranch)),
      elseBranch_(std::move(elseBranch)) {}

const Expr& IfStmt::condition() const {
    return *condition_;
}

const Stmt& IfStmt::thenBranch() const {
    return *thenBranch_;
}

const Stmt* IfStmt::elseBranch() const {
    return elseBranch_.get();
}

WhileStmt::WhileStmt(SourceLocation location, std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
    : Stmt(location), condition_(std::move(condition)), body_(std::move(body)) {}

const Expr& WhileStmt::condition() const {
    return *condition_;
}

const Stmt& WhileStmt::body() const {
    return *body_;
}

} // namespace zake
