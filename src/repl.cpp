#include "zake/repl.hpp"

#include "zake/interpreter.hpp"
#include "zake/lexer.hpp"
#include "zake/parser.hpp"
#include "zake/version.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace zake {
namespace {

std::string trim(const std::string& text) {
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

std::string formatReplError(const std::string& stage, const ZakeError& error) {
    std::ostringstream output;
    output << "repl:" << error.location().line << ':' << error.location().column
           << ": " << stage << " error: " << error.what();
    return output.str();
}

int braceBalance(const std::string& source) {
    int balance = 0;
    bool inString = false;
    bool escaped = false;

    for (std::size_t index = 0; index < source.size(); ++index) {
        const char ch = source[index];

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

        if (ch == '/' && index + 1 < source.size() && source[index + 1] == '/') {
            while (index < source.size() && source[index] != '\n') {
                ++index;
            }
            continue;
        }

        if (ch == '"') {
            inString = true;
        } else if (ch == '{') {
            ++balance;
        } else if (ch == '}') {
            --balance;
        }
    }

    return balance;
}

void clearScreen(std::ostream& output) {
    for (int line = 0; line < 40; ++line) {
        output << '\n';
    }
}

void printVars(const Interpreter& interpreter, std::ostream& output) {
    const auto globals = interpreter.globalsSnapshot();
    if (globals.empty()) {
        output << "(no variables)\n";
        return;
    }

    for (const auto& entry : globals) {
        output << entry.first << " = " << entry.second << '\n';
    }
}

bool handleDotCommand(const std::string& command, Interpreter& interpreter) {
    if (command == ".exit" || command == ".quit") {
        return false;
    }

    if (command == ".help") {
        printReplHelp(std::cout);
    } else if (command == ".clear") {
        clearScreen(std::cout);
    } else if (command == ".vars") {
        printVars(interpreter, std::cout);
    } else if (command == ".version") {
        std::cout << "Zake " << kZakeVersion << '\n';
    } else {
        std::cerr << "repl: unknown command '" << command << "'. Type .help for commands.\n";
    }

    return true;
}

} // namespace

void printReplHelp(std::ostream& output) {
    output
        << "Zake REPL\n\n"
        << "Usage:\n"
        << "  zake repl\n"
        << "  zake repl --help\n\n"
        << "Commands:\n"
        << "  .help       Show REPL commands\n"
        << "  .exit       Exit the REPL\n"
        << "  .quit       Exit the REPL\n"
        << "  .clear      Clear the screen\n"
        << "  .vars       Show global variables and functions\n"
        << "  .version    Show Zake version\n\n"
        << "Notes:\n"
        << "  Variables and functions stay available until you exit.\n"
        << "  Multi-line blocks continue while braces are not balanced.\n";
}

int runRepl() {
    Interpreter interpreter(std::filesystem::current_path() / "repl.zk");
    std::vector<std::vector<std::unique_ptr<Stmt>>> history;

    std::cout
        << "Zake " << kZakeVersion << " REPL\n"
        << "Type .help for commands, .exit to quit.\n\n";

    std::string buffer;
    std::string line;
    while (true) {
        std::cout << (buffer.empty() ? "zake> " : "....> ");
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }

        const std::string trimmed = trim(line);
        if (buffer.empty() && trimmed.empty()) {
            continue;
        }

        if (buffer.empty() && !trimmed.empty() && trimmed[0] == '.') {
            if (!handleDotCommand(trimmed, interpreter)) {
                break;
            }
            continue;
        }

        buffer += line;
        buffer += '\n';

        if (braceBalance(buffer) > 0) {
            continue;
        }

        try {
            Lexer lexer(buffer);
            auto tokens = lexer.scanTokens();
            Parser parser(std::move(tokens));
            auto statements = parser.parse();

            history.push_back(std::move(statements));
            interpreter.executeInteractive(history.back());
        } catch (const LexerError& error) {
            std::cerr << formatReplError("lexer", error) << '\n';
        } catch (const ParseError& error) {
            std::cerr << formatReplError("parser", error) << '\n';
        } catch (const RuntimeError& error) {
            std::cerr << formatReplError("runtime", error) << '\n';
        } catch (const std::exception& error) {
            std::cerr << "repl: error: " << error.what() << '\n';
        }

        buffer.clear();
    }

    return 0;
}

} // namespace zake
