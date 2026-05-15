#include "zake/interpreter.hpp"

#include <iostream>

namespace zake {

void Interpreter::execute(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& statement : statements) {
        executeStatement(*statement);
    }
}

void Interpreter::executeStatement(const Stmt& statement) {
    if (const auto* print = dynamic_cast<const PrintStmt*>(&statement)) {
        const Value value = evaluate(print->expression());
        std::cout << value.toString() << '\n';
        return;
    }

    if (const auto* let = dynamic_cast<const LetStmt*>(&statement)) {
        environment_[let->name()] = evaluate(let->initializer());
        return;
    }

    throw RuntimeError("Unsupported statement.", statement.location());
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
    const auto found = environment_.find(expression.name());
    if (found == environment_.end()) {
        throw RuntimeError("Unknown variable '" + expression.name() + "'.", expression.location());
    }
    return found->second;
}

Value Interpreter::evaluateUnary(const UnaryExpr& expression) {
    const Value right = evaluate(expression.right());

    switch (expression.op()) {
    case TokenType::Minus:
        requireNumber(right, expression.location(), "Unary '-' requires a number operand.");
        return Value::number(-right.asNumber());
    default:
        break;
    }

    throw RuntimeError("Unsupported unary operator.", expression.location());
}

Value Interpreter::evaluateBinary(const BinaryExpr& expression) {
    const Value left = evaluate(expression.left());
    const Value right = evaluate(expression.right());

    requireNumberPair(left, right, expression.location());

    switch (expression.op()) {
    case TokenType::Plus:
        return Value::number(left.asNumber() + right.asNumber());
    case TokenType::Minus:
        return Value::number(left.asNumber() - right.asNumber());
    case TokenType::Star:
        return Value::number(left.asNumber() * right.asNumber());
    case TokenType::Slash:
        if (right.asNumber() == 0.0) {
            throw RuntimeError("Division by zero.", expression.location());
        }
        return Value::number(left.asNumber() / right.asNumber());
    default:
        break;
    }

    throw RuntimeError("Unsupported binary operator.", expression.location());
}

Value Interpreter::evaluateGrouping(const GroupingExpr& expression) {
    return evaluate(expression.expression());
}

void Interpreter::requireNumber(const Value& value, const SourceLocation& location, const std::string& context) {
    if (!value.isNumber()) {
        throw RuntimeError(context + " Got " + value.typeName() + ".", location);
    }
}

void Interpreter::requireNumberPair(const Value& left, const Value& right, const SourceLocation& location) {
    if (!left.isNumber() || !right.isNumber()) {
        throw RuntimeError(
            "Arithmetic operators require number operands. Got " + left.typeName() + " and " + right.typeName() + ".",
            location);
    }
}

} // namespace zake
