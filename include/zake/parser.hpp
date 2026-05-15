#pragma once

#include <memory>
#include <vector>

#include "zake/ast.hpp"
#include "zake/token.hpp"

namespace zake {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::vector<std::unique_ptr<Stmt>> parse();

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
    std::unique_ptr<Stmt> printStatement();
    std::unique_ptr<Stmt> letStatement();
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> primary();

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
};

} // namespace zake
