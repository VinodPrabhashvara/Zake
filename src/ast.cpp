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

LetStmt::LetStmt(SourceLocation location, std::string name, std::unique_ptr<Expr> initializer)
    : Stmt(location), name_(std::move(name)), initializer_(std::move(initializer)) {}

const std::string& LetStmt::name() const {
    return name_;
}

const Expr& LetStmt::initializer() const {
    return *initializer_;
}

} // namespace zake
