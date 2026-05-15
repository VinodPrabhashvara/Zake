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
    void executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements);
    Value callFunction(const FunctionValue& function, const std::vector<Value>& arguments, const SourceLocation& location);
    Value evaluate(const Expr& expression);
    Value evaluateLiteral(const LiteralExpr& expression);
    Value evaluateVariable(const VariableExpr& expression);
    Value evaluateCall(const CallExpr& expression);
    Value evaluateUnary(const UnaryExpr& expression);
    Value evaluateBinary(const BinaryExpr& expression);
    Value evaluateGrouping(const GroupingExpr& expression);
    Value lookupVariable(const std::string& name, const SourceLocation& location) const;
    void assignOrDefine(const std::string& name, Value value);
    static bool isTruthy(const Value& value, const SourceLocation& location);
    static bool valuesEqual(const Value& left, const Value& right);
    static void requireNumber(const Value& value, const SourceLocation& location, const std::string& context);
    static void requireNumberPair(const Value& left, const Value& right, const SourceLocation& location);

    std::vector<std::unordered_map<std::string, Value>> scopes_ = {{}};
    int call_depth_ = 0;
};

} // namespace zake
