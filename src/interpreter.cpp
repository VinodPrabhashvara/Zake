#include "zake/interpreter.hpp"
#include "zake/lexer.hpp"
#include "zake/parser.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace zake {

namespace {

class ReturnSignal final : public std::runtime_error {
public:
    explicit ReturnSignal(Value value)
        : std::runtime_error("return"), value_(std::move(value)) {}

    const Value& value() const {
        return value_;
    }

private:
    Value value_;
};

class ThrownSignal final : public std::runtime_error {
public:
    ThrownSignal(Value value, SourceLocation location)
        : std::runtime_error("throw"), value_(std::move(value)), location_(location) {}

    const Value& value() const {
        return value_;
    }

    const SourceLocation& location() const {
        return location_;
    }

private:
    Value value_;
    SourceLocation location_;
};

Value makeErrorValue(const std::string& type, const std::string& message, const SourceLocation& location) {
    auto error = std::make_shared<MapValue>();
    error->keys = {"type", "message", "line", "column"};
    error->entries["type"] = Value::string(type);
    error->entries["message"] = Value::string(message);
    error->entries["line"] = Value::number(static_cast<double>(location.line));
    error->entries["column"] = Value::number(static_cast<double>(location.column));
    return Value::map(std::move(error));
}

std::string classifyRuntimeError(const std::string& message) {
    if (message.find("file") != std::string::npos || message.find("File") != std::string::npos ||
        message.find("directory") != std::string::npos || message.find("Directory") != std::string::npos ||
        message.find("Permission denied") != std::string::npos) {
        return "FileError";
    }

    if (message.find("expects") != std::string::npos || message.find("Got ") != std::string::npos ||
        message.find("must be") != std::string::npos || message.find("Only functions can be called") != std::string::npos) {
        return "TypeError";
    }

    return "RuntimeError";
}

Value makeRuntimeErrorValue(const RuntimeError& error) {
    return makeErrorValue(classifyRuntimeError(error.what()), error.what(), error.location());
}

std::string describeThrownValue(const Value& value) {
    if (value.isMap()) {
        const auto map = value.asMap();
        const auto type = map->entries.find("type");
        const auto message = map->entries.find("message");
        if (type != map->entries.end() && type->second.isString() && message != map->entries.end() && message->second.isString()) {
            return type->second.asString() + ": " + message->second.asString();
        }
    }

    return value.toString();
}

std::string requireStringArgument(const std::vector<Value>& arguments, std::size_t index, const std::string& signature, const SourceLocation& location) {
    if (!arguments[index].isString()) {
        throw RuntimeError("Built-in " + signature + " expects argument " + std::to_string(index + 1) + " to be string, got " + arguments[index].typeName() + ".", location);
    }

    return arguments[index].asString();
}

double requireNumberArgument(const std::vector<Value>& arguments, std::size_t index, const std::string& signature, const SourceLocation& location) {
    if (!arguments[index].isNumber()) {
        throw RuntimeError("Built-in " + signature + " expects argument " + std::to_string(index + 1) + " to be number, got " + arguments[index].typeName() + ".", location);
    }

    return arguments[index].asNumber();
}

void requireArity(const std::vector<Value>& arguments, std::size_t expected, const std::string& signature, const SourceLocation& location) {
    if (arguments.size() != expected) {
        throw RuntimeError("Built-in " + signature + " expected " + std::to_string(expected) + " argument(s), got " + std::to_string(arguments.size()) + ".", location);
    }
}

std::size_t requireStringOffset(double value, std::size_t max, const std::string& signature, const SourceLocation& location) {
    if (!std::isfinite(value) || std::floor(value) != value) {
        throw RuntimeError("Built-in " + signature + " expects a whole-number string index.", location);
    }

    if (value < 0) {
        throw RuntimeError("Built-in " + signature + " string index cannot be negative.", location);
    }

    const auto index = static_cast<std::size_t>(value);
    if (index > max) {
        throw RuntimeError("Built-in " + signature + " string index " + std::to_string(index) + " is out of range for length " + std::to_string(max) + ".", location);
    }

    return index;
}

std::size_t requireStringElementIndex(double value, std::size_t size, const std::string& signature, const SourceLocation& location) {
    const auto index = requireStringOffset(value, size, signature, location);
    if (index >= size) {
        throw RuntimeError("Built-in " + signature + " string index " + std::to_string(index) + " is out of range for length " + std::to_string(size) + ".", location);
    }
    return index;
}

std::string trimString(std::string text) {
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });

    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    if (first >= last) {
        return "";
    }

    return std::string(first, last);
}

std::string hexEncode(const std::string& text) {
    static const char* digits = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(text.size() * 2);

    for (unsigned char ch : text) {
        encoded.push_back(digits[(ch >> 4) & 0x0F]);
        encoded.push_back(digits[ch & 0x0F]);
    }

    return encoded;
}

int hexValue(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

std::string hexDecode(const std::string& text, const SourceLocation& location) {
    if (text.size() % 2 != 0) {
        throw RuntimeError("Built-in __hex_decode(hex) expects an even-length hex string.", location);
    }

    std::string decoded;
    decoded.reserve(text.size() / 2);

    for (std::size_t index = 0; index < text.size(); index += 2) {
        const int high = hexValue(text[index]);
        const int low = hexValue(text[index + 1]);
        if (high < 0 || low < 0) {
            throw RuntimeError("Built-in __hex_decode(hex) received invalid hex text.", location);
        }
        decoded.push_back(static_cast<char>((high << 4) | low));
    }

    return decoded;
}

std::string base64Encode(const std::string& text) {
    static const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    int value = 0;
    int bits = -6;

    for (unsigned char ch : text) {
        value = (value << 8) + ch;
        bits += 8;
        while (bits >= 0) {
            encoded.push_back(alphabet[(value >> bits) & 0x3F]);
            bits -= 6;
        }
    }

    if (bits > -6) {
        encoded.push_back(alphabet[((value << 8) >> (bits + 8)) & 0x3F]);
    }

    while (encoded.size() % 4 != 0) {
        encoded.push_back('=');
    }

    return encoded;
}

std::string base64Decode(const std::string& text, const SourceLocation& location) {
    static const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> table(256, -1);
    for (std::size_t index = 0; index < alphabet.size(); ++index) {
        table[static_cast<unsigned char>(alphabet[index])] = static_cast<int>(index);
    }

    std::string decoded;
    int value = 0;
    int bits = -8;

    for (unsigned char ch : text) {
        if (std::isspace(ch)) {
            continue;
        }
        if (ch == '=') {
            break;
        }
        if (table[ch] < 0) {
            throw RuntimeError("Built-in __base64_decode(text) received invalid base64 text.", location);
        }

        value = (value << 6) + table[ch];
        bits += 6;
        if (bits >= 0) {
            decoded.push_back(static_cast<char>((value >> bits) & 0xFF));
            bits -= 8;
        }
    }

    return decoded;
}

std::string currentTimeString() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime {};

#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    std::ostringstream output;
    output << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

std::filesystem::path requirePathArgument(const std::vector<Value>& arguments, std::size_t index, const std::string& signature, const SourceLocation& location) {
    return std::filesystem::path(requireStringArgument(arguments, index, signature, location));
}

bool fileExists(const std::filesystem::path& path, const SourceLocation& location) {
    std::error_code error;
    const bool result = std::filesystem::exists(path, error);
    if (error) {
        throw RuntimeError("Built-in file operation could not inspect '" + path.string() + "': " + error.message() + ".", location);
    }
    return result;
}

bool isRegularFilePath(const std::filesystem::path& path, const SourceLocation& location) {
    std::error_code error;
    const bool result = std::filesystem::is_regular_file(path, error);
    if (error) {
        throw RuntimeError("Built-in file operation could not inspect file '" + path.string() + "': " + error.message() + ".", location);
    }
    return result;
}

bool isDirectoryPath(const std::filesystem::path& path, const SourceLocation& location) {
    std::error_code error;
    const bool result = std::filesystem::is_directory(path, error);
    if (error) {
        throw RuntimeError("Built-in file operation could not inspect directory '" + path.string() + "': " + error.message() + ".", location);
    }
    return result;
}

void requireExistingFile(const std::filesystem::path& path, const SourceLocation& location) {
    if (!fileExists(path, location)) {
        throw RuntimeError("File does not exist: '" + path.string() + "'.", location);
    }

    if (!isRegularFilePath(path, location)) {
        throw RuntimeError("Path is not a file: '" + path.string() + "'.", location);
    }
}

void rejectDirectoryTarget(const std::filesystem::path& path, const SourceLocation& location) {
    if (fileExists(path, location) && isDirectoryPath(path, location)) {
        throw RuntimeError("Path is a directory, not a file: '" + path.string() + "'.", location);
    }
}

} // namespace

Interpreter::Interpreter(std::filesystem::path entryFile)
    : project_root_(findProjectRoot(std::filesystem::absolute(entryFile).parent_path())),
      stdlib_root_(project_root_ / "stdlib") {}

Interpreter::Interpreter(std::filesystem::path entryFile, std::filesystem::path projectRoot, std::filesystem::path stdlibRoot)
    : project_root_(std::filesystem::absolute(std::move(projectRoot)).lexically_normal()),
      stdlib_root_(std::filesystem::absolute(std::move(stdlibRoot)).lexically_normal()) {
    (void)entryFile;
}

std::string Interpreter::readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Could not open file '" + path.string() + "'.");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void Interpreter::executeFile(const std::filesystem::path& path) {
    const std::filesystem::path absolutePath = std::filesystem::absolute(path).lexically_normal();
    file_stack_.push_back(absolutePath);

    try {
        const std::string source = readFile(absolutePath);
        Lexer lexer(source);
        const auto tokens = lexer.scanTokens();

        Parser parser(tokens);
        const auto statements = parser.parse();
        execute(statements);
    } catch (const ThrownSignal& thrown) {
        file_stack_.pop_back();
        throw RuntimeError("Uncaught thrown value: " + describeThrownValue(thrown.value()), thrown.location());
    } catch (...) {
        file_stack_.pop_back();
        throw;
    }

    file_stack_.pop_back();
}

void Interpreter::execute(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& statement : statements) {
        executeStatement(*statement);
    }
}

void Interpreter::executeInteractive(const std::vector<std::unique_ptr<Stmt>>& statements) {
    try {
        execute(statements);
    } catch (const ThrownSignal& thrown) {
        throw RuntimeError("Uncaught thrown value: " + describeThrownValue(thrown.value()), thrown.location());
    }
}

std::vector<std::pair<std::string, std::string>> Interpreter::globalsSnapshot() const {
    std::vector<std::pair<std::string, std::string>> globals;
    if (scopes_.empty()) {
        return globals;
    }

    globals.reserve(scopes_.front().size());
    for (const auto& entry : scopes_.front()) {
        globals.emplace_back(entry.first, entry.second.toString());
    }

    std::sort(globals.begin(), globals.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
    });
    return globals;
}

void Interpreter::executeStatement(const Stmt& statement) {
    if (const auto* import = dynamic_cast<const ImportStmt*>(&statement)) {
        executeImport(*import);
        return;
    }

    if (const auto* function = dynamic_cast<const FunctionStmt*>(&statement)) {
        auto functionValue = std::make_shared<FunctionValue>();
        functionValue->name = function->name();
        functionValue->declaration = function;
        scopes_.back()[function->name()] = Value::function(std::move(functionValue));
        return;
    }

    if (const auto* returnStatement = dynamic_cast<const ReturnStmt*>(&statement)) {
        if (call_depth_ == 0) {
            throw RuntimeError("'return' can only be used inside a function.", returnStatement->location());
        }
        throw ReturnSignal(evaluate(returnStatement->value()));
    }

    if (const auto* throwStatement = dynamic_cast<const ThrowStmt*>(&statement)) {
        throw ThrownSignal(evaluate(throwStatement->value()), throwStatement->location());
    }

    if (const auto* tryCatch = dynamic_cast<const TryCatchStmt*>(&statement)) {
        try {
            executeBlock(tryCatch->tryBody());
        } catch (const ThrownSignal& thrown) {
            executeCatchBlock(*tryCatch, thrown.value());
        } catch (const RuntimeError& error) {
            executeCatchBlock(*tryCatch, makeRuntimeErrorValue(error));
        }
        return;
    }

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

    if (const auto* expression = dynamic_cast<const ExpressionStmt*>(&statement)) {
        evaluate(expression->expression());
        return;
    }

    if (const auto* indexAssign = dynamic_cast<const IndexAssignStmt*>(&statement)) {
        assignIndex(*indexAssign);
        return;
    }

    if (const auto* let = dynamic_cast<const LetStmt*>(&statement)) {
        assignOrDefine(let->name(), evaluate(let->initializer()));
        return;
    }

    throw RuntimeError("Unsupported statement.", statement.location());
}

void Interpreter::executeImport(const ImportStmt& statement) {
    const std::filesystem::path resolved = resolveImportPath(statement);

    if (statement.hasAlias()) {
        if (nameExists(statement.alias())) {
            throw RuntimeError("Import alias '" + statement.alias() + "' conflicts with an existing name.", statement.location());
        }

        scopes_.front()[statement.alias()] = makeNamespaceValue(loadModuleExports(resolved, statement.location()));
        return;
    }

    if (statement.isSelective()) {
        const auto& exports = loadModuleExports(resolved, statement.location());
        for (const auto& symbol : statement.symbols()) {
            if (exports.find(symbol) == exports.end()) {
                throw RuntimeError("Imported symbol '" + symbol + "' does not exist in module '" + statement.module() + "'.", statement.location());
            }
            if (nameExists(symbol)) {
                throw RuntimeError("Imported symbol '" + symbol + "' conflicts with an existing name.", statement.location());
            }
        }

        for (const auto& symbol : statement.symbols()) {
            defineImportedName(symbol, exports.at(symbol), statement.location());
        }
        return;
    }

    executeModuleInGlobalScope(resolved, statement.location());
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

void Interpreter::executeCatchBlock(const TryCatchStmt& statement, Value errorValue) {
    scopes_.push_back({});
    scopes_.back()[statement.catchName()] = std::move(errorValue);

    try {
        for (const auto& catchStatement : statement.catchBody()) {
            executeStatement(*catchStatement);
        }
    } catch (...) {
        scopes_.pop_back();
        throw;
    }

    scopes_.pop_back();
}

std::filesystem::path Interpreter::resolveImportPath(const ImportStmt& statement) const {
    if (statement.isStandardLibrary()) {
        std::string module = statement.module().substr(4);
        std::replace(module.begin(), module.end(), '.', '/');
        auto resolved = stdlib_root_ / module;
        resolved.replace_extension(".zk");
        return resolved.lexically_normal();
    }

    if (file_stack_.empty()) {
        return std::filesystem::absolute(statement.module()).lexically_normal();
    }

    return (file_stack_.back().parent_path() / statement.module()).lexically_normal();
}

std::filesystem::path Interpreter::findProjectRoot(const std::filesystem::path& start) const {
    std::filesystem::path current = start;

    while (!current.empty()) {
        if (std::filesystem::exists(current / "stdlib") || std::filesystem::exists(current / "zake.toml")) {
            return current;
        }

        const std::filesystem::path parent = current.parent_path();
        if (parent == current) {
            break;
        }
        current = parent;
    }

    return start;
}

void Interpreter::executeModuleInGlobalScope(const std::filesystem::path& path, const SourceLocation& importLocation) {
    const std::filesystem::path absolutePath = std::filesystem::absolute(path).lexically_normal();
    const std::string key = absolutePath.string();

    if (imported_files_.find(key) != imported_files_.end()) {
        return;
    }

    const auto& exports = loadModuleExports(absolutePath, importLocation);
    for (const auto& entry : exports) {
        scopes_.front()[entry.first] = entry.second;
    }
    imported_files_.insert(key);
}

const std::unordered_map<std::string, Value>& Interpreter::loadModuleExports(const std::filesystem::path& path, const SourceLocation& importLocation) {
    const std::filesystem::path absolutePath = std::filesystem::absolute(path).lexically_normal();
    const std::string key = absolutePath.string();

    const auto cached = module_exports_.find(key);
    if (cached != module_exports_.end()) {
        return cached->second;
    }

    if (importing_files_.find(key) != importing_files_.end()) {
        throw RuntimeError("Circular import detected for '" + absolutePath.string() + "'.", importLocation);
    }

    if (!std::filesystem::exists(absolutePath)) {
        throw RuntimeError("Import file not found: '" + absolutePath.string() + "'.", importLocation);
    }

    importing_files_.insert(key);
    file_stack_.push_back(absolutePath);

    try {
        const std::string source = readFile(absolutePath);
        Lexer lexer(source);
        const auto tokens = lexer.scanTokens();

        Parser parser(tokens);
        auto statements = parser.parse();
        loaded_modules_.push_back(std::move(statements));

        auto savedScopes = std::move(scopes_);
        scopes_.clear();
        scopes_.push_back({});

        try {
            execute(loaded_modules_.back());
        } catch (...) {
            scopes_ = std::move(savedScopes);
            throw;
        }

        auto exports = std::move(scopes_.front());
        scopes_ = std::move(savedScopes);
        module_exports_.emplace(key, std::move(exports));
    } catch (const ZakeError& error) {
        importing_files_.erase(key);
        file_stack_.pop_back();
        throw RuntimeError(
            "Import failed for '" + absolutePath.string() + "' at line " + std::to_string(error.location().line) + ", column " + std::to_string(error.location().column) + ": " + error.what(),
            importLocation);
    } catch (const std::exception& error) {
        importing_files_.erase(key);
        file_stack_.pop_back();
        throw RuntimeError("Import failed for '" + absolutePath.string() + "': " + error.what(), importLocation);
    }

    importing_files_.erase(key);
    file_stack_.pop_back();
    return module_exports_.at(key);
}

void Interpreter::defineImportedName(const std::string& name, const Value& value, const SourceLocation& location) {
    if (scopes_.empty()) {
        throw RuntimeError("Internal error: no global scope is available for import.", location);
    }
    scopes_.front()[name] = value;
}

bool Interpreter::nameExists(const std::string& name) const {
    if (isBuiltinName(name)) {
        return true;
    }

    for (const auto& scope : scopes_) {
        if (scope.find(name) != scope.end()) {
            return true;
        }
    }

    return false;
}

Value Interpreter::makeNamespaceValue(const std::unordered_map<std::string, Value>& exports) {
    auto map = std::make_shared<MapValue>();
    map->keys.reserve(exports.size());
    for (const auto& entry : exports) {
        map->keys.push_back(entry.first);
    }
    std::sort(map->keys.begin(), map->keys.end());
    for (const auto& key : map->keys) {
        map->entries[key] = exports.at(key);
    }
    return Value::map(std::move(map));
}

Value Interpreter::callFunction(const FunctionValue& function, const std::vector<Value>& arguments, const SourceLocation& location) {
    if (function.declaration == nullptr) {
        throw RuntimeError("Cannot call function '" + function.name + "' because its declaration is missing.", location);
    }

    const auto& parameters = function.declaration->parameters();
    if (arguments.size() != parameters.size()) {
        throw RuntimeError(
            "Function '" + function.name + "' expected " + std::to_string(parameters.size()) + " argument(s), but got " + std::to_string(arguments.size()) + ".",
            location);
    }

    scopes_.push_back({});
    for (std::size_t index = 0; index < parameters.size(); ++index) {
        scopes_.back()[parameters[index]] = arguments[index];
    }

    ++call_depth_;
    try {
        for (const auto& statement : function.declaration->body()) {
            executeStatement(*statement);
        }
    } catch (const ReturnSignal& signal) {
        --call_depth_;
        scopes_.pop_back();
        return signal.value();
    } catch (...) {
        --call_depth_;
        scopes_.pop_back();
        throw;
    }

    --call_depth_;
    scopes_.pop_back();
    return Value::nil();
}

Value Interpreter::evaluate(const Expr& expression) {
    if (const auto* literal = dynamic_cast<const LiteralExpr*>(&expression)) {
        return evaluateLiteral(*literal);
    }

    if (const auto* variable = dynamic_cast<const VariableExpr*>(&expression)) {
        return evaluateVariable(*variable);
    }

    if (const auto* call = dynamic_cast<const CallExpr*>(&expression)) {
        return evaluateCall(*call);
    }

    if (const auto* array = dynamic_cast<const ArrayExpr*>(&expression)) {
        return evaluateArray(*array);
    }

    if (const auto* map = dynamic_cast<const MapExpr*>(&expression)) {
        return evaluateMap(*map);
    }

    if (const auto* index = dynamic_cast<const IndexExpr*>(&expression)) {
        return evaluateIndex(*index);
    }

    if (const auto* member = dynamic_cast<const MemberExpr*>(&expression)) {
        return evaluateMember(*member);
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
    if (expression.value().isString()) {
        return interpolateString(expression.value().asString(), expression.location());
    }
    return expression.value();
}

Value Interpreter::interpolateString(const std::string& text, const SourceLocation& location) {
    if (text.find("${") == std::string::npos) {
        return Value::string(text);
    }

    std::string result;

    for (std::size_t index = 0; index < text.size();) {
        if (text[index] != '$' || index + 1 >= text.size() || text[index + 1] != '{') {
            result.push_back(text[index]);
            ++index;
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
            throw RuntimeError("Invalid interpolation syntax: missing '}' after '${'.", location);
        }

        const std::string expressionSource = text.substr(expressionStart, cursor - expressionStart);
        if (trimString(expressionSource).empty()) {
            throw RuntimeError("Invalid interpolation syntax: empty expression.", location);
        }

        result += evaluateInterpolationExpression(expressionSource, location).toString();
        index = cursor + 1;
    }

    return Value::string(std::move(result));
}

Value Interpreter::evaluateInterpolationExpression(const std::string& source, const SourceLocation& location) {
    try {
        Lexer lexer(source);
        const auto tokens = lexer.scanTokens();
        Parser parser(tokens);
        auto expression = parser.parseExpressionOnly();
        return evaluate(*expression);
    } catch (const ZakeError& error) {
        throw RuntimeError("Invalid interpolation expression: " + std::string(error.what()), location);
    }
}

Value Interpreter::evaluateVariable(const VariableExpr& expression) {
    return lookupVariable(expression.name(), expression.location());
}

Value Interpreter::evaluateCall(const CallExpr& expression) {
    if (const auto* variable = dynamic_cast<const VariableExpr*>(&expression.callee())) {
        if (isBuiltinName(variable->name())) {
            std::vector<Value> arguments;
            arguments.reserve(expression.arguments().size());
            for (const auto& argument : expression.arguments()) {
                arguments.push_back(evaluate(*argument));
            }
            return callBuiltin(variable->name(), arguments, expression.location());
        }
    }

    const Value callee = evaluate(expression.callee());
    if (!callee.isFunction()) {
        throw RuntimeError("Only functions can be called. Got " + callee.typeName() + ".", expression.location());
    }

    std::vector<Value> arguments;
    arguments.reserve(expression.arguments().size());
    for (const auto& argument : expression.arguments()) {
        arguments.push_back(evaluate(*argument));
    }

    return callFunction(*callee.asFunction(), arguments, expression.location());
}

Value Interpreter::evaluateArray(const ArrayExpr& expression) {
    auto array = std::make_shared<ArrayValue>();
    array->elements.reserve(expression.elements().size());
    for (const auto& element : expression.elements()) {
        array->elements.push_back(evaluate(*element));
    }
    return Value::array(std::move(array));
}

Value Interpreter::evaluateMap(const MapExpr& expression) {
    auto map = std::make_shared<MapValue>();

    for (const auto& entry : expression.entries()) {
        const bool isNewKey = map->entries.find(entry.key) == map->entries.end();
        if (isNewKey) {
            map->keys.push_back(entry.key);
        }
        map->entries[entry.key] = evaluate(*entry.value);
    }

    return Value::map(std::move(map));
}

Value Interpreter::evaluateIndex(const IndexExpr& expression) {
    const Value object = evaluate(expression.object());
    if (object.isArray()) {
        auto array = object.asArray();
        const std::size_t index = requireIndex(evaluate(expression.index()), array->elements.size(), expression.index().location());
        return array->elements[index];
    }

    if (object.isMap()) {
        auto map = object.asMap();
        const std::string key = requireMapKey(evaluate(expression.index()), expression.index().location());
        const auto found = map->entries.find(key);
        if (found == map->entries.end()) {
            throw RuntimeError("Map key '" + key + "' was not found.", expression.location());
        }
        return found->second;
    }

    throw RuntimeError("Indexing requires an array or map, got " + object.typeName() + ".", expression.location());
}

Value Interpreter::evaluateMember(const MemberExpr& expression) {
    const Value object = evaluate(expression.object());
    if (!object.isMap()) {
        throw RuntimeError("Member access requires a module namespace or map, got " + object.typeName() + ".", expression.location());
    }

    const auto map = object.asMap();
    const auto found = map->entries.find(expression.member());
    if (found == map->entries.end()) {
        throw RuntimeError("Member '" + expression.member() + "' does not exist.", expression.location());
    }

    return found->second;
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
        if (left.isString() && right.isString()) {
            return Value::string(left.asString() + right.asString());
        }
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

void Interpreter::assignIndex(const IndexAssignStmt& statement) {
    const Value object = evaluate(statement.object());
    if (object.isArray()) {
        auto array = object.asArray();
        const std::size_t index = requireIndex(evaluate(statement.index()), array->elements.size(), statement.index().location());
        array->elements[index] = evaluate(statement.value());
        return;
    }

    if (object.isMap()) {
        auto map = object.asMap();
        const std::string key = requireMapKey(evaluate(statement.index()), statement.index().location());
        if (map->entries.find(key) == map->entries.end()) {
            map->keys.push_back(key);
        }
        map->entries[key] = evaluate(statement.value());
        return;
    }

    throw RuntimeError("Index assignment requires an array or map, got " + object.typeName() + ".", statement.location());
}

Value Interpreter::callBuiltin(const std::string& name, const std::vector<Value>& arguments, const SourceLocation& location) {
    if (name == "len") {
        if (arguments.size() != 1) {
            throw RuntimeError("Built-in len(value) expected 1 argument, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (arguments[0].isArray()) {
            return Value::number(static_cast<double>(arguments[0].asArray()->elements.size()));
        }

        if (arguments[0].isMap()) {
            return Value::number(static_cast<double>(arguments[0].asMap()->keys.size()));
        }

        if (arguments[0].isString()) {
            return Value::number(static_cast<double>(arguments[0].asString().size()));
        }

        throw RuntimeError("Built-in len(value) expects an array, map, or string, got " + arguments[0].typeName() + ".", location);
    }

    if (name == "push") {
        if (arguments.size() != 2) {
            throw RuntimeError("Built-in push(array, value) expected 2 arguments, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (!arguments[0].isArray()) {
            throw RuntimeError("Built-in push(array, value) expects first argument to be array, got " + arguments[0].typeName() + ".", location);
        }

        arguments[0].asArray()->elements.push_back(arguments[1]);
        return Value::nil();
    }

    if (name == "pop") {
        if (arguments.size() != 1) {
            throw RuntimeError("Built-in pop(array) expected 1 argument, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (!arguments[0].isArray()) {
            throw RuntimeError("Built-in pop(array) expects an array, got " + arguments[0].typeName() + ".", location);
        }

        auto array = arguments[0].asArray();
        if (array->elements.empty()) {
            throw RuntimeError("Built-in pop(array) cannot pop from an empty array.", location);
        }

        const Value value = array->elements.back();
        array->elements.pop_back();
        return value;
    }

    if (name == "type") {
        if (arguments.size() != 1) {
            throw RuntimeError("Built-in type(value) expected 1 argument, got " + std::to_string(arguments.size()) + ".", location);
        }

        return Value::string(arguments[0].typeName());
    }

    if (name == "__upper") {
        if (arguments.size() != 1) {
            throw RuntimeError("Built-in __upper(text) expected 1 argument, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (!arguments[0].isString()) {
            throw RuntimeError("Built-in __upper(text) expects a string, got " + arguments[0].typeName() + ".", location);
        }

        std::string text = arguments[0].asString();
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
            return static_cast<char>(std::toupper(ch));
        });
        return Value::string(std::move(text));
    }

    if (name == "__lower") {
        if (arguments.size() != 1) {
            throw RuntimeError("Built-in __lower(text) expected 1 argument, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (!arguments[0].isString()) {
            throw RuntimeError("Built-in __lower(text) expects a string, got " + arguments[0].typeName() + ".", location);
        }

        std::string text = arguments[0].asString();
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return Value::string(std::move(text));
    }

    if (name == "__contains") {
        if (arguments.size() != 2) {
            throw RuntimeError("Built-in __contains(text, part) expected 2 arguments, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (!arguments[0].isString() || !arguments[1].isString()) {
            throw RuntimeError("Built-in __contains(text, part) expects two strings, got " + arguments[0].typeName() + " and " + arguments[1].typeName() + ".", location);
        }

        return Value::boolean(arguments[0].asString().find(arguments[1].asString()) != std::string::npos);
    }

    if (name == "__starts_with") {
        requireArity(arguments, 2, "__starts_with(text, part)", location);
        const std::string text = requireStringArgument(arguments, 0, "__starts_with(text, part)", location);
        const std::string part = requireStringArgument(arguments, 1, "__starts_with(text, part)", location);
        return Value::boolean(text.rfind(part, 0) == 0);
    }

    if (name == "__ends_with") {
        requireArity(arguments, 2, "__ends_with(text, part)", location);
        const std::string text = requireStringArgument(arguments, 0, "__ends_with(text, part)", location);
        const std::string part = requireStringArgument(arguments, 1, "__ends_with(text, part)", location);
        if (part.size() > text.size()) {
            return Value::boolean(false);
        }
        return Value::boolean(text.compare(text.size() - part.size(), part.size(), part) == 0);
    }

    if (name == "__trim") {
        requireArity(arguments, 1, "__trim(text)", location);
        return Value::string(trimString(requireStringArgument(arguments, 0, "__trim(text)", location)));
    }

    if (name == "__strlen") {
        requireArity(arguments, 1, "__strlen(text)", location);
        return Value::number(static_cast<double>(requireStringArgument(arguments, 0, "__strlen(text)", location).size()));
    }

    if (name == "__split") {
        requireArity(arguments, 2, "__split(text, sep)", location);
        const std::string text = requireStringArgument(arguments, 0, "__split(text, sep)", location);
        const std::string separator = requireStringArgument(arguments, 1, "__split(text, sep)", location);
        if (separator.empty()) {
            throw RuntimeError("Built-in __split(text, sep) separator cannot be empty.", location);
        }

        auto result = std::make_shared<ArrayValue>();
        std::size_t start = 0;
        while (true) {
            const std::size_t found = text.find(separator, start);
            if (found == std::string::npos) {
                result->elements.push_back(Value::string(text.substr(start)));
                break;
            }
            result->elements.push_back(Value::string(text.substr(start, found - start)));
            start = found + separator.size();
        }
        return Value::array(std::move(result));
    }

    if (name == "__replace") {
        requireArity(arguments, 3, "__replace(text, old, new)", location);
        std::string text = requireStringArgument(arguments, 0, "__replace(text, old, new)", location);
        const std::string oldText = requireStringArgument(arguments, 1, "__replace(text, old, new)", location);
        const std::string newText = requireStringArgument(arguments, 2, "__replace(text, old, new)", location);
        if (oldText.empty()) {
            throw RuntimeError("Built-in __replace(text, old, new) old text cannot be empty.", location);
        }

        std::size_t cursor = 0;
        while ((cursor = text.find(oldText, cursor)) != std::string::npos) {
            text.replace(cursor, oldText.size(), newText);
            cursor += newText.size();
        }
        return Value::string(std::move(text));
    }

    if (name == "__substring") {
        requireArity(arguments, 3, "__substring(text, start, end)", location);
        const std::string text = requireStringArgument(arguments, 0, "__substring(text, start, end)", location);
        const std::size_t start = requireStringOffset(
            requireNumberArgument(arguments, 1, "__substring(text, start, end)", location),
            text.size(),
            "__substring(text, start, end)",
            location);
        const std::size_t end = requireStringOffset(
            requireNumberArgument(arguments, 2, "__substring(text, start, end)", location),
            text.size(),
            "__substring(text, start, end)",
            location);
        if (end < start) {
            throw RuntimeError("Built-in __substring(text, start, end) end index cannot be less than start index.", location);
        }
        return Value::string(text.substr(start, end - start));
    }

    if (name == "__char_at") {
        requireArity(arguments, 2, "__char_at(text, index)", location);
        const std::string text = requireStringArgument(arguments, 0, "__char_at(text, index)", location);
        const std::size_t index = requireStringElementIndex(
            requireNumberArgument(arguments, 1, "__char_at(text, index)", location),
            text.size(),
            "__char_at(text, index)",
            location);
        return Value::string(text.substr(index, 1));
    }

    if (name == "__index_of") {
        requireArity(arguments, 2, "__index_of(text, part)", location);
        const std::string text = requireStringArgument(arguments, 0, "__index_of(text, part)", location);
        const std::string part = requireStringArgument(arguments, 1, "__index_of(text, part)", location);
        const std::size_t found = text.find(part);
        if (found == std::string::npos) {
            return Value::number(-1);
        }
        return Value::number(static_cast<double>(found));
    }

    if (name == "__sqrt") {
        if (arguments.size() != 1) {
            throw RuntimeError("Built-in __sqrt(number) expected 1 argument, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (!arguments[0].isNumber()) {
            throw RuntimeError("Built-in __sqrt(number) expects a number, got " + arguments[0].typeName() + ".", location);
        }

        if (arguments[0].asNumber() < 0) {
            throw RuntimeError("Built-in __sqrt(number) cannot take the square root of a negative number.", location);
        }

        return Value::number(std::sqrt(arguments[0].asNumber()));
    }

    if (name == "__abs") {
        requireArity(arguments, 1, "__abs(number)", location);
        return Value::number(std::fabs(requireNumberArgument(arguments, 0, "__abs(number)", location)));
    }

    if (name == "__min") {
        requireArity(arguments, 2, "__min(a, b)", location);
        return Value::number(std::min(
            requireNumberArgument(arguments, 0, "__min(a, b)", location),
            requireNumberArgument(arguments, 1, "__min(a, b)", location)));
    }

    if (name == "__max") {
        requireArity(arguments, 2, "__max(a, b)", location);
        return Value::number(std::max(
            requireNumberArgument(arguments, 0, "__max(a, b)", location),
            requireNumberArgument(arguments, 1, "__max(a, b)", location)));
    }

    if (name == "__round") {
        requireArity(arguments, 1, "__round(number)", location);
        return Value::number(std::round(requireNumberArgument(arguments, 0, "__round(number)", location)));
    }

    if (name == "__hex_encode") {
        requireArity(arguments, 1, "__hex_encode(text)", location);
        return Value::string(hexEncode(requireStringArgument(arguments, 0, "__hex_encode(text)", location)));
    }

    if (name == "__hex_decode") {
        requireArity(arguments, 1, "__hex_decode(hex)", location);
        return Value::string(hexDecode(requireStringArgument(arguments, 0, "__hex_decode(hex)", location), location));
    }

    if (name == "__base64_encode") {
        requireArity(arguments, 1, "__base64_encode(text)", location);
        return Value::string(base64Encode(requireStringArgument(arguments, 0, "__base64_encode(text)", location)));
    }

    if (name == "__base64_decode") {
        requireArity(arguments, 1, "__base64_decode(text)", location);
        return Value::string(base64Decode(requireStringArgument(arguments, 0, "__base64_decode(text)", location), location));
    }

    if (name == "__now") {
        requireArity(arguments, 0, "__now()", location);
        return Value::string(currentTimeString());
    }

    if (name == "__timestamp") {
        requireArity(arguments, 0, "__timestamp()", location);
        const auto now = std::chrono::system_clock::now();
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        return Value::number(static_cast<double>(seconds));
    }

    if (name == "__path_join") {
        requireArity(arguments, 2, "__path_join(a, b)", location);
        const std::filesystem::path left(requireStringArgument(arguments, 0, "__path_join(a, b)", location));
        const std::filesystem::path right(requireStringArgument(arguments, 1, "__path_join(a, b)", location));
        return Value::string((left / right).generic_string());
    }

    if (name == "__path_basename") {
        requireArity(arguments, 1, "__path_basename(path)", location);
        return Value::string(std::filesystem::path(requireStringArgument(arguments, 0, "__path_basename(path)", location)).filename().generic_string());
    }

    if (name == "__path_dirname") {
        requireArity(arguments, 1, "__path_dirname(path)", location);
        return Value::string(std::filesystem::path(requireStringArgument(arguments, 0, "__path_dirname(path)", location)).parent_path().generic_string());
    }

    if (name == "__path_extension") {
        requireArity(arguments, 1, "__path_extension(path)", location);
        return Value::string(std::filesystem::path(requireStringArgument(arguments, 0, "__path_extension(path)", location)).extension().generic_string());
    }

    if (name == "__read_file") {
        requireArity(arguments, 1, "__read_file(path)", location);
        const auto path = requirePathArgument(arguments, 0, "__read_file(path)", location);
        requireExistingFile(path, location);

        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw RuntimeError("Permission denied or unable to read file: '" + path.string() + "'.", location);
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();
        if (input.bad()) {
            throw RuntimeError("Failed while reading file: '" + path.string() + "'.", location);
        }
        return Value::string(buffer.str());
    }

    if (name == "__write_file") {
        requireArity(arguments, 2, "__write_file(path, content)", location);
        const auto path = requirePathArgument(arguments, 0, "__write_file(path, content)", location);
        const std::string content = requireStringArgument(arguments, 1, "__write_file(path, content)", location);
        rejectDirectoryTarget(path, location);

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw RuntimeError("Permission denied or unable to write file: '" + path.string() + "'.", location);
        }
        output << content;
        if (!output) {
            throw RuntimeError("Failed while writing file: '" + path.string() + "'.", location);
        }
        return Value::nil();
    }

    if (name == "__append_file") {
        requireArity(arguments, 2, "__append_file(path, content)", location);
        const auto path = requirePathArgument(arguments, 0, "__append_file(path, content)", location);
        const std::string content = requireStringArgument(arguments, 1, "__append_file(path, content)", location);
        rejectDirectoryTarget(path, location);

        std::ofstream output(path, std::ios::binary | std::ios::app);
        if (!output) {
            throw RuntimeError("Permission denied or unable to append file: '" + path.string() + "'.", location);
        }
        output << content;
        if (!output) {
            throw RuntimeError("Failed while appending file: '" + path.string() + "'.", location);
        }
        return Value::nil();
    }

    if (name == "__exists") {
        requireArity(arguments, 1, "__exists(path)", location);
        return Value::boolean(fileExists(requirePathArgument(arguments, 0, "__exists(path)", location), location));
    }

    if (name == "__is_file") {
        requireArity(arguments, 1, "__is_file(path)", location);
        const auto path = requirePathArgument(arguments, 0, "__is_file(path)", location);
        return Value::boolean(fileExists(path, location) && isRegularFilePath(path, location));
    }

    if (name == "__is_dir") {
        requireArity(arguments, 1, "__is_dir(path)", location);
        const auto path = requirePathArgument(arguments, 0, "__is_dir(path)", location);
        return Value::boolean(fileExists(path, location) && isDirectoryPath(path, location));
    }

    if (name == "__list_dir") {
        requireArity(arguments, 1, "__list_dir(path)", location);
        const auto path = requirePathArgument(arguments, 0, "__list_dir(path)", location);
        if (!fileExists(path, location)) {
            throw RuntimeError("Directory does not exist: '" + path.string() + "'.", location);
        }
        if (!isDirectoryPath(path, location)) {
            throw RuntimeError("Path is not a directory: '" + path.string() + "'.", location);
        }

        std::error_code error;
        std::vector<std::string> names;
        for (const auto& entry : std::filesystem::directory_iterator(path, error)) {
            names.push_back(entry.path().filename().generic_string());
        }
        if (error) {
            throw RuntimeError("Permission denied or unable to list directory: '" + path.string() + "': " + error.message() + ".", location);
        }
        std::sort(names.begin(), names.end());

        auto result = std::make_shared<ArrayValue>();
        for (const auto& item : names) {
            result->elements.push_back(Value::string(item));
        }
        return Value::array(std::move(result));
    }

    if (name == "__file_size") {
        requireArity(arguments, 1, "__file_size(path)", location);
        const auto path = requirePathArgument(arguments, 0, "__file_size(path)", location);
        requireExistingFile(path, location);

        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        if (error) {
            throw RuntimeError("Permission denied or unable to get file size for '" + path.string() + "': " + error.message() + ".", location);
        }
        return Value::number(static_cast<double>(size));
    }

    if (name == "__remove_file") {
        requireArity(arguments, 1, "__remove_file(path)", location);
        const auto path = requirePathArgument(arguments, 0, "__remove_file(path)", location);
        requireExistingFile(path, location);

        std::error_code error;
        const bool removed = std::filesystem::remove(path, error);
        if (error) {
            throw RuntimeError("Permission denied or unable to remove file '" + path.string() + "': " + error.message() + ".", location);
        }
        return Value::boolean(removed);
    }

    if (name == "keys") {
        if (arguments.size() != 1) {
            throw RuntimeError("Built-in keys(map) expected 1 argument, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (!arguments[0].isMap()) {
            throw RuntimeError("Built-in keys(map) expects a map, got " + arguments[0].typeName() + ".", location);
        }

        auto result = std::make_shared<ArrayValue>();
        for (const auto& key : arguments[0].asMap()->keys) {
            result->elements.push_back(Value::string(key));
        }
        return Value::array(std::move(result));
    }

    if (name == "values") {
        if (arguments.size() != 1) {
            throw RuntimeError("Built-in values(map) expected 1 argument, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (!arguments[0].isMap()) {
            throw RuntimeError("Built-in values(map) expects a map, got " + arguments[0].typeName() + ".", location);
        }

        auto result = std::make_shared<ArrayValue>();
        const auto map = arguments[0].asMap();
        for (const auto& key : map->keys) {
            result->elements.push_back(map->entries.at(key));
        }
        return Value::array(std::move(result));
    }

    if (name == "has") {
        if (arguments.size() != 2) {
            throw RuntimeError("Built-in has(map, key) expected 2 arguments, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (!arguments[0].isMap()) {
            throw RuntimeError("Built-in has(map, key) expects first argument to be map, got " + arguments[0].typeName() + ".", location);
        }

        const std::string key = requireMapKey(arguments[1], location);
        return Value::boolean(arguments[0].asMap()->entries.find(key) != arguments[0].asMap()->entries.end());
    }

    if (name == "remove") {
        if (arguments.size() != 2) {
            throw RuntimeError("Built-in remove(map, key) expected 2 arguments, got " + std::to_string(arguments.size()) + ".", location);
        }

        if (!arguments[0].isMap()) {
            throw RuntimeError("Built-in remove(map, key) expects first argument to be map, got " + arguments[0].typeName() + ".", location);
        }

        const std::string key = requireMapKey(arguments[1], location);
        auto map = arguments[0].asMap();
        const auto removed = map->entries.erase(key);
        if (removed > 0) {
            auto& keys = map->keys;
            keys.erase(std::remove(keys.begin(), keys.end(), key), keys.end());
        }
        return Value::nil();
    }

    throw RuntimeError("Unknown built-in function '" + name + "'.", location);
}

bool Interpreter::isBuiltinName(const std::string& name) const {
    return name == "len" || name == "push" || name == "pop" || name == "type" || name == "keys" || name == "values" || name == "has" || name == "remove" || name == "__upper" || name == "__lower" || name == "__contains" || name == "__starts_with" || name == "__ends_with" || name == "__trim" || name == "__strlen" || name == "__split" || name == "__replace" || name == "__substring" || name == "__char_at" || name == "__index_of" || name == "__sqrt" || name == "__abs" || name == "__min" || name == "__max" || name == "__round" || name == "__hex_encode" || name == "__hex_decode" || name == "__base64_encode" || name == "__base64_decode" || name == "__now" || name == "__timestamp" || name == "__path_join" || name == "__path_basename" || name == "__path_dirname" || name == "__path_extension" || name == "__read_file" || name == "__write_file" || name == "__append_file" || name == "__exists" || name == "__is_file" || name == "__is_dir" || name == "__list_dir" || name == "__file_size" || name == "__remove_file";
}

std::size_t Interpreter::requireIndex(const Value& value, std::size_t size, const SourceLocation& location) {
    if (!value.isNumber()) {
        throw RuntimeError("Array index must be a number, got " + value.typeName() + ".", location);
    }

    const double number = value.asNumber();
    if (!std::isfinite(number) || std::floor(number) != number) {
        throw RuntimeError("Array index must be a whole number.", location);
    }

    if (number < 0) {
        throw RuntimeError("Array index cannot be negative.", location);
    }

    const auto index = static_cast<std::size_t>(number);
    if (index >= size) {
        throw RuntimeError("Array index " + std::to_string(index) + " is out of range for length " + std::to_string(size) + ".", location);
    }

    return index;
}

std::string Interpreter::requireMapKey(const Value& value, const SourceLocation& location) {
    if (!value.isString()) {
        throw RuntimeError("Map key must be a string, got " + value.typeName() + ".", location);
    }

    return value.asString();
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

    if (left.isFunction()) {
        return left.asFunction() == right.asFunction();
    }

    if (left.isArray()) {
        return left.asArray() == right.asArray();
    }

    if (left.isMap()) {
        return left.asMap() == right.asMap();
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
