#pragma once

#include <memory>
#include <string>
#include <vector>

#include "zake/token.hpp"
#include "zake/value.hpp"

namespace zake {

class Expr {
public:
    explicit Expr(SourceLocation location);
    virtual ~Expr();

    const SourceLocation& location() const;

private:
    SourceLocation location_;
};

class LiteralExpr final : public Expr {
public:
    LiteralExpr(SourceLocation location, Value value);

    const Value& value() const;

private:
    Value value_;
};

class VariableExpr final : public Expr {
public:
    VariableExpr(SourceLocation location, std::string name);

    const std::string& name() const;

private:
    std::string name_;
};

class CallExpr final : public Expr {
public:
    CallExpr(SourceLocation location, std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> arguments);

    const Expr& callee() const;
    const std::vector<std::unique_ptr<Expr>>& arguments() const;

private:
    std::unique_ptr<Expr> callee_;
    std::vector<std::unique_ptr<Expr>> arguments_;
};

class UnaryExpr final : public Expr {
public:
    UnaryExpr(SourceLocation location, TokenType op, std::unique_ptr<Expr> right);

    TokenType op() const;
    const Expr& right() const;

private:
    TokenType op_;
    std::unique_ptr<Expr> right_;
};

class BinaryExpr final : public Expr {
public:
    BinaryExpr(SourceLocation location, std::unique_ptr<Expr> left, TokenType op, std::unique_ptr<Expr> right);

    const Expr& left() const;
    TokenType op() const;
    const Expr& right() const;

private:
    std::unique_ptr<Expr> left_;
    TokenType op_;
    std::unique_ptr<Expr> right_;
};

class GroupingExpr final : public Expr {
public:
    GroupingExpr(SourceLocation location, std::unique_ptr<Expr> expression);

    const Expr& expression() const;

private:
    std::unique_ptr<Expr> expression_;
};

class Stmt {
public:
    explicit Stmt(SourceLocation location);
    virtual ~Stmt();

    const SourceLocation& location() const;

private:
    SourceLocation location_;
};

class PrintStmt final : public Stmt {
public:
    PrintStmt(SourceLocation location, std::unique_ptr<Expr> expression);

    const Expr& expression() const;

private:
    std::unique_ptr<Expr> expression_;
};

class ExpressionStmt final : public Stmt {
public:
    ExpressionStmt(SourceLocation location, std::unique_ptr<Expr> expression);

    const Expr& expression() const;

private:
    std::unique_ptr<Expr> expression_;
};

class LetStmt final : public Stmt {
public:
    LetStmt(SourceLocation location, std::string name, std::unique_ptr<Expr> initializer);

    const std::string& name() const;
    const Expr& initializer() const;

private:
    std::string name_;
    std::unique_ptr<Expr> initializer_;
};

class FunctionStmt final : public Stmt {
public:
    FunctionStmt(SourceLocation location, std::string name, std::vector<std::string> parameters, std::vector<std::unique_ptr<Stmt>> body);

    const std::string& name() const;
    const std::vector<std::string>& parameters() const;
    const std::vector<std::unique_ptr<Stmt>>& body() const;

private:
    std::string name_;
    std::vector<std::string> parameters_;
    std::vector<std::unique_ptr<Stmt>> body_;
};

class ReturnStmt final : public Stmt {
public:
    ReturnStmt(SourceLocation location, std::unique_ptr<Expr> value);

    const Expr& value() const;

private:
    std::unique_ptr<Expr> value_;
};

class BlockStmt final : public Stmt {
public:
    BlockStmt(SourceLocation location, std::vector<std::unique_ptr<Stmt>> statements);

    const std::vector<std::unique_ptr<Stmt>>& statements() const;

private:
    std::vector<std::unique_ptr<Stmt>> statements_;
};

class IfStmt final : public Stmt {
public:
    IfStmt(
        SourceLocation location,
        std::unique_ptr<Expr> condition,
        std::unique_ptr<Stmt> thenBranch,
        std::unique_ptr<Stmt> elseBranch);

    const Expr& condition() const;
    const Stmt& thenBranch() const;
    const Stmt* elseBranch() const;

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> thenBranch_;
    std::unique_ptr<Stmt> elseBranch_;
};

class WhileStmt final : public Stmt {
public:
    WhileStmt(SourceLocation location, std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body);

    const Expr& condition() const;
    const Stmt& body() const;

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> body_;
};

} // namespace zake
