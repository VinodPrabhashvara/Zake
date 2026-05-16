#include "zake/linter.hpp"
#include "zake/lexer.hpp"
#include "zake/parser.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace zake {

void LintResult::addWarning(SourceLocation location, std::string message) {
    diagnostics_.push_back({LintSeverity::Warning, location, std::move(message)});
}

void LintResult::addError(SourceLocation location, std::string message) {
    diagnostics_.push_back({LintSeverity::Error, location, std::move(message)});
}

const std::vector<LintDiagnostic>& LintResult::diagnostics() const {
    return diagnostics_;
}

int LintResult::warningCount() const {
    return static_cast<int>(std::count_if(diagnostics_.begin(), diagnostics_.end(), [](const LintDiagnostic& diagnostic) {
        return diagnostic.severity == LintSeverity::Warning;
    }));
}

int LintResult::errorCount() const {
    return static_cast<int>(std::count_if(diagnostics_.begin(), diagnostics_.end(), [](const LintDiagnostic& diagnostic) {
        return diagnostic.severity == LintSeverity::Error;
    }));
}

const char* lintSeverityName(LintSeverity severity) {
    return severity == LintSeverity::Warning ? "WARN" : "ERROR";
}

namespace {

struct VariableInfo {
    SourceLocation location;
    bool used = false;
    bool reportUnused = true;
};

struct Scope {
    std::unordered_map<std::string, VariableInfo> variables;
};

struct FunctionInfo {
    int arity = 0;
    SourceLocation location;
};

class Linter {
public:
    Linter(std::filesystem::path filePath, std::filesystem::path projectRoot)
        : filePath_(std::move(filePath)),
          projectRoot_(std::move(projectRoot)) {}

    LintResult run(const std::vector<std::unique_ptr<Stmt>>& statements) {
        collectFunctions(statements);
        pushScope();
        analyzeStatements(statements);
        popScope();
        return result_;
    }

private:
    void collectFunctions(const std::vector<std::unique_ptr<Stmt>>& statements) {
        for (const auto& statement : statements) {
            if (const auto* function = dynamic_cast<const FunctionStmt*>(statement.get())) {
                functions_[function->name()] = {
                    static_cast<int>(function->parameters().size()),
                    function->location(),
                };
                collectFunctions(function->body());
                continue;
            }

            if (const auto* block = dynamic_cast<const BlockStmt*>(statement.get())) {
                collectFunctions(block->statements());
            } else if (const auto* ifStmt = dynamic_cast<const IfStmt*>(statement.get())) {
                collectFunctionFromBranch(ifStmt->thenBranch());
                if (ifStmt->elseBranch() != nullptr) {
                    collectFunctionFromBranch(*ifStmt->elseBranch());
                }
            } else if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(statement.get())) {
                collectFunctionFromBranch(whileStmt->body());
            } else if (const auto* tryCatch = dynamic_cast<const TryCatchStmt*>(statement.get())) {
                collectFunctions(tryCatch->tryBody());
                collectFunctions(tryCatch->catchBody());
            }
        }
    }

    void collectFunctionFromBranch(const Stmt& statement) {
        if (const auto* block = dynamic_cast<const BlockStmt*>(&statement)) {
            collectFunctions(block->statements());
        } else if (const auto* function = dynamic_cast<const FunctionStmt*>(&statement)) {
            functions_[function->name()] = {
                static_cast<int>(function->parameters().size()),
                function->location(),
            };
            collectFunctions(function->body());
        }
    }

    void analyzeStatements(const std::vector<std::unique_ptr<Stmt>>& statements) {
        bool unreachable = false;
        std::string reason;

        for (const auto& statement : statements) {
            if (unreachable) {
                result_.addWarning(statement->location(), "unreachable code after " + reason);
            }

            analyzeStatement(*statement);

            if (dynamic_cast<const ReturnStmt*>(statement.get()) != nullptr) {
                unreachable = true;
                reason = "return";
            } else if (dynamic_cast<const ThrowStmt*>(statement.get()) != nullptr) {
                unreachable = true;
                reason = "throw";
            }
        }
    }

    void analyzeStatement(const Stmt& statement) {
        if (const auto* printStmt = dynamic_cast<const PrintStmt*>(&statement)) {
            analyzeExpression(printStmt->expression());
        } else if (const auto* exprStmt = dynamic_cast<const ExpressionStmt*>(&statement)) {
            analyzeExpression(exprStmt->expression());
        } else if (const auto* letStmt = dynamic_cast<const LetStmt*>(&statement)) {
            analyzeExpression(letStmt->initializer());
            declareVariable(letStmt->name(), letStmt->location(), true);
        } else if (const auto* importStmt = dynamic_cast<const ImportStmt*>(&statement)) {
            checkImport(*importStmt);
        } else if (const auto* assignStmt = dynamic_cast<const IndexAssignStmt*>(&statement)) {
            analyzeExpression(assignStmt->object());
            analyzeExpression(assignStmt->index());
            analyzeExpression(assignStmt->value());
        } else if (const auto* functionStmt = dynamic_cast<const FunctionStmt*>(&statement)) {
            analyzeFunction(*functionStmt);
        } else if (const auto* returnStmt = dynamic_cast<const ReturnStmt*>(&statement)) {
            analyzeExpression(returnStmt->value());
        } else if (const auto* throwStmt = dynamic_cast<const ThrowStmt*>(&statement)) {
            analyzeExpression(throwStmt->value());
        } else if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(&statement)) {
            analyzeBlock(*blockStmt);
        } else if (const auto* ifStmt = dynamic_cast<const IfStmt*>(&statement)) {
            analyzeExpression(ifStmt->condition());
            analyzeStatement(ifStmt->thenBranch());
            if (ifStmt->elseBranch() != nullptr) {
                analyzeStatement(*ifStmt->elseBranch());
            }
        } else if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(&statement)) {
            analyzeExpression(whileStmt->condition());
            analyzeStatement(whileStmt->body());
        } else if (const auto* tryCatch = dynamic_cast<const TryCatchStmt*>(&statement)) {
            analyzeTryCatch(*tryCatch);
        }
    }

    void analyzeFunction(const FunctionStmt& functionStmt) {
        if (functionStmt.body().empty()) {
            result_.addWarning(functionStmt.location(), "empty block");
        }

        pushScope();
        for (const auto& parameter : functionStmt.parameters()) {
            declareVariable(parameter, functionStmt.location(), false);
        }
        analyzeStatements(functionStmt.body());
        popScope();
    }

    void analyzeBlock(const BlockStmt& blockStmt) {
        if (blockStmt.statements().empty()) {
            result_.addWarning(blockStmt.location(), "empty block");
        }

        pushScope();
        analyzeStatements(blockStmt.statements());
        popScope();
    }

    void analyzeTryCatch(const TryCatchStmt& tryCatch) {
        if (tryCatch.tryBody().empty()) {
            result_.addWarning(tryCatch.location(), "empty block");
        }

        pushScope();
        analyzeStatements(tryCatch.tryBody());
        popScope();

        if (tryCatch.catchBody().empty()) {
            result_.addWarning(tryCatch.location(), "empty block");
        }

        pushScope();
        declareVariable(tryCatch.catchName(), tryCatch.location(), false);
        analyzeStatements(tryCatch.catchBody());
        popScope();
    }

    void analyzeExpression(const Expr& expression) {
        if (const auto* literal = dynamic_cast<const LiteralExpr*>(&expression)) {
            analyzeInterpolatedString(*literal);
        } else if (const auto* variable = dynamic_cast<const VariableExpr*>(&expression)) {
            markVariableUsed(variable->name());
        } else if (const auto* call = dynamic_cast<const CallExpr*>(&expression)) {
            analyzeCall(*call);
        } else if (const auto* array = dynamic_cast<const ArrayExpr*>(&expression)) {
            for (const auto& element : array->elements()) {
                analyzeExpression(*element);
            }
        } else if (const auto* map = dynamic_cast<const MapExpr*>(&expression)) {
            for (const auto& entry : map->entries()) {
                analyzeExpression(*entry.value);
            }
        } else if (const auto* index = dynamic_cast<const IndexExpr*>(&expression)) {
            analyzeExpression(index->object());
            analyzeExpression(index->index());
        } else if (const auto* member = dynamic_cast<const MemberExpr*>(&expression)) {
            analyzeExpression(member->object());
        } else if (const auto* unary = dynamic_cast<const UnaryExpr*>(&expression)) {
            analyzeExpression(unary->right());
        } else if (const auto* binary = dynamic_cast<const BinaryExpr*>(&expression)) {
            analyzeBinary(*binary);
        } else if (const auto* grouping = dynamic_cast<const GroupingExpr*>(&expression)) {
            analyzeExpression(grouping->expression());
        }
    }

    void analyzeInterpolatedString(const LiteralExpr& literal) {
        if (!literal.value().isString()) {
            return;
        }

        const std::string& text = literal.value().asString();
        for (std::size_t index = 0; index + 1 < text.size(); ++index) {
            if (text[index] != '$' || text[index + 1] != '{') {
                continue;
            }

            const std::size_t expressionStart = index + 2;
            std::size_t cursor = expressionStart;
            int braceDepth = 0;
            bool inString = false;
            bool escaped = false;

            for (; cursor < text.size(); ++cursor) {
                const char ch = text[cursor];
                if (inString) {
                    if (escaped) {
                        escaped = false;
                        continue;
                    }
                    if (ch == '\\') {
                        escaped = true;
                        continue;
                    }
                    if (ch == '"') {
                        inString = false;
                    }
                    continue;
                }

                if (ch == '"') {
                    inString = true;
                    continue;
                }
                if (ch == '{') {
                    ++braceDepth;
                    continue;
                }
                if (ch == '}') {
                    if (braceDepth == 0) {
                        break;
                    }
                    --braceDepth;
                }
            }

            if (cursor >= text.size()) {
                return;
            }

            const std::string expressionSource = text.substr(expressionStart, cursor - expressionStart);
            try {
                Lexer lexer(expressionSource);
                Parser parser(lexer.scanTokens());
                auto expression = parser.parseExpressionOnly();
                analyzeExpression(*expression);
            } catch (const ZakeError&) {
                return;
            }

            index = cursor;
        }
    }

    void analyzeCall(const CallExpr& call) {
        if (const auto* variable = dynamic_cast<const VariableExpr*>(&call.callee())) {
            const auto found = functions_.find(variable->name());
            if (found != functions_.end() && static_cast<int>(call.arguments().size()) != found->second.arity) {
                result_.addError(call.location(),
                                 "function '" + variable->name() + "' expects " +
                                     std::to_string(found->second.arity) + " argument(s), got " +
                                     std::to_string(call.arguments().size()));
            }
        }

        analyzeExpression(call.callee());
        for (const auto& argument : call.arguments()) {
            analyzeExpression(*argument);
        }
    }

    void analyzeBinary(const BinaryExpr& binary) {
        analyzeExpression(binary.left());
        analyzeExpression(binary.right());

        if (binary.op() == TokenType::Slash && isConstantZero(binary.right())) {
            result_.addError(binary.location(), "division by zero");
        }
    }

    bool isConstantZero(const Expr& expression) const {
        if (const auto* literal = dynamic_cast<const LiteralExpr*>(&expression)) {
            return literal->value().isNumber() && std::fabs(literal->value().asNumber()) < 0.0000001;
        }

        if (const auto* grouping = dynamic_cast<const GroupingExpr*>(&expression)) {
            return isConstantZero(grouping->expression());
        }

        return false;
    }

    void checkImport(const ImportStmt& importStmt) {
        std::filesystem::path resolved;

        if (importStmt.isStandardLibrary()) {
            std::string module = importStmt.module();
            const std::string prefix = "std.";
            if (module.rfind(prefix, 0) == 0) {
                module = module.substr(prefix.size());
            }

            std::replace(module.begin(), module.end(), '.', '/');
            resolved = projectRoot_ / "stdlib" / (module + ".zk");
        } else {
            resolved = filePath_.parent_path() / importStmt.module();
        }

        std::error_code ec;
        if (!std::filesystem::exists(resolved, ec) || !std::filesystem::is_regular_file(resolved, ec)) {
            result_.addError(importStmt.location(), "import path not found: '" + importStmt.module() + "'");
        }
    }

    void pushScope() {
        scopes_.push_back({});
    }

    void popScope() {
        if (scopes_.empty()) {
            return;
        }

        for (const auto& entry : scopes_.back().variables) {
            if (!entry.second.used && entry.second.reportUnused) {
                result_.addWarning(entry.second.location, "unused variable '" + entry.first + "'");
            }
        }

        scopes_.pop_back();
    }

    void declareVariable(const std::string& name, SourceLocation location, bool reportUnused) {
        if (scopes_.empty()) {
            pushScope();
        }

        auto& current = scopes_.back().variables;
        if (current.find(name) != current.end()) {
            result_.addWarning(location, "duplicate variable declaration '" + name + "'");
        } else if (isVariableInOuterScope(name)) {
            result_.addWarning(location, "variable '" + name + "' shadows an outer variable");
        }

        current[name] = {location, false, reportUnused};
    }

    bool isVariableInOuterScope(const std::string& name) const {
        if (scopes_.size() < 2) {
            return false;
        }

        for (auto scope = scopes_.rbegin() + 1; scope != scopes_.rend(); ++scope) {
            if (scope->variables.find(name) != scope->variables.end()) {
                return true;
            }
        }

        return false;
    }

    void markVariableUsed(const std::string& name) {
        for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
            auto found = scope->variables.find(name);
            if (found != scope->variables.end()) {
                found->second.used = true;
                return;
            }
        }
    }

    std::filesystem::path filePath_;
    std::filesystem::path projectRoot_;
    LintResult result_;
    std::vector<Scope> scopes_;
    std::unordered_map<std::string, FunctionInfo> functions_;
};

} // namespace

LintResult lintProgram(const std::vector<std::unique_ptr<Stmt>>& statements,
                       const std::filesystem::path& filePath,
                       const std::filesystem::path& projectRoot) {
    return Linter(filePath, projectRoot).run(statements);
}

} // namespace zake
