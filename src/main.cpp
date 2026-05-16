#include "zake/debug_printer.hpp"
#include "zake/formatter.hpp"
#include "zake/interpreter.hpp"
#include "zake/linter.hpp"
#include "zake/lexer.hpp"
#include "zake/parser.hpp"
#include "zake/project.hpp"
#include "zake/repl.hpp"
#include "zake/version.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

enum class Command {
    Run,
    Help,
    Version,
    Tokens,
    Ast,
    Check,
    Test,
    TestHelp,
    Fmt,
    FmtHelp,
    Lint,
    LintHelp,
    Repl,
    ReplHelp,
    Init,
    ProjectRun,
    ProjectCheck,
    ProjectHelp
};

struct CliRequest {
    Command command = Command::Run;
    std::string path;
    bool verbose = false;
    bool checkMode = false;
    bool strict = false;
};

struct TestResult {
    std::filesystem::path path;
    bool passed = false;
    std::string output;
    std::string error;
};

void printHelp() {
    std::cout
        << "Zake - A modern scripting language for builders and ethical hackers.\n\n"
        << "Usage:\n"
        << "  zake <file.zk>\n"
        << "  zake [options] <file.zk>\n"
        << "  zake test [path] [--verbose]\n"
        << "  zake fmt <path> [--check]\n"
        << "  zake lint <path> [--strict]\n"
        << "  zake repl\n"
        << "  zake init [project-name]\n"
        << "  zake run\n"
        << "  zake check\n\n"
        << "Commands:\n"
        << "  test             Run Zake test files\n"
        << "  fmt              Format Zake source files\n"
        << "  lint             Lint Zake source files\n"
        << "  repl             Start the interactive REPL\n"
        << "  init             Create a basic Zake project\n"
        << "  run              Run the current Zake project\n"
        << "  check            Parse/check the current Zake project\n"
        << "  project --help   Show project command help\n\n"
        << "Options:\n"
        << "  -h, --help       Show help information\n"
        << "  -v, --version    Show Zake version\n"
        << "  --tokens         Print lexer tokens for a file\n"
        << "  --ast            Print parsed AST for a file\n"
        << "  --check          Parse/check a file without running it\n\n"
        << "Examples:\n"
        << "  zake examples/hello.zk\n"
        << "  zake --tokens examples/hello.zk\n"
        << "  zake --ast examples/hello.zk\n"
        << "  zake --check examples/hello.zk\n"
        << "  zake test\n"
        << "  zake test tests/pass --verbose\n"
        << "  zake fmt examples/hello.zk --check\n"
        << "  zake lint examples/hello.zk\n"
        << "  zake repl\n"
        << "  zake init sample-project\n"
        << "  zake run\n";
}

void printTestHelp() {
    std::cout
        << "Zake test runner\n\n"
        << "Usage:\n"
        << "  zake test\n"
        << "  zake test <folder>\n"
        << "  zake test <file.zk>\n"
        << "  zake test [path] --verbose\n\n"
        << "Options:\n"
        << "  --verbose        Print output for each test file\n"
        << "  --help           Show test help\n\n"
        << "Examples:\n"
        << "  zake test\n"
        << "  zake test tests\n"
        << "  zake test tests/pass\n"
        << "  zake test tests/pass/hello.zk --verbose\n";
}

void printFmtHelp() {
    std::cout
        << "Zake formatter\n\n"
        << "Usage:\n"
        << "  zake fmt <file.zk>\n"
        << "  zake fmt <folder>\n"
        << "  zake fmt <file.zk> --check\n"
        << "  zake fmt <folder> --check\n\n"
        << "Options:\n"
        << "  --check          Check formatting without changing files\n"
        << "  --help           Show formatter help\n\n"
        << "Examples:\n"
        << "  zake fmt examples/hello.zk\n"
        << "  zake fmt examples --check\n";
}

void printLintHelp() {
    std::cout
        << "Zake linter\n\n"
        << "Usage:\n"
        << "  zake lint <file.zk>\n"
        << "  zake lint <folder>\n"
        << "  zake lint <file.zk> --strict\n"
        << "  zake lint <folder> --strict\n\n"
        << "Options:\n"
        << "  --strict         Treat warnings as command failures\n"
        << "  --help           Show linter help\n\n"
        << "Examples:\n"
        << "  zake lint examples/hello.zk\n"
        << "  zake lint tests/linter --strict\n";
}

void printVersion() {
    std::cout << "Zake " << zake::kZakeVersion << '\n';
}

void printUsageHint() {
    std::cerr << "Run 'zake --help' for usage.\n";
}

void reportCliError(const std::string& message) {
    std::cerr << "zake: error: " << message << '\n';
    printUsageHint();
}

std::string formatError(const std::string& path, const std::string& stage, const zake::ZakeError& error) {
    std::ostringstream output;
    output << path
           << ":" << error.location().line
           << ":" << error.location().column
           << ": " << stage << " error: "
           << error.what();
    return output.str();
}

void reportError(const std::string& path, const std::string& stage, const zake::ZakeError& error) {
    std::cerr << formatError(path, stage, error) << '\n';
}

bool startsWithDash(const std::string& text) {
    return !text.empty() && text[0] == '-';
}

bool pathHasComponent(const std::filesystem::path& path, const std::string& component) {
    for (const auto& part : path) {
        if (part.string() == component) {
            return true;
        }
    }
    return false;
}

bool isExpectedFailureTest(const std::filesystem::path& path) {
    return pathHasComponent(path, "fail");
}

bool validateInputFile(const std::string& path) {
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        std::cerr << "zake: error: file does not exist: " << path << '\n';
        return false;
    }

    if (error) {
        std::cerr << "zake: error: could not inspect file: " << path << ": " << error.message() << '\n';
        return false;
    }

    if (!std::filesystem::is_regular_file(path, error)) {
        std::cerr << "zake: error: path is not a file: " << path << '\n';
        return false;
    }

    if (error) {
        std::cerr << "zake: error: could not inspect file: " << path << ": " << error.message() << '\n';
        return false;
    }

    return true;
}

std::string readSourceFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Could not open file '" + path + "'.");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::vector<zake::Token> lexSource(const std::string& source) {
    zake::Lexer lexer(source);
    return lexer.scanTokens();
}

std::vector<std::unique_ptr<zake::Stmt>> parseTokens(std::vector<zake::Token> tokens) {
    zake::Parser parser(std::move(tokens));
    return parser.parse();
}

std::string displayPath(const std::filesystem::path& path) {
    std::error_code error;
    const auto relative = std::filesystem::relative(path, std::filesystem::current_path(), error);
    if (!error && !relative.empty()) {
        return relative.generic_string();
    }
    return path.generic_string();
}

std::vector<std::filesystem::path> discoverTestFiles(const std::string& requestedPath) {
    const std::filesystem::path path = requestedPath.empty() ? std::filesystem::path("tests") : std::filesystem::path(requestedPath);
    std::error_code error;

    if (!std::filesystem::exists(path, error)) {
        throw std::runtime_error("test path does not exist: " + path.string());
    }
    if (error) {
        throw std::runtime_error("could not inspect test path: " + path.string() + ": " + error.message());
    }

    std::vector<std::filesystem::path> files;
    if (std::filesystem::is_regular_file(path, error)) {
        if (path.extension() != ".zk") {
            throw std::runtime_error("test file must use .zk extension: " + path.string());
        }
        files.push_back(path);
    } else if (std::filesystem::is_directory(path, error)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".zk") {
                files.push_back(entry.path());
            }
        }
    } else {
        throw std::runtime_error("test path is not a file or folder: " + path.string());
    }

    if (error) {
        throw std::runtime_error("could not inspect test path: " + path.string() + ": " + error.message());
    }

    std::sort(files.begin(), files.end(), [](const auto& left, const auto& right) {
        return left.generic_string() < right.generic_string();
    });
    return files;
}

bool shouldSkipFormatDirectory(const std::filesystem::path& path) {
    const std::string name = path.filename().string();
    return name == "build" || (!name.empty() && name[0] == '.');
}

std::filesystem::path findProjectRoot(const std::filesystem::path& startPath) {
    std::error_code error;
    std::filesystem::path current = std::filesystem::absolute(startPath, error);
    if (error) {
        current = std::filesystem::current_path();
    }

    if (std::filesystem::is_regular_file(current, error)) {
        current = current.parent_path();
    }

    while (!current.empty()) {
        if (std::filesystem::exists(current / "stdlib", error) ||
            std::filesystem::exists(current / "CMakeLists.txt", error)) {
            return current;
        }

        const auto parent = current.parent_path();
        if (parent == current) {
            break;
        }
        current = parent;
    }

    return std::filesystem::current_path();
}

std::filesystem::path projectMainPath(const zake::ProjectConfig& config) {
    return (config.root / config.mainFile).lexically_normal();
}

bool validateProjectMainFile(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        std::cerr << "zake: error: project main file does not exist: " << path.string() << '\n';
        return false;
    }
    if (error) {
        std::cerr << "zake: error: could not inspect project main file: " << path.string() << ": " << error.message() << '\n';
        return false;
    }
    if (!std::filesystem::is_regular_file(path, error)) {
        std::cerr << "zake: error: project main path is not a file: " << path.string() << '\n';
        return false;
    }
    if (error) {
        std::cerr << "zake: error: could not inspect project main file: " << path.string() << ": " << error.message() << '\n';
        return false;
    }
    return true;
}

std::vector<std::filesystem::path> discoverFormatFiles(const std::string& requestedPath) {
    const std::filesystem::path path = requestedPath;
    std::error_code error;

    if (!std::filesystem::exists(path, error)) {
        throw std::runtime_error("format path does not exist: " + path.string());
    }
    if (error) {
        throw std::runtime_error("could not inspect format path: " + path.string() + ": " + error.message());
    }

    std::vector<std::filesystem::path> files;
    if (std::filesystem::is_regular_file(path, error)) {
        if (path.extension() != ".zk") {
            throw std::runtime_error("format file must use .zk extension: " + path.string());
        }
        files.push_back(path);
    } else if (std::filesystem::is_directory(path, error)) {
        std::filesystem::recursive_directory_iterator iterator(path, error);
        const std::filesystem::recursive_directory_iterator end;
        while (!error && iterator != end) {
            const auto entry = *iterator;
            if (entry.is_directory(error)) {
                if (shouldSkipFormatDirectory(entry.path())) {
                    iterator.disable_recursion_pending();
                }
            } else if (entry.is_regular_file(error) && entry.path().extension() == ".zk") {
                files.push_back(entry.path());
            }
            iterator.increment(error);
        }
    } else {
        throw std::runtime_error("format path is not a file or folder: " + path.string());
    }

    if (error) {
        throw std::runtime_error("could not inspect format path: " + path.string() + ": " + error.message());
    }

    std::sort(files.begin(), files.end(), [](const auto& left, const auto& right) {
        return left.generic_string() < right.generic_string();
    });
    return files;
}

std::vector<std::filesystem::path> discoverLintFiles(const std::string& requestedPath) {
    const std::filesystem::path path = requestedPath;
    std::error_code error;

    if (!std::filesystem::exists(path, error)) {
        throw std::runtime_error("lint path does not exist: " + path.string());
    }
    if (error) {
        throw std::runtime_error("could not inspect lint path: " + path.string() + ": " + error.message());
    }

    std::vector<std::filesystem::path> files;
    if (std::filesystem::is_regular_file(path, error)) {
        if (path.extension() != ".zk") {
            throw std::runtime_error("lint file must use .zk extension: " + path.string());
        }
        files.push_back(path);
    } else if (std::filesystem::is_directory(path, error)) {
        std::filesystem::recursive_directory_iterator iterator(path, error);
        const std::filesystem::recursive_directory_iterator end;
        while (!error && iterator != end) {
            const auto entry = *iterator;
            if (entry.is_directory(error)) {
                if (shouldSkipFormatDirectory(entry.path())) {
                    iterator.disable_recursion_pending();
                }
            } else if (entry.is_regular_file(error) && entry.path().extension() == ".zk") {
                files.push_back(entry.path());
            }
            iterator.increment(error);
        }
    } else {
        throw std::runtime_error("lint path is not a file or folder: " + path.string());
    }

    if (error) {
        throw std::runtime_error("could not inspect lint path: " + path.string() + ": " + error.message());
    }

    std::sort(files.begin(), files.end(), [](const auto& left, const auto& right) {
        return left.generic_string() < right.generic_string();
    });
    return files;
}

void writeSourceFile(const std::filesystem::path& path, const std::string& source) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("could not write file: " + path.string());
    }
    output << source;
}

TestResult runSingleTest(const std::filesystem::path& path) {
    TestResult result;
    result.path = path;

    std::ostringstream output;
    std::ostringstream errors;
    auto* previousOut = std::cout.rdbuf(output.rdbuf());
    auto* previousErr = std::cerr.rdbuf(errors.rdbuf());

    try {
        const std::string filePath = path.string();
        zake::Interpreter interpreter(filePath);
        interpreter.executeFile(filePath);
        result.passed = true;
    } catch (const zake::LexerError& error) {
        result.passed = false;
        errors << formatError(path.string(), "lexer", error) << '\n';
    } catch (const zake::ParseError& error) {
        result.passed = false;
        errors << formatError(path.string(), "parser", error) << '\n';
    } catch (const zake::RuntimeError& error) {
        result.passed = false;
        errors << formatError(path.string(), "runtime", error) << '\n';
    } catch (const std::exception& error) {
        result.passed = false;
        errors << path.string() << ": " << error.what() << '\n';
    }

    std::cout.rdbuf(previousOut);
    std::cerr.rdbuf(previousErr);
    result.output = output.str();
    result.error = errors.str();
    return result;
}

CliRequest parseArguments(int argc, char* argv[]) {
    if (argc == 1) {
        throw std::runtime_error("no input file provided.");
    }

    if (std::string(argv[1]) == "fmt") {
        CliRequest request;
        request.command = Command::Fmt;

        bool hasPath = false;
        for (int index = 2; index < argc; ++index) {
            const std::string arg = argv[index];
            if (arg == "--help" || arg == "-h") {
                if (argc != 3) {
                    throw std::runtime_error("fmt --help cannot be combined with other arguments.");
                }
                request.command = Command::FmtHelp;
                return request;
            }
            if (arg == "--check") {
                request.checkMode = true;
                continue;
            }
            if (startsWithDash(arg)) {
                throw std::runtime_error("unknown fmt option: " + arg);
            }
            if (hasPath) {
                throw std::runtime_error("fmt accepts at most one path.");
            }
            request.path = arg;
            hasPath = true;
        }

        if (!hasPath) {
            throw std::runtime_error("fmt requires a file or folder.");
        }

        return request;
    }

    if (std::string(argv[1]) == "lint") {
        CliRequest request;
        request.command = Command::Lint;

        bool hasPath = false;
        for (int index = 2; index < argc; ++index) {
            const std::string arg = argv[index];
            if (arg == "--help" || arg == "-h") {
                if (argc != 3) {
                    throw std::runtime_error("lint --help cannot be combined with other arguments.");
                }
                request.command = Command::LintHelp;
                return request;
            }
            if (arg == "--strict") {
                request.strict = true;
                continue;
            }
            if (startsWithDash(arg)) {
                throw std::runtime_error("unknown lint option: " + arg);
            }
            if (hasPath) {
                throw std::runtime_error("lint accepts at most one path.");
            }
            request.path = arg;
            hasPath = true;
        }

        if (!hasPath) {
            throw std::runtime_error("lint requires a file or folder.");
        }

        return request;
    }

    if (std::string(argv[1]) == "repl") {
        CliRequest request;
        request.command = Command::Repl;

        if (argc == 2) {
            return request;
        }

        if (argc == 3 && (std::string(argv[2]) == "--help" || std::string(argv[2]) == "-h")) {
            request.command = Command::ReplHelp;
            return request;
        }

        throw std::runtime_error("repl accepts only --help.");
    }

    if (std::string(argv[1]) == "init") {
        if (argc > 3) {
            throw std::runtime_error("init accepts at most one project name.");
        }

        CliRequest request;
        request.command = Command::Init;
        if (argc == 3) {
            request.path = argv[2];
            if (startsWithDash(request.path)) {
                throw std::runtime_error("project name cannot start with '-'.");
            }
        }
        return request;
    }

    if (std::string(argv[1]) == "run") {
        if (argc != 2) {
            throw std::runtime_error("run does not accept arguments yet.");
        }
        return CliRequest {Command::ProjectRun, "", false};
    }

    if (std::string(argv[1]) == "check") {
        if (argc != 2) {
            throw std::runtime_error("check does not accept arguments yet. Use --check <file.zk> for single files.");
        }
        return CliRequest {Command::ProjectCheck, "", false};
    }

    if (std::string(argv[1]) == "project") {
        if (argc == 2 || (argc == 3 && (std::string(argv[2]) == "--help" || std::string(argv[2]) == "-h"))) {
            return CliRequest {Command::ProjectHelp, "", false};
        }
        throw std::runtime_error("project accepts only --help.");
    }

    if (std::string(argv[1]) == "test") {
        CliRequest request;
        request.command = Command::Test;
        request.path = "tests";

        bool hasPath = false;
        for (int index = 2; index < argc; ++index) {
            const std::string arg = argv[index];
            if (arg == "--help" || arg == "-h") {
                if (argc != 3) {
                    throw std::runtime_error("test --help cannot be combined with other arguments.");
                }
                request.command = Command::TestHelp;
                return request;
            }
            if (arg == "--verbose") {
                request.verbose = true;
                continue;
            }
            if (startsWithDash(arg)) {
                throw std::runtime_error("unknown test option: " + arg);
            }
            if (hasPath) {
                throw std::runtime_error("test accepts at most one path.");
            }
            request.path = arg;
            hasPath = true;
        }

        return request;
    }

    if (argc == 2) {
        const std::string arg = argv[1];
        if (arg == "-h" || arg == "--help") {
            return CliRequest {Command::Help, "", false};
        }
        if (arg == "-v" || arg == "--version") {
            return CliRequest {Command::Version, "", false};
        }
        if (arg == "--tokens" || arg == "--ast" || arg == "--check") {
            throw std::runtime_error(arg + " requires a file.");
        }
        if (startsWithDash(arg)) {
            throw std::runtime_error("unknown option: " + arg);
        }
        return CliRequest {Command::Run, arg, false};
    }

    if (argc == 3) {
        const std::string option = argv[1];
        const std::string path = argv[2];
        if (option == "--tokens") {
            return CliRequest {Command::Tokens, path, false};
        }
        if (option == "--ast") {
            return CliRequest {Command::Ast, path, false};
        }
        if (option == "--check") {
            return CliRequest {Command::Check, path, false};
        }
        if (startsWithDash(option)) {
            throw std::runtime_error("unknown option: " + option);
        }
        throw std::runtime_error("too many arguments.");
    }

    throw std::runtime_error("too many arguments.");
}

int runDiagnosticsCommand(const CliRequest& request) {
    if (!validateInputFile(request.path)) {
        printUsageHint();
        return 1;
    }

    try {
        const std::string source = readSourceFile(request.path);
        auto tokens = lexSource(source);

        if (request.command == Command::Tokens) {
            for (const auto& token : tokens) {
                std::cout << zake::formatToken(token) << '\n';
            }
            return 0;
        }

        auto statements = parseTokens(std::move(tokens));

        if (request.command == Command::Ast) {
            std::cout << zake::formatAst(statements);
            return 0;
        }

        std::cout << "OK: " << request.path << '\n';
        return 0;
    } catch (const zake::LexerError& error) {
        reportError(request.path, "lexer", error);
    } catch (const zake::ParseError& error) {
        reportError(request.path, "parser", error);
    } catch (const std::exception& error) {
        std::cerr << request.path << ": " << error.what() << '\n';
    }

    return 1;
}

int runFile(const std::string& path) {
    if (!validateInputFile(path)) {
        printUsageHint();
        return 1;
    }

    try {
        zake::Interpreter interpreter(path);
        interpreter.executeFile(path);
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

int runProjectInit(const CliRequest& request) {
    try {
        zake::initProject(request.path, std::cout);
        return 0;
    } catch (const std::exception& error) {
        reportCliError(error.what());
        return 1;
    }
}

int runProjectFile(bool execute) {
    std::filesystem::path activePath = "zake.toml";
    try {
        const auto config = zake::loadProjectFromCurrentDirectory();
        const auto mainFile = projectMainPath(config);
        activePath = mainFile;
        if (!validateProjectMainFile(mainFile)) {
            return 1;
        }

        if (!execute) {
            const std::string source = readSourceFile(mainFile.string());
            auto tokens = lexSource(source);
            auto statements = parseTokens(std::move(tokens));
            (void)statements;
            std::cout << "OK: " << displayPath(mainFile) << '\n';
            return 0;
        }

        const auto stdlibRoot = (config.root / config.stdlibPath).lexically_normal();
        zake::Interpreter interpreter(mainFile, config.root, stdlibRoot);
        interpreter.executeFile(mainFile);
        return 0;
    } catch (const zake::LexerError& error) {
        reportError(displayPath(activePath), "lexer", error);
    } catch (const zake::ParseError& error) {
        reportError(displayPath(activePath), "parser", error);
    } catch (const zake::RuntimeError& error) {
        reportError(displayPath(activePath), "runtime", error);
    } catch (const std::exception& error) {
        reportCliError(error.what());
    }

    return 1;
}

int runTests(const CliRequest& request) {
    std::vector<std::filesystem::path> files;
    try {
        files = discoverTestFiles(request.path);
    } catch (const std::exception& error) {
        reportCliError(error.what());
        return 1;
    }

    if (files.empty()) {
        std::cerr << "zake: error: no .zk test files found in " << request.path << '\n';
        return 1;
    }

    std::cout << "Running Zake tests...\n\n";

    int passed = 0;
    int failed = 0;
    for (const auto& file : files) {
        const TestResult result = runSingleTest(file);
        const std::string shownPath = displayPath(file);
        const bool expectedFailure = isExpectedFailureTest(file);
        const bool testPassed = expectedFailure ? !result.passed : result.passed;

        if (testPassed) {
            ++passed;
            std::cout << "PASS " << shownPath;
            if (expectedFailure) {
                std::cout << " expected failure";
            }
            std::cout << '\n';
        } else {
            ++failed;
            std::cout << "FAIL " << shownPath;
            if (expectedFailure) {
                std::cout << " unexpectedly passed";
            } else {
                std::cout << " unexpected error";
            }
            std::cout << '\n';
        }

        if (request.verbose) {
            if (!result.output.empty()) {
                std::cout << "Output:\n" << result.output;
                if (result.output.back() != '\n') {
                    std::cout << '\n';
                }
            }
            if (!result.error.empty()) {
                std::cout << (expectedFailure ? "Expected error:\n" : "Error:\n") << result.error;
                if (result.error.back() != '\n') {
                    std::cout << '\n';
                }
            }
        }
    }

    std::cout
        << "\nResult:\n"
        << "  Passed: " << passed << '\n'
        << "  Failed: " << failed << '\n'
        << "  Total: " << (passed + failed) << '\n';

    return failed == 0 ? 0 : 1;
}

int runFormatter(const CliRequest& request) {
    std::vector<std::filesystem::path> files;
    try {
        files = discoverFormatFiles(request.path);
    } catch (const std::exception& error) {
        reportCliError(error.what());
        return 1;
    }

    if (files.empty()) {
        std::cerr << "zake: error: no .zk files found in " << request.path << '\n';
        return 1;
    }

    int changed = 0;
    int failed = 0;

    for (const auto& file : files) {
        try {
            const std::string source = readSourceFile(file.string());
            const std::string formatted = zake::formatSource(source);
            const bool needsFormatting = source != formatted;
            const std::string shownPath = displayPath(file);

            if (request.checkMode) {
                if (needsFormatting) {
                    ++changed;
                    std::cout << "Needs formatting: " << shownPath << '\n';
                } else {
                    std::cout << "Formatted: " << shownPath << '\n';
                }
                continue;
            }

            if (needsFormatting) {
                writeSourceFile(file, formatted);
                ++changed;
                std::cout << "Formatted: " << shownPath << '\n';
            } else {
                std::cout << "Already formatted: " << shownPath << '\n';
            }
        } catch (const std::exception& error) {
            ++failed;
            std::cerr << "zake: error: " << file.string() << ": " << error.what() << '\n';
        }
    }

    if (request.checkMode && changed > 0) {
        std::cout << "\nFormatter check failed: " << changed << " file(s) need formatting.\n";
        return 1;
    }

    if (failed > 0) {
        std::cerr << "\nFormatter failed: " << failed << " file(s) could not be processed.\n";
        return 1;
    }

    if (!request.checkMode) {
        std::cout << "\nFormatter complete: " << changed << " file(s) updated.\n";
    }

    return 0;
}

int runLinter(const CliRequest& request) {
    std::vector<std::filesystem::path> files;
    try {
        files = discoverLintFiles(request.path);
    } catch (const std::exception& error) {
        reportCliError(error.what());
        return 1;
    }

    if (files.empty()) {
        std::cerr << "zake: error: no .zk files found in " << request.path << '\n';
        return 1;
    }

    std::cout << "Linting Zake files...\n\n";

    int warnings = 0;
    int errors = 0;
    const std::filesystem::path projectRoot = findProjectRoot(files.front());

    for (const auto& file : files) {
        try {
            const std::string source = readSourceFile(file.string());
            auto tokens = lexSource(source);
            auto statements = parseTokens(std::move(tokens));
            const zake::LintResult result = zake::lintProgram(statements, file, projectRoot);
            const std::string shownPath = displayPath(file);

            for (const auto& diagnostic : result.diagnostics()) {
                std::cout
                    << zake::lintSeverityName(diagnostic.severity) << ' '
                    << shownPath << ':' << diagnostic.location.line << ':' << diagnostic.location.column
                    << ": " << diagnostic.message << '\n';
            }

            warnings += result.warningCount();
            errors += result.errorCount();
        } catch (const zake::LexerError& error) {
            ++errors;
            std::cout << "ERROR " << formatError(displayPath(file), "lexer", error) << '\n';
        } catch (const zake::ParseError& error) {
            ++errors;
            std::cout << "ERROR " << formatError(displayPath(file), "parser", error) << '\n';
        } catch (const std::exception& error) {
            ++errors;
            std::cout << "ERROR " << displayPath(file) << ": " << error.what() << '\n';
        }
    }

    std::cout
        << "\nResult:\n"
        << "  Warnings: " << warnings << '\n'
        << "  Errors: " << errors << '\n'
        << "  Files: " << files.size() << '\n';

    if (errors > 0 || (request.strict && warnings > 0)) {
        return 1;
    }

    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    CliRequest request;
    try {
        request = parseArguments(argc, argv);
    } catch (const std::exception& error) {
        reportCliError(error.what());
        return 1;
    }

    switch (request.command) {
    case Command::Help:
        printHelp();
        return 0;
    case Command::TestHelp:
        printTestHelp();
        return 0;
    case Command::FmtHelp:
        printFmtHelp();
        return 0;
    case Command::LintHelp:
        printLintHelp();
        return 0;
    case Command::ReplHelp:
        zake::printReplHelp(std::cout);
        return 0;
    case Command::ProjectHelp:
        zake::printProjectHelp(std::cout);
        return 0;
    case Command::Version:
        printVersion();
        return 0;
    case Command::Tokens:
    case Command::Ast:
    case Command::Check:
        return runDiagnosticsCommand(request);
    case Command::Test:
        return runTests(request);
    case Command::Fmt:
        return runFormatter(request);
    case Command::Lint:
        return runLinter(request);
    case Command::Repl:
        return zake::runRepl();
    case Command::Init:
        return runProjectInit(request);
    case Command::ProjectRun:
        return runProjectFile(true);
    case Command::ProjectCheck:
        return runProjectFile(false);
    case Command::Run:
        return runFile(request.path);
    }

    return 1;
}
