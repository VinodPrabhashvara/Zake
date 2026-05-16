#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>

namespace zake {

struct ProjectConfig {
    std::string name;
    std::string version;
    std::filesystem::path mainFile;
    std::filesystem::path sourcePath = "src";
    std::filesystem::path testsPath = "tests";
    std::filesystem::path stdlibPath = "stdlib";
    std::filesystem::path root;
    std::filesystem::path configPath;
};

void printProjectHelp(std::ostream& output);
std::filesystem::path findProjectConfig(const std::filesystem::path& start);
ProjectConfig loadProjectConfig(const std::filesystem::path& configPath);
ProjectConfig loadProjectFromCurrentDirectory();
void initProject(const std::string& projectName, std::ostream& output);

} // namespace zake
