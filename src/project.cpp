#include "zake/project.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
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

std::string stripComment(const std::string& text) {
    bool inString = false;
    bool escaped = false;

    for (std::size_t index = 0; index < text.size(); ++index) {
        const char ch = text[index];
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
        } else if (ch == '#') {
            return text.substr(0, index);
        }
    }

    return text;
}

std::string parseQuotedValue(const std::string& value, int line) {
    const std::string trimmed = trim(value);
    if (trimmed.size() < 2 || trimmed.front() != '"' || trimmed.back() != '"') {
        throw std::runtime_error("zake.toml line " + std::to_string(line) + ": expected quoted string value.");
    }

    return trimmed.substr(1, trimmed.size() - 2);
}

void writeTextFile(const std::filesystem::path& path, const std::string& text) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("could not write file: " + path.string());
    }
    output << text;
}

void ensureSafeInitTarget(const std::filesystem::path& root, bool namedProject) {
    std::error_code error;
    if (namedProject && std::filesystem::exists(root, error)) {
        throw std::runtime_error("project path already exists: " + root.string());
    }

    const std::vector<std::filesystem::path> importantFiles = {
        root / "zake.toml",
        root / "src" / "main.zk",
        root / "tests" / "hello_test.zk",
        root / "README.md",
    };

    for (const auto& file : importantFiles) {
        if (std::filesystem::exists(file, error)) {
            throw std::runtime_error("refusing to overwrite existing file: " + file.string());
        }
    }
}

std::string defaultConfig(const std::string& name) {
    std::ostringstream output;
    output
        << "name = \"" << name << "\"\n"
        << "version = \"0.1.0\"\n"
        << "main = \"src/main.zk\"\n\n"
        << "[paths]\n"
        << "source = \"src\"\n"
        << "tests = \"tests\"\n"
        << "stdlib = \"stdlib\"\n";
    return output.str();
}

std::string defaultReadme(const std::string& name) {
    std::ostringstream output;
    output
        << "# " << name << "\n\n"
        << "A Zake project.\n\n"
        << "## Run\n\n"
        << "```powershell\n"
        << "zake run\n"
        << "```\n\n"
        << "## Check\n\n"
        << "```powershell\n"
        << "zake check\n"
        << "```\n";
    return output.str();
}

} // namespace

void printProjectHelp(std::ostream& output) {
    output
        << "Zake project commands\n\n"
        << "Usage:\n"
        << "  zake init <project-name>\n"
        << "  zake init\n"
        << "  zake run\n"
        << "  zake check\n"
        << "  zake project --help\n\n"
        << "Commands:\n"
        << "  init             Create a basic Zake project\n"
        << "  run              Run the main file from zake.toml\n"
        << "  check            Parse/check the main file from zake.toml\n"
        << "  project --help   Show project command help\n";
}

std::filesystem::path findProjectConfig(const std::filesystem::path& start) {
    std::error_code error;
    std::filesystem::path current = std::filesystem::absolute(start, error);
    if (error) {
        current = std::filesystem::current_path();
    }

    if (std::filesystem::is_regular_file(current, error)) {
        current = current.parent_path();
    }

    while (!current.empty()) {
        const auto candidate = current / "zake.toml";
        if (std::filesystem::exists(candidate, error) && std::filesystem::is_regular_file(candidate, error)) {
            return candidate;
        }

        const auto parent = current.parent_path();
        if (parent == current) {
            break;
        }
        current = parent;
    }

    return {};
}

ProjectConfig loadProjectConfig(const std::filesystem::path& configPath) {
    std::ifstream input(configPath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("could not open project config: " + configPath.string());
    }

    ProjectConfig config;
    config.configPath = std::filesystem::absolute(configPath).lexically_normal();
    config.root = config.configPath.parent_path();

    std::string section;
    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = trim(stripComment(line));
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            section = trim(line.substr(1, line.size() - 2));
            if (section != "paths") {
                throw std::runtime_error("zake.toml line " + std::to_string(lineNumber) + ": unsupported section [" + section + "].");
            }
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            throw std::runtime_error("zake.toml line " + std::to_string(lineNumber) + ": expected key = \"value\".");
        }

        const std::string key = trim(line.substr(0, equals));
        const std::string value = parseQuotedValue(line.substr(equals + 1), lineNumber);

        if (section.empty()) {
            if (key == "name") {
                config.name = value;
            } else if (key == "version") {
                config.version = value;
            } else if (key == "main") {
                config.mainFile = value;
            } else {
                throw std::runtime_error("zake.toml line " + std::to_string(lineNumber) + ": unsupported key '" + key + "'.");
            }
        } else if (section == "paths") {
            if (key == "source") {
                config.sourcePath = value;
            } else if (key == "tests") {
                config.testsPath = value;
            } else if (key == "stdlib") {
                config.stdlibPath = value;
            } else {
                throw std::runtime_error("zake.toml line " + std::to_string(lineNumber) + ": unsupported paths key '" + key + "'.");
            }
        }
    }

    if (config.name.empty()) {
        throw std::runtime_error("zake.toml is missing required key: name.");
    }
    if (config.version.empty()) {
        throw std::runtime_error("zake.toml is missing required key: version.");
    }
    if (config.mainFile.empty()) {
        throw std::runtime_error("zake.toml is missing required key: main.");
    }

    return config;
}

ProjectConfig loadProjectFromCurrentDirectory() {
    const auto configPath = findProjectConfig(std::filesystem::current_path());
    if (configPath.empty()) {
        throw std::runtime_error("no zake.toml found. Run 'zake init <project-name>' or use 'zake <file.zk>'.");
    }
    return loadProjectConfig(configPath);
}

void initProject(const std::string& projectName, std::ostream& output) {
    const bool namedProject = !projectName.empty();
    const std::string name = namedProject ? projectName : std::filesystem::current_path().filename().string();
    if (name.empty()) {
        throw std::runtime_error("could not determine project name for current folder.");
    }

    const auto root = namedProject ? (std::filesystem::current_path() / projectName) : std::filesystem::current_path();
    ensureSafeInitTarget(root, namedProject);

    std::filesystem::create_directories(root / "src");
    std::filesystem::create_directories(root / "tests");

    writeTextFile(root / "zake.toml", defaultConfig(name));
    writeTextFile(root / "src" / "main.zk", "print(\"Hello from Zake project\")\n");
    writeTextFile(root / "tests" / "hello_test.zk", "print(\"Project test passed\")\n");
    writeTextFile(root / "README.md", defaultReadme(name));

    output << "Created Zake project: " << name << '\n';
    output << "Project path: " << root.lexically_normal().string() << '\n';
}

} // namespace zake
