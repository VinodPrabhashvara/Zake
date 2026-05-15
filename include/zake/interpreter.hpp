#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "zake/ast.hpp"
#include "zake/value.hpp"

namespace zake {

class Interpreter {
public:
    void execute(const std::vector<std::unique_ptr<Stmt>>& statements);

private:
    void executeStatement(const Stmt& statement);
    Value evaluate(const Expr& expression);
    Value evaluateLiteral(const LiteralExpr& expression);
    Value evaluateVariable(const VariableExpr& expression);
    Value evaluateUnary(const UnaryExpr& expression);
    Value evaluateBinary(const BinaryExpr& expression);
    Value evaluateGrouping(const GroupingExpr& expression);
    static void requireNumber(const Value& value, const SourceLocation& location, const std::string& context);
    static void requireNumberPair(const Value& left, const Value& right, const SourceLocation& location);

    std::unordered_map<std::string, Value> environment_;
};

} // namespace zake
