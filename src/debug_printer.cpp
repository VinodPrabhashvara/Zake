#include "zake/debug_printer.hpp"

#include <sstream>

namespace zake {

namespace {

std::string escapeText(const std::string& text) {
    std::string escaped;
    for (char ch : text) {
        switch (ch) {
        case '\n':
            escaped += "\\n";
            break;
        case '\t':
            escaped += "\\t";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

void writeIndent(std::ostringstream& output, int indent) {
    for (int index = 0; index < indent; ++index) {
        output << "  ";
    }
}

void writeLocation(std::ostringstream& output, const SourceLocation& location) {
    output << " @ " << location.line << ":" << location.column;
}

void writeExpr(std::ostringstream& output, const Expr& expression, int indent);
void writeStmt(std::ostringstream& output, const Stmt& statement, int indent);

void writeExprList(std::ostringstream& output, const std::vector<std::unique_ptr<Expr>>& expressions, int indent) {
    for (const auto& expression : expressions) {
        writeExpr(output, *expression, indent);
    }
}

void writeStmtList(std::ostringstream& output, const std::vector<std::unique_ptr<Stmt>>& statements, int indent) {
    for (const auto& statement : statements) {
        writeStmt(output, *statement, indent);
    }
}

void writeExpr(std::ostringstream& output, const Expr& expression, int indent) {
    writeIndent(output, indent);

    if (const auto* literal = dynamic_cast<const LiteralExpr*>(&expression)) {
        output << "Literal(" << literal->value().typeName() << ", \"" << escapeText(literal->value().toString()) << "\")";
        writeLocation(output, literal->location());
        output << '\n';
        return;
    }

    if (const auto* variable = dynamic_cast<const VariableExpr*>(&expression)) {
        output << "Variable(" << variable->name() << ")";
        writeLocation(output, variable->location());
        output << '\n';
        return;
    }

    if (const auto* call = dynamic_cast<const CallExpr*>(&expression)) {
        output << "Call";
        writeLocation(output, call->location());
        output << '\n';
        writeIndent(output, indent + 1);
        output << "callee\n";
        writeExpr(output, call->callee(), indent + 2);
        writeIndent(output, indent + 1);
        output << "arguments\n";
        writeExprList(output, call->arguments(), indent + 2);
        return;
    }

    if (const auto* array = dynamic_cast<const ArrayExpr*>(&expression)) {
        output << "Array";
        writeLocation(output, array->location());
        output << '\n';
        writeExprList(output, array->elements(), indent + 1);
        return;
    }

    if (const auto* map = dynamic_cast<const MapExpr*>(&expression)) {
        output << "Map";
        writeLocation(output, map->location());
        output << '\n';
        for (const auto& entry : map->entries()) {
            writeIndent(output, indent + 1);
            output << "Entry(\"" << escapeText(entry.key) << "\")";
            writeLocation(output, entry.location);
            output << '\n';
            writeExpr(output, *entry.value, indent + 2);
        }
        return;
    }

    if (const auto* index = dynamic_cast<const IndexExpr*>(&expression)) {
        output << "Index";
        writeLocation(output, index->location());
        output << '\n';
        writeIndent(output, indent + 1);
        output << "object\n";
        writeExpr(output, index->object(), indent + 2);
        writeIndent(output, indent + 1);
        output << "index\n";
        writeExpr(output, index->index(), indent + 2);
        return;
    }

    if (const auto* member = dynamic_cast<const MemberExpr*>(&expression)) {
        output << "Member(" << member->member() << ")";
        writeLocation(output, member->location());
        output << '\n';
        writeIndent(output, indent + 1);
        output << "object\n";
        writeExpr(output, member->object(), indent + 2);
        return;
    }

    if (const auto* unary = dynamic_cast<const UnaryExpr*>(&expression)) {
        output << "Unary(" << token_type_name(unary->op()) << ")";
        writeLocation(output, unary->location());
        output << '\n';
        writeExpr(output, unary->right(), indent + 1);
        return;
    }

    if (const auto* binary = dynamic_cast<const BinaryExpr*>(&expression)) {
        output << "Binary(" << token_type_name(binary->op()) << ")";
        writeLocation(output, binary->location());
        output << '\n';
        writeExpr(output, binary->left(), indent + 1);
        writeExpr(output, binary->right(), indent + 1);
        return;
    }

    if (const auto* grouping = dynamic_cast<const GroupingExpr*>(&expression)) {
        output << "Grouping";
        writeLocation(output, grouping->location());
        output << '\n';
        writeExpr(output, grouping->expression(), indent + 1);
        return;
    }

    output << "UnknownExpr\n";
}

void writeStmt(std::ostringstream& output, const Stmt& statement, int indent) {
    writeIndent(output, indent);

    if (const auto* print = dynamic_cast<const PrintStmt*>(&statement)) {
        output << "PrintStmt";
        writeLocation(output, print->location());
        output << '\n';
        writeExpr(output, print->expression(), indent + 1);
        return;
    }

    if (const auto* expression = dynamic_cast<const ExpressionStmt*>(&statement)) {
        output << "ExpressionStmt";
        writeLocation(output, expression->location());
        output << '\n';
        writeExpr(output, expression->expression(), indent + 1);
        return;
    }

    if (const auto* let = dynamic_cast<const LetStmt*>(&statement)) {
        output << "LetStmt(" << let->name() << ")";
        writeLocation(output, let->location());
        output << '\n';
        writeExpr(output, let->initializer(), indent + 1);
        return;
    }

    if (const auto* import = dynamic_cast<const ImportStmt*>(&statement)) {
        output << "ImportStmt(" << (import->isStandardLibrary() ? "std" : "local") << ", \"" << escapeText(import->module()) << "\")";
        if (import->hasAlias()) {
            output << " as " << import->alias();
        }
        if (import->isSelective()) {
            output << " symbols";
            for (const auto& symbol : import->symbols()) {
                output << " " << symbol;
            }
        }
        writeLocation(output, import->location());
        output << '\n';
        return;
    }

    if (const auto* assign = dynamic_cast<const IndexAssignStmt*>(&statement)) {
        output << "IndexAssignStmt";
        writeLocation(output, assign->location());
        output << '\n';
        writeIndent(output, indent + 1);
        output << "object\n";
        writeExpr(output, assign->object(), indent + 2);
        writeIndent(output, indent + 1);
        output << "index\n";
        writeExpr(output, assign->index(), indent + 2);
        writeIndent(output, indent + 1);
        output << "value\n";
        writeExpr(output, assign->value(), indent + 2);
        return;
    }

    if (const auto* function = dynamic_cast<const FunctionStmt*>(&statement)) {
        output << "FunctionStmt(" << function->name() << ")";
        writeLocation(output, function->location());
        output << '\n';
        writeIndent(output, indent + 1);
        output << "parameters";
        for (const auto& parameter : function->parameters()) {
            output << " " << parameter;
        }
        output << '\n';
        writeStmtList(output, function->body(), indent + 1);
        return;
    }

    if (const auto* returnStatement = dynamic_cast<const ReturnStmt*>(&statement)) {
        output << "ReturnStmt";
        writeLocation(output, returnStatement->location());
        output << '\n';
        writeExpr(output, returnStatement->value(), indent + 1);
        return;
    }

    if (const auto* throwStatement = dynamic_cast<const ThrowStmt*>(&statement)) {
        output << "ThrowStmt";
        writeLocation(output, throwStatement->location());
        output << '\n';
        writeExpr(output, throwStatement->value(), indent + 1);
        return;
    }

    if (const auto* block = dynamic_cast<const BlockStmt*>(&statement)) {
        output << "BlockStmt";
        writeLocation(output, block->location());
        output << '\n';
        writeStmtList(output, block->statements(), indent + 1);
        return;
    }

    if (const auto* ifStatement = dynamic_cast<const IfStmt*>(&statement)) {
        output << "IfStmt";
        writeLocation(output, ifStatement->location());
        output << '\n';
        writeIndent(output, indent + 1);
        output << "condition\n";
        writeExpr(output, ifStatement->condition(), indent + 2);
        writeIndent(output, indent + 1);
        output << "then\n";
        writeStmt(output, ifStatement->thenBranch(), indent + 2);
        if (const Stmt* elseBranch = ifStatement->elseBranch()) {
            writeIndent(output, indent + 1);
            output << "else\n";
            writeStmt(output, *elseBranch, indent + 2);
        }
        return;
    }

    if (const auto* whileStatement = dynamic_cast<const WhileStmt*>(&statement)) {
        output << "WhileStmt";
        writeLocation(output, whileStatement->location());
        output << '\n';
        writeIndent(output, indent + 1);
        output << "condition\n";
        writeExpr(output, whileStatement->condition(), indent + 2);
        writeIndent(output, indent + 1);
        output << "body\n";
        writeStmt(output, whileStatement->body(), indent + 2);
        return;
    }

    if (const auto* tryCatch = dynamic_cast<const TryCatchStmt*>(&statement)) {
        output << "TryCatchStmt(catch " << tryCatch->catchName() << ")";
        writeLocation(output, tryCatch->location());
        output << '\n';
        writeIndent(output, indent + 1);
        output << "try\n";
        writeStmtList(output, tryCatch->tryBody(), indent + 2);
        writeIndent(output, indent + 1);
        output << "catch\n";
        writeStmtList(output, tryCatch->catchBody(), indent + 2);
        return;
    }

    output << "UnknownStmt\n";
}

} // namespace

std::string formatToken(const Token& token) {
    std::ostringstream output;
    output << "line=" << token.location.line
           << " column=" << token.location.column
           << " type=" << token_type_name(token.type)
           << " lexeme=\"" << escapeText(token.lexeme) << "\"";
    return output.str();
}

std::string formatAst(const std::vector<std::unique_ptr<Stmt>>& statements) {
    std::ostringstream output;
    output << "Program\n";
    writeStmtList(output, statements, 1);
    return output.str();
}

} // namespace zake
