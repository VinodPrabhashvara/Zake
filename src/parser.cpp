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

std::unique_ptr<Expr> Parser::parseExpressionOnly() {
    skipNewlines();
    auto value = expression();
    skipNewlines();
    if (!isAtEnd()) {
        throw ParseError("Expected end of interpolation expression.", peek().location);
    }
    return value;
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
    if (match(TokenType::Fn)) {
        return functionStatement();
    }

    if (match(TokenType::If)) {
        return ifStatement();
    }

    if (match(TokenType::While)) {
        return whileStatement();
    }

    if (match(TokenType::Try)) {
        return tryCatchStatement();
    }

    if (match(TokenType::Print)) {
        return printStatement();
    }

    if (match(TokenType::Return)) {
        return returnStatement();
    }

    if (match(TokenType::Throw)) {
        return throwStatement();
    }

    if (match(TokenType::Let)) {
        return letStatement();
    }

    if (match(TokenType::From)) {
        return fromImportStatement();
    }

    if (match(TokenType::Import)) {
        return importStatement();
    }

    if (match(TokenType::LeftBrace)) {
        return blockStatement(previous());
    }

    return expressionStatement();
}

std::unique_ptr<Stmt> Parser::functionStatement() {
    const Token keyword = previous();
    const Token name = consume(TokenType::Identifier, "Expected a function name after 'fn'.");
    consume(TokenType::LeftParen, "Expected '(' after function name.");

    std::vector<std::string> parameters;
    if (!check(TokenType::RightParen)) {
        do {
            const Token parameter = consume(TokenType::Identifier, "Expected parameter name.");
            parameters.push_back(parameter.lexeme);
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RightParen, "Expected ')' after function parameters.");
    consume(TokenType::LeftBrace, "Expected '{' before function body.");
    return std::make_unique<FunctionStmt>(keyword.location, name.lexeme, std::move(parameters), blockBody());
}

std::unique_ptr<Stmt> Parser::printStatement() {
    const Token keyword = previous();
    consume(TokenType::LeftParen, "Expected '(' after 'print'.");
    auto value = expression();
    consume(TokenType::RightParen, "Expected ')' after expression.");
    return std::make_unique<PrintStmt>(keyword.location, std::move(value));
}

std::unique_ptr<Stmt> Parser::expressionStatement() {
    auto value = expression();
    if (match(TokenType::Equal)) {
        return indexAssignmentStatement(std::move(value));
    }
    return std::make_unique<ExpressionStmt>(value->location(), std::move(value));
}

std::unique_ptr<Stmt> Parser::indexAssignmentStatement(std::unique_ptr<Expr> target) {
    const Token equal = previous();
    auto value = expression();

    auto* indexTarget = dynamic_cast<IndexExpr*>(target.get());
    if (indexTarget == nullptr) {
        throw ParseError("Only index assignment is supported. Use target[index] = value.", equal.location);
    }

    return std::make_unique<IndexAssignStmt>(
        equal.location,
        indexTarget->takeObject(),
        indexTarget->takeIndex(),
        std::move(value));
}

std::unique_ptr<Stmt> Parser::importStatement() {
    const Token keyword = previous();

    if (match(TokenType::String)) {
        const std::string module = previous().lexeme;
        std::string alias;
        if (match(TokenType::As)) {
            const Token aliasName = consume(TokenType::Identifier, "Expected an alias name after 'as'.");
            alias = aliasName.lexeme;
        }
        return std::make_unique<ImportStmt>(keyword.location, module, false, alias);
    }

    const Token root = consume(TokenType::Identifier, "Expected a module path after 'import'.");
    std::string module = standardModulePath(root);
    std::string alias;
    if (match(TokenType::As)) {
        const Token aliasName = consume(TokenType::Identifier, "Expected an alias name after 'as'.");
        alias = aliasName.lexeme;
    }

    return std::make_unique<ImportStmt>(keyword.location, module, true, alias);
}

std::unique_ptr<Stmt> Parser::fromImportStatement() {
    const Token keyword = previous();
    const Token root = consume(TokenType::Identifier, "Expected a module path after 'from'.");
    std::string module = standardModulePath(root);

    consume(TokenType::Import, "Expected 'import' after module path.");

    std::vector<std::string> symbols;
    do {
        const Token symbol = consume(TokenType::Identifier, "Expected an imported symbol name.");
        symbols.push_back(symbol.lexeme);
    } while (match(TokenType::Comma));

    return std::make_unique<ImportStmt>(keyword.location, module, true, "", std::move(symbols));
}

std::string Parser::standardModulePath(const Token& root) {
    std::string module = root.lexeme;
    while (match(TokenType::Dot)) {
        const Token part = consume(TokenType::Identifier, "Expected module name after '.'.");
        module += "." + part.lexeme;
    }

    if (module.rfind("std.", 0) != 0) {
        throw ParseError("Only quoted local imports or std.* imports are supported.", root.location);
    }

    return module;
}

std::unique_ptr<Stmt> Parser::returnStatement() {
    const Token keyword = previous();
    auto value = expression();
    return std::make_unique<ReturnStmt>(keyword.location, std::move(value));
}

std::unique_ptr<Stmt> Parser::throwStatement() {
    const Token keyword = previous();
    auto value = expression();
    return std::make_unique<ThrowStmt>(keyword.location, std::move(value));
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

std::unique_ptr<Stmt> Parser::tryCatchStatement() {
    const Token keyword = previous();
    consume(TokenType::LeftBrace, "Expected '{' after 'try'.");
    auto tryBody = blockBody();

    skipNewlines();
    consume(TokenType::Catch, "Expected 'catch' after try block.");
    const Token catchName = consume(TokenType::Identifier, "Expected catch variable name after 'catch'.");
    consume(TokenType::LeftBrace, "Expected '{' after catch variable.");
    auto catchBody = blockBody();

    return std::make_unique<TryCatchStmt>(
        keyword.location,
        std::move(tryBody),
        catchName.lexeme,
        std::move(catchBody));
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

    return call();
}

std::unique_ptr<Expr> Parser::call() {
    auto expr = primary();

    while (true) {
        if (match(TokenType::LeftParen)) {
            expr = finishCall(std::move(expr), previous());
            continue;
        }

        if (match(TokenType::LeftBracket)) {
            expr = finishIndex(std::move(expr), previous());
            continue;
        }

        if (match(TokenType::Dot)) {
            const Token member = consume(TokenType::Identifier, "Expected member name after '.'.");
            expr = std::make_unique<MemberExpr>(member.location, std::move(expr), member.lexeme);
            continue;
        }

        break;
    }

    return expr;
}

std::unique_ptr<Expr> Parser::finishCall(std::unique_ptr<Expr> callee, const Token& leftParen) {
    std::vector<std::unique_ptr<Expr>> arguments;

    if (!check(TokenType::RightParen)) {
        do {
            arguments.push_back(expression());
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RightParen, "Expected ')' after function call arguments.");
    return std::make_unique<CallExpr>(leftParen.location, std::move(callee), std::move(arguments));
}

std::unique_ptr<Expr> Parser::finishIndex(std::unique_ptr<Expr> object, const Token& leftBracket) {
    auto index = expression();
    consume(TokenType::RightBracket, "Expected ']' after index.");
    return std::make_unique<IndexExpr>(leftBracket.location, std::move(object), std::move(index));
}

std::unique_ptr<Expr> Parser::mapLiteral(const Token& leftBrace) {
    std::vector<MapEntry> entries;

    skipNewlines();

    if (!check(TokenType::RightBrace)) {
        do {
            skipNewlines();
            const Token key = consume(TokenType::String, "Expected a string key in map literal.");
            consume(TokenType::Colon, "Expected ':' after map key.");
            auto value = expression();
            entries.push_back(MapEntry {key.lexeme, key.location, std::move(value)});
            skipNewlines();
        } while (match(TokenType::Comma));
    }

    skipNewlines();
    consume(TokenType::RightBrace, "Expected '}' after map literal.");
    return std::make_unique<MapExpr>(leftBrace.location, std::move(entries));
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

    if (match(TokenType::LeftBracket)) {
        const Token leftBracket = previous();
        std::vector<std::unique_ptr<Expr>> elements;

        if (!check(TokenType::RightBracket)) {
            do {
                elements.push_back(expression());
            } while (match(TokenType::Comma));
        }

        consume(TokenType::RightBracket, "Expected ']' after array literal.");
        return std::make_unique<ArrayExpr>(leftBracket.location, std::move(elements));
    }

    if (match(TokenType::LeftBrace)) {
        return mapLiteral(previous());
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
