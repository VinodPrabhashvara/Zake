#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "zake/ast.hpp"
#include "zake/token.hpp"

namespace zake {

enum class LintSeverity {
    Warning,
    Error,
};

struct LintDiagnostic {
    LintSeverity severity;
    SourceLocation location;
    std::string message;
};

class LintResult {
public:
    void addWarning(SourceLocation location, std::string message);
    void addError(SourceLocation location, std::string message);

    const std::vector<LintDiagnostic>& diagnostics() const;
    int warningCount() const;
    int errorCount() const;

private:
    std::vector<LintDiagnostic> diagnostics_;
};

LintResult lintProgram(const std::vector<std::unique_ptr<Stmt>>& statements,
                       const std::filesystem::path& filePath,
                       const std::filesystem::path& projectRoot);

const char* lintSeverityName(LintSeverity severity);

} // namespace zake
