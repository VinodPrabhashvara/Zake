#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "zake/ast.hpp"
#include "zake/value.hpp"

namespace zake {

class Interpreter {
public:
    explicit Interpreter(std::filesystem::path entryFile);
    Interpreter(std::filesystem::path entryFile, std::filesystem::path projectRoot, std::filesystem::path stdlibRoot);

    void executeFile(const std::filesystem::path& path);
    void execute(const std::vector<std::unique_ptr<Stmt>>& statements);
    void executeInteractive(const std::vector<std::unique_ptr<Stmt>>& statements);
    std::vector<std::pair<std::string, std::string>> globalsSnapshot() const;

private:
    void executeStatement(const Stmt& statement);
    void executeImport(const ImportStmt& statement);
    void executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements);
    void executeCatchBlock(const TryCatchStmt& statement, Value errorValue);
    void executeModuleInGlobalScope(const std::filesystem::path& path, const SourceLocation& importLocation);
    const std::unordered_map<std::string, Value>& loadModuleExports(const std::filesystem::path& path, const SourceLocation& importLocation);
    void defineImportedName(const std::string& name, const Value& value, const SourceLocation& location);
    bool nameExists(const std::string& name) const;
    static Value makeNamespaceValue(const std::unordered_map<std::string, Value>& exports);
    std::filesystem::path resolveImportPath(const ImportStmt& statement) const;
    std::filesystem::path findProjectRoot(const std::filesystem::path& start) const;
    static std::string readFile(const std::filesystem::path& path);
    Value callFunction(const FunctionValue& function, const std::vector<Value>& arguments, const SourceLocation& location);
    Value evaluate(const Expr& expression);
    Value evaluateLiteral(const LiteralExpr& expression);
    Value interpolateString(const std::string& text, const SourceLocation& location);
    Value evaluateInterpolationExpression(const std::string& source, const SourceLocation& location);
    Value evaluateVariable(const VariableExpr& expression);
    Value evaluateCall(const CallExpr& expression);
    Value evaluateArray(const ArrayExpr& expression);
    Value evaluateMap(const MapExpr& expression);
    Value evaluateIndex(const IndexExpr& expression);
    Value evaluateMember(const MemberExpr& expression);
    Value evaluateUnary(const UnaryExpr& expression);
    Value evaluateBinary(const BinaryExpr& expression);
    Value evaluateGrouping(const GroupingExpr& expression);
    Value lookupVariable(const std::string& name, const SourceLocation& location) const;
    void assignOrDefine(const std::string& name, Value value);
    void assignIndex(const IndexAssignStmt& statement);
    Value callBuiltin(const std::string& name, const std::vector<Value>& arguments, const SourceLocation& location);
    bool isBuiltinName(const std::string& name) const;
    static std::size_t requireIndex(const Value& value, std::size_t size, const SourceLocation& location);
    static std::string requireMapKey(const Value& value, const SourceLocation& location);
    static bool isTruthy(const Value& value, const SourceLocation& location);
    static bool valuesEqual(const Value& left, const Value& right);
    static void requireNumber(const Value& value, const SourceLocation& location, const std::string& context);
    static void requireNumberPair(const Value& left, const Value& right, const SourceLocation& location);

    std::vector<std::unordered_map<std::string, Value>> scopes_ = {{}};
    std::vector<std::filesystem::path> file_stack_;
    std::filesystem::path project_root_;
    std::filesystem::path stdlib_root_;
    std::unordered_set<std::string> imported_files_;
    std::unordered_set<std::string> importing_files_;
    std::unordered_map<std::string, std::unordered_map<std::string, Value>> module_exports_;
    std::vector<std::vector<std::unique_ptr<Stmt>>> loaded_modules_;
    int call_depth_ = 0;
};

} // namespace zake
