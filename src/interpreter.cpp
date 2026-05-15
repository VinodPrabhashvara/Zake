#include "zake/interpreter.hpp"

#include <iostream>
#include <utility>

namespace zake {

void Interpreter::execute(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& statement : statements) {
        executeStatement(*statement);
    }
}

void Interpreter::executeStatement(const Stmt& statement) {
    if (const auto* block = dynamic_cast<const BlockStmt*>(&statement)) {
        executeBlock(block->statements());
        return;
    }

    if (const auto* ifStatement = dynamic_cast<const IfStmt*>(&statement)) {
        if (isTruthy(evaluate(ifStatement->condition()), ifStatement->condition().location())) {
            executeStatement(ifStatement->thenBranch());
        } else if (const Stmt* elseBranch = ifStatement->elseBranch()) {
            executeStatement(*elseBranch);
        }
        return;
    }

    if (const auto* whileStatement = dynamic_cast<const WhileStmt*>(&statement)) {
        while (isTruthy(evaluate(whileStatement->condition()), whileStatement->condition().location())) {
            executeStatement(whileStatement->body());
        }
        return;
    }

    if (const auto* print = dynamic_cast<const PrintStmt*>(&statement)) {
        const Value value = evaluate(print->expression());
        std::cout << value.toString() << '\n';
        return;
    }

    if (const auto* let = dynamic_cast<const LetStmt*>(&statement)) {
        assignOrDefine(let->name(), evaluate(let->initializer()));
        return;
    }

    throw RuntimeError("Unsupported statement.", statement.location());
}

void Interpreter::executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements) {
    scopes_.push_back({});

    try {
        for (const auto& statement : statements) {
            executeStatement(*statement);
        }
    } catch (...) {
        scopes_.pop_back();
        throw;
    }

    scopes_.pop_back();
}

Value Interpreter::evaluate(const Expr& expression) {
    if (const auto* literal = dynamic_cast<const LiteralExpr*>(&expression)) {
        return evaluateLiteral(*literal);
    }

    if (const auto* variable = dynamic_cast<const VariableExpr*>(&expression)) {
        return evaluateVariable(*variable);
    }

    if (const auto* unary = dynamic_cast<const UnaryExpr*>(&expression)) {
        return evaluateUnary(*unary);
    }

    if (const auto* binary = dynamic_cast<const BinaryExpr*>(&expression)) {
        return evaluateBinary(*binary);
    }

    if (const auto* grouping = dynamic_cast<const GroupingExpr*>(&expression)) {
        return evaluateGrouping(*grouping);
    }

    throw RuntimeError("Unsupported expression.", expression.location());
}

Value Interpreter::evaluateLiteral(const LiteralExpr& expression) {
    return expression.value();
}

Value Interpreter::evaluateVariable(const VariableExpr& expression) {
    return lookupVariable(expression.name(), expression.location());
}

Value Interpreter::evaluateUnary(const UnaryExpr& expression) {
    const Value right = evaluate(expression.right());

    switch (expression.op()) {
    case TokenType::Minus:
        requireNumber(right, expression.location(), "Unary '-' requires a number operand.");
        return Value::number(-right.asNumber());
    case TokenType::Not:
    case TokenType::Bang:
        return Value::boolean(!isTruthy(right, expression.location()));
    default:
        break;
    }

    throw RuntimeError("Unsupported unary operator.", expression.location());
}

Value Interpreter::evaluateBinary(const BinaryExpr& expression) {
    if (expression.op() == TokenType::Or) {
        const Value left = evaluate(expression.left());
        if (isTruthy(left, expression.location())) {
            return Value::boolean(true);
        }
        return Value::boolean(isTruthy(evaluate(expression.right()), expression.location()));
    }

    if (expression.op() == TokenType::And) {
        const Value left = evaluate(expression.left());
        if (!isTruthy(left, expression.location())) {
            return Value::boolean(false);
        }
        return Value::boolean(isTruthy(evaluate(expression.right()), expression.location()));
    }

    const Value left = evaluate(expression.left());
    const Value right = evaluate(expression.right());

    switch (expression.op()) {
    case TokenType::Plus:
        requireNumberPair(left, right, expression.location());
        return Value::number(left.asNumber() + right.asNumber());
    case TokenType::Minus:
        requireNumberPair(left, right, expression.location());
        return Value::number(left.asNumber() - right.asNumber());
    case TokenType::Star:
        requireNumberPair(left, right, expression.location());
        return Value::number(left.asNumber() * right.asNumber());
    case TokenType::Slash:
        requireNumberPair(left, right, expression.location());
        if (right.asNumber() == 0.0) {
            throw RuntimeError("Division by zero.", expression.location());
        }
        return Value::number(left.asNumber() / right.asNumber());
    case TokenType::Greater:
        requireNumberPair(left, right, expression.location());
        return Value::boolean(left.asNumber() > right.asNumber());
    case TokenType::GreaterEqual:
        requireNumberPair(left, right, expression.location());
        return Value::boolean(left.asNumber() >= right.asNumber());
    case TokenType::Less:
        requireNumberPair(left, right, expression.location());
        return Value::boolean(left.asNumber() < right.asNumber());
    case TokenType::LessEqual:
        requireNumberPair(left, right, expression.location());
        return Value::boolean(left.asNumber() <= right.asNumber());
    case TokenType::EqualEqual:
        return Value::boolean(valuesEqual(left, right));
    case TokenType::BangEqual:
        return Value::boolean(!valuesEqual(left, right));
    default:
        break;
    }

    throw RuntimeError("Unsupported binary operator.", expression.location());
}

Value Interpreter::evaluateGrouping(const GroupingExpr& expression) {
    return evaluate(expression.expression());
}

Value Interpreter::lookupVariable(const std::string& name, const SourceLocation& location) const {
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        const auto found = scope->find(name);
        if (found != scope->end()) {
            return found->second;
        }
    }

    throw RuntimeError("Unknown variable '" + name + "'. Define it with 'let " + name + " = ...' before using it.", location);
}

void Interpreter::assignOrDefine(const std::string& name, Value value) {
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        const auto found = scope->find(name);
        if (found != scope->end()) {
            found->second = std::move(value);
            return;
        }
    }

    scopes_.back()[name] = std::move(value);
}

bool Interpreter::isTruthy(const Value& value, const SourceLocation& location) {
    if (!value.isBoolean()) {
        throw RuntimeError("Condition must be boolean, got " + value.typeName() + ". Use a comparison such as ==, !=, >, >=, <, or <=.", location);
    }

    return value.asBoolean();
}

bool Interpreter::valuesEqual(const Value& left, const Value& right) {
    if (left.type() != right.type()) {
        return false;
    }

    if (left.isNumber()) {
        return left.asNumber() == right.asNumber();
    }

    if (left.isString()) {
        return left.asString() == right.asString();
    }

    if (left.isBoolean()) {
        return left.asBoolean() == right.asBoolean();
    }

    return true;
}

void Interpreter::requireNumber(const Value& value, const SourceLocation& location, const std::string& context) {
    if (!value.isNumber()) {
        throw RuntimeError(context + " Got " + value.typeName() + ".", location);
    }
}

void Interpreter::requireNumberPair(const Value& left, const Value& right, const SourceLocation& location) {
    if (!left.isNumber() || !right.isNumber()) {
        throw RuntimeError(
            "Arithmetic operators require number operands. Comparison operators also require numbers. Got " + left.typeName() + " and " + right.typeName() + ".",
            location);
    }
}

} // namespace zake
