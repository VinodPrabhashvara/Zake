#include "zake/parser.hpp"

#include <utility>

namespace zake {

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;

    skipNewlines();

    while (!isAtEnd()) {
        statements.push_back(statement());

        if (match(TokenType::Newline)) {
            skipNewlines();
            continue;
        }

        if (!isAtEnd()) {
            throw ParseError("Expected end of line after statement.", peek().location);
        }
    }

    return statements;
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EndOfFile;
}

const Token& Parser::peek() const {
    return tokens_[current_];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        ++current_;
    }
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) {
        return type == TokenType::EndOfFile;
    }
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    throw ParseError(message, peek().location);
}

void Parser::skipNewlines() {
    while (match(TokenType::Newline)) {
    }
}

std::unique_ptr<Stmt> Parser::statement() {
    if (match(TokenType::Print)) {
        return printStatement();
    }

    if (match(TokenType::Let)) {
        return letStatement();
    }

    throw ParseError("Expected a statement starting with 'print' or 'let'.", peek().location);
}

std::unique_ptr<Stmt> Parser::printStatement() {
    const Token keyword = previous();
    consume(TokenType::LeftParen, "Expected '(' after 'print'.");
    auto value = expression();
    consume(TokenType::RightParen, "Expected ')' after expression.");
    return std::make_unique<PrintStmt>(keyword.location, std::move(value));
}

std::unique_ptr<Stmt> Parser::letStatement() {
    const Token keyword = previous();
    const Token name = consume(TokenType::Identifier, "Expected a variable name after 'let'.");
    consume(TokenType::Equal, "Expected '=' after variable name.");
    auto initializer = expression();
    return std::make_unique<LetStmt>(keyword.location, name.lexeme, std::move(initializer));
}

std::unique_ptr<Expr> Parser::expression() {
    return term();
}

std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();

    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        const Token op = previous();
        auto right = factor();
        expr = std::make_unique<BinaryExpr>(op.location, std::move(expr), op.type, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    auto expr = unary();

    while (match(TokenType::Star) || match(TokenType::Slash)) {
        const Token op = previous();
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(op.location, std::move(expr), op.type, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match(TokenType::Minus)) {
        const Token op = previous();
        auto right = unary();
        return std::make_unique<UnaryExpr>(op.location, op.type, std::move(right));
    }

    return primary();
}

std::unique_ptr<Expr> Parser::primary() {
    if (match(TokenType::Number)) {
        const Token token = previous();
        return std::make_unique<LiteralExpr>(token.location, Value::number(std::stod(token.lexeme)));
    }

    if (match(TokenType::String)) {
        const Token token = previous();
        return std::make_unique<LiteralExpr>(token.location, Value::string(token.lexeme));
    }

    if (match(TokenType::True)) {
        return std::make_unique<LiteralExpr>(previous().location, Value::boolean(true));
    }

    if (match(TokenType::False)) {
        return std::make_unique<LiteralExpr>(previous().location, Value::boolean(false));
    }

    if (match(TokenType::Identifier)) {
        const Token token = previous();
        return std::make_unique<VariableExpr>(token.location, token.lexeme);
    }

    if (match(TokenType::LeftParen)) {
        const Token leftParen = previous();
        auto expr = expression();
        consume(TokenType::RightParen, "Expected ')' after expression.");
        return std::make_unique<GroupingExpr>(leftParen.location, std::move(expr));
    }

    throw ParseError("Expected an expression.", peek().location);
}

} // namespace zake
