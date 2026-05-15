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
    if (match(TokenType::If)) {
        return ifStatement();
    }

    if (match(TokenType::While)) {
        return whileStatement();
    }

    if (match(TokenType::Print)) {
        return printStatement();
    }

    if (match(TokenType::Let)) {
        return letStatement();
    }

    if (match(TokenType::LeftBrace)) {
        return blockStatement(previous());
    }

    throw ParseError("Expected a statement starting with 'if', 'while', 'print', 'let', or '{'.", peek().location);
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

std::unique_ptr<Stmt> Parser::ifStatement() {
    const Token keyword = previous();
    auto condition = expression();
    const Token leftBrace = consume(TokenType::LeftBrace, "Expected '{' after if condition.");
    auto thenBranch = blockStatement(leftBrace);
    std::unique_ptr<Stmt> elseBranch;

    const std::size_t afterThenBranch = current_;
    skipNewlines();

    if (match(TokenType::Else)) {
        const Token elseKeyword = previous();
        const Token elseLeftBrace = consume(TokenType::LeftBrace, "Expected '{' after 'else'.");
        elseBranch = blockStatement(elseLeftBrace);

        if (elseBranch == nullptr) {
            throw ParseError("Expected a block after 'else'.", elseKeyword.location);
        }
    } else {
        current_ = afterThenBranch;
    }

    return std::make_unique<IfStmt>(keyword.location, std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::whileStatement() {
    const Token keyword = previous();
    auto condition = expression();
    const Token leftBrace = consume(TokenType::LeftBrace, "Expected '{' after while condition.");
    auto body = blockStatement(leftBrace);
    return std::make_unique<WhileStmt>(keyword.location, std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::blockStatement(const Token& leftBrace) {
    return std::make_unique<BlockStmt>(leftBrace.location, blockBody());
}

std::vector<std::unique_ptr<Stmt>> Parser::blockBody() {
    std::vector<std::unique_ptr<Stmt>> statements;

    skipNewlines();

    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        statements.push_back(statement());

        if (match(TokenType::Newline)) {
            skipNewlines();
            continue;
        }

        if (!check(TokenType::RightBrace) && !isAtEnd()) {
            throw ParseError("Expected end of line or '}' after statement.", peek().location);
        }
    }

    consume(TokenType::RightBrace, "Expected '}' after block.");
    return statements;
}

std::unique_ptr<Expr> Parser::expression() {
    return logicOr();
}

std::unique_ptr<Expr> Parser::logicOr() {
    auto expr = logicAnd();

    while (match(TokenType::Or)) {
        const Token op = previous();
        auto right = logicAnd();
        expr = std::make_unique<BinaryExpr>(op.location, std::move(expr), op.type, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::logicAnd() {
    auto expr = equality();

    while (match(TokenType::And)) {
        const Token op = previous();
        auto right = equality();
        expr = std::make_unique<BinaryExpr>(op.location, std::move(expr), op.type, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();

    while (match(TokenType::EqualEqual) || match(TokenType::BangEqual)) {
        const Token op = previous();
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(op.location, std::move(expr), op.type, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = term();

    while (match(TokenType::Greater) || match(TokenType::GreaterEqual) || match(TokenType::Less) || match(TokenType::LessEqual)) {
        const Token op = previous();
        auto right = term();
        expr = std::make_unique<BinaryExpr>(op.location, std::move(expr), op.type, std::move(right));
    }

    return expr;
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
    if (match(TokenType::Minus) || match(TokenType::Not) || match(TokenType::Bang)) {
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
