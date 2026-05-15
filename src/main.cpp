#include "zake/interpreter.hpp"
#include "zake/lexer.hpp"
#include "zake/parser.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string readFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Could not open file.");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void reportError(const std::string& path, const std::string& stage, const zake::ZakeError& error) {
    std::cerr << path
              << ":" << error.location().line
              << ":" << error.location().column
              << ": " << stage << " error: "
              << error.what()
              << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: zake <path-to-file.zk>\n";
        return 1;
    }

    const std::string path = argv[1];

    try {
        const std::string source = readFile(path);

        zake::Lexer lexer(source);
        const auto tokens = lexer.scanTokens();

        zake::Parser parser(tokens);
        const auto statements = parser.parse();

        zake::Interpreter interpreter;
        interpreter.execute(statements);
        return 0;
    } catch (const zake::LexerError& error) {
        reportError(path, "lexer", error);
    } catch (const zake::ParseError& error) {
        reportError(path, "parser", error);
    } catch (const zake::RuntimeError& error) {
        reportError(path, "runtime", error);
    } catch (const std::exception& error) {
        std::cerr << path << ": " << error.what() << '\n';
    }

    return 1;
}
