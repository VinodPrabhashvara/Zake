#pragma once

#include <memory>
#include <string>
#include <vector>

#include "zake/ast.hpp"
#include "zake/token.hpp"

namespace zake {

std::string formatToken(const Token& token);
std::string formatAst(const std::vector<std::unique_ptr<Stmt>>& statements);

} // namespace zake
