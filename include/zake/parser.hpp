#pragma once

#include <memory>
#include <string>
#include <vector>

#include "zake/ast.hpp"
#include "zake/token.hpp"

namespace zake {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::vector<std::unique_ptr<Stmt>> parse();
    std::unique_ptr<Expr> parseExpressionOnly();

private:
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& consume(TokenType type, const std::string& message);
    void skipNewlines();
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> functionStatement();
    std::unique_ptr<Stmt> printStatement();
    std::unique_ptr<Stmt> expressionStatement();
    std::unique_ptr<Stmt> indexAssignmentStatement(std::unique_ptr<Expr> target);
    std::unique_ptr<Stmt> importStatement();
    std::unique_ptr<Stmt> fromImportStatement();
    std::string standardModulePath(const Token& root);
    std::unique_ptr<Stmt> returnStatement();
    std::unique_ptr<Stmt> throwStatement();
    std::unique_ptr<Stmt> letStatement();
    std::unique_ptr<Stmt> ifStatement();
    std::unique_ptr<Stmt> whileStatement();
    std::unique_ptr<Stmt> tryCatchStatement();
    std::unique_ptr<Stmt> blockStatement(const Token& leftBrace);
    std::vector<std::unique_ptr<Stmt>> blockBody();
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> logicOr();
    std::unique_ptr<Expr> logicAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> call();
    std::unique_ptr<Expr> finishCall(std::unique_ptr<Expr> callee, const Token& leftParen);
    std::unique_ptr<Expr> finishIndex(std::unique_ptr<Expr> object, const Token& leftBracket);
    std::unique_ptr<Expr> mapLiteral(const Token& leftBrace);
    std::unique_ptr<Expr> primary();

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
};

} // namespace zake
