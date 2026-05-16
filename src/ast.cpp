#include "zake/ast.hpp"

#include <utility>

namespace zake {

Expr::Expr(SourceLocation location)
    : location_(location) {}

Expr::~Expr() = default;

const SourceLocation& Expr::location() const {
    return location_;
}

LiteralExpr::LiteralExpr(SourceLocation location, Value value)
    : Expr(location), value_(std::move(value)) {}

const Value& LiteralExpr::value() const {
    return value_;
}

VariableExpr::VariableExpr(SourceLocation location, std::string name)
    : Expr(location), name_(std::move(name)) {}

const std::string& VariableExpr::name() const {
    return name_;
}

CallExpr::CallExpr(SourceLocation location, std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> arguments)
    : Expr(location), callee_(std::move(callee)), arguments_(std::move(arguments)) {}

const Expr& CallExpr::callee() const {
    return *callee_;
}

const std::vector<std::unique_ptr<Expr>>& CallExpr::arguments() const {
    return arguments_;
}

ArrayExpr::ArrayExpr(SourceLocation location, std::vector<std::unique_ptr<Expr>> elements)
    : Expr(location), elements_(std::move(elements)) {}

const std::vector<std::unique_ptr<Expr>>& ArrayExpr::elements() const {
    return elements_;
}

MapExpr::MapExpr(SourceLocation location, std::vector<MapEntry> entries)
    : Expr(location), entries_(std::move(entries)) {}

const std::vector<MapEntry>& MapExpr::entries() const {
    return entries_;
}

IndexExpr::IndexExpr(SourceLocation location, std::unique_ptr<Expr> object, std::unique_ptr<Expr> index)
    : Expr(location), object_(std::move(object)), index_(std::move(index)) {}

const Expr& IndexExpr::object() const {
    return *object_;
}

const Expr& IndexExpr::index() const {
    return *index_;
}

std::unique_ptr<Expr> IndexExpr::takeObject() {
    return std::move(object_);
}

std::unique_ptr<Expr> IndexExpr::takeIndex() {
    return std::move(index_);
}

MemberExpr::MemberExpr(SourceLocation location, std::unique_ptr<Expr> object, std::string member)
    : Expr(location), object_(std::move(object)), member_(std::move(member)) {}

const Expr& MemberExpr::object() const {
    return *object_;
}

const std::string& MemberExpr::member() const {
    return member_;
}

UnaryExpr::UnaryExpr(SourceLocation location, TokenType op, std::unique_ptr<Expr> right)
    : Expr(location), op_(op), right_(std::move(right)) {}

TokenType UnaryExpr::op() const {
    return op_;
}

const Expr& UnaryExpr::right() const {
    return *right_;
}

BinaryExpr::BinaryExpr(SourceLocation location, std::unique_ptr<Expr> left, TokenType op, std::unique_ptr<Expr> right)
    : Expr(location), left_(std::move(left)), op_(op), right_(std::move(right)) {}

const Expr& BinaryExpr::left() const {
    return *left_;
}

TokenType BinaryExpr::op() const {
    return op_;
}

const Expr& BinaryExpr::right() const {
    return *right_;
}

GroupingExpr::GroupingExpr(SourceLocation location, std::unique_ptr<Expr> expression)
    : Expr(location), expression_(std::move(expression)) {}

const Expr& GroupingExpr::expression() const {
    return *expression_;
}

Stmt::Stmt(SourceLocation location)
    : location_(location) {}

Stmt::~Stmt() = default;

const SourceLocation& Stmt::location() const {
    return location_;
}

PrintStmt::PrintStmt(SourceLocation location, std::unique_ptr<Expr> expression)
    : Stmt(location), expression_(std::move(expression)) {}

const Expr& PrintStmt::expression() const {
    return *expression_;
}

ExpressionStmt::ExpressionStmt(SourceLocation location, std::unique_ptr<Expr> expression)
    : Stmt(location), expression_(std::move(expression)) {}

const Expr& ExpressionStmt::expression() const {
    return *expression_;
}

LetStmt::LetStmt(SourceLocation location, std::string name, std::unique_ptr<Expr> initializer)
    : Stmt(location), name_(std::move(name)), initializer_(std::move(initializer)) {}

const std::string& LetStmt::name() const {
    return name_;
}

const Expr& LetStmt::initializer() const {
    return *initializer_;
}

ImportStmt::ImportStmt(
    SourceLocation location,
    std::string module,
    bool standardLibrary,
    std::string alias,
    std::vector<std::string> symbols)
    : Stmt(location),
      module_(std::move(module)),
      standardLibrary_(standardLibrary),
      alias_(std::move(alias)),
      symbols_(std::move(symbols)) {}

const std::string& ImportStmt::module() const {
    return module_;
}

bool ImportStmt::isStandardLibrary() const {
    return standardLibrary_;
}

const std::string& ImportStmt::alias() const {
    return alias_;
}

const std::vector<std::string>& ImportStmt::symbols() const {
    return symbols_;
}

bool ImportStmt::hasAlias() const {
    return !alias_.empty();
}

bool ImportStmt::isSelective() const {
    return !symbols_.empty();
}

IndexAssignStmt::IndexAssignStmt(SourceLocation location, std::unique_ptr<Expr> object, std::unique_ptr<Expr> index, std::unique_ptr<Expr> value)
    : Stmt(location), object_(std::move(object)), index_(std::move(index)), value_(std::move(value)) {}

const Expr& IndexAssignStmt::object() const {
    return *object_;
}

const Expr& IndexAssignStmt::index() const {
    return *index_;
}

const Expr& IndexAssignStmt::value() const {
    return *value_;
}

FunctionStmt::FunctionStmt(SourceLocation location, std::string name, std::vector<std::string> parameters, std::vector<std::unique_ptr<Stmt>> body)
    : Stmt(location), name_(std::move(name)), parameters_(std::move(parameters)), body_(std::move(body)) {}

const std::string& FunctionStmt::name() const {
    return name_;
}

const std::vector<std::string>& FunctionStmt::parameters() const {
    return parameters_;
}

const std::vector<std::unique_ptr<Stmt>>& FunctionStmt::body() const {
    return body_;
}

ReturnStmt::ReturnStmt(SourceLocation location, std::unique_ptr<Expr> value)
    : Stmt(location), value_(std::move(value)) {}

const Expr& ReturnStmt::value() const {
    return *value_;
}

ThrowStmt::ThrowStmt(SourceLocation location, std::unique_ptr<Expr> value)
    : Stmt(location), value_(std::move(value)) {}

const Expr& ThrowStmt::value() const {
    return *value_;
}

BlockStmt::BlockStmt(SourceLocation location, std::vector<std::unique_ptr<Stmt>> statements)
    : Stmt(location), statements_(std::move(statements)) {}

const std::vector<std::unique_ptr<Stmt>>& BlockStmt::statements() const {
    return statements_;
}

IfStmt::IfStmt(
    SourceLocation location,
    std::unique_ptr<Expr> condition,
    std::unique_ptr<Stmt> thenBranch,
    std::unique_ptr<Stmt> elseBranch)
    : Stmt(location),
      condition_(std::move(condition)),
      thenBranch_(std::move(thenBranch)),
      elseBranch_(std::move(elseBranch)) {}

const Expr& IfStmt::condition() const {
    return *condition_;
}

const Stmt& IfStmt::thenBranch() const {
    return *thenBranch_;
}

const Stmt* IfStmt::elseBranch() const {
    return elseBranch_.get();
}

WhileStmt::WhileStmt(SourceLocation location, std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
    : Stmt(location), condition_(std::move(condition)), body_(std::move(body)) {}

const Expr& WhileStmt::condition() const {
    return *condition_;
}

const Stmt& WhileStmt::body() const {
    return *body_;
}

TryCatchStmt::TryCatchStmt(
    SourceLocation location,
    std::vector<std::unique_ptr<Stmt>> tryBody,
    std::string catchName,
    std::vector<std::unique_ptr<Stmt>> catchBody)
    : Stmt(location),
      tryBody_(std::move(tryBody)),
      catchName_(std::move(catchName)),
      catchBody_(std::move(catchBody)) {}

const std::vector<std::unique_ptr<Stmt>>& TryCatchStmt::tryBody() const {
    return tryBody_;
}

const std::string& TryCatchStmt::catchName() const {
    return catchName_;
}

const std::vector<std::unique_ptr<Stmt>>& TryCatchStmt::catchBody() const {
    return catchBody_;
}

} // namespace zake
