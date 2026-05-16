#include "zake/formatter.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace zake {

namespace {

struct LineParts {
    std::string code;
    std::string comment;
};

std::string trim(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(start, end - start);
}

bool isIdentifierStart(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool isIdentifierPart(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

bool isOperator(const std::string& token) {
    return token == "=" || token == "==" || token == "!=" || token == ">" || token == ">=" ||
           token == "<" || token == "<=" || token == "+" || token == "-" || token == "*" ||
           token == "/" || token == "and" || token == "or";
}

bool isWordLike(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    return isIdentifierStart(token[0]) ||
           std::isdigit(static_cast<unsigned char>(token[0])) ||
           token[0] == '"';
}

bool isUnaryMinus(const std::vector<std::string>& tokens, std::size_t index) {
    if (tokens[index] != "-") {
        return false;
    }

    if (index == 0) {
        return true;
    }

    const std::string& previous = tokens[index - 1];
    return previous == "(" || previous == "[" || previous == "{" || previous == "," ||
           previous == ":" || isOperator(previous) || previous == "not" || previous == "!";
}

LineParts splitComment(const std::string& line) {
    bool inString = false;
    bool escaped = false;

    for (std::size_t index = 0; index + 1 < line.size(); ++index) {
        const char ch = line[index];

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

        if (ch == '/' && line[index + 1] == '/') {
            return LineParts {line.substr(0, index), trim(line.substr(index))};
        }
    }

    return LineParts {line, ""};
}

std::vector<std::string> tokenizeCode(const std::string& code) {
    std::vector<std::string> tokens;

    for (std::size_t index = 0; index < code.size();) {
        const char ch = code[index];

        if (std::isspace(static_cast<unsigned char>(ch))) {
            ++index;
            continue;
        }

        if (isIdentifierStart(ch)) {
            const std::size_t start = index++;
            while (index < code.size() && isIdentifierPart(code[index])) {
                ++index;
            }
            tokens.push_back(code.substr(start, index - start));
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(ch))) {
            const std::size_t start = index++;
            while (index < code.size() && std::isdigit(static_cast<unsigned char>(code[index]))) {
                ++index;
            }
            if (index + 1 < code.size() && code[index] == '.' && std::isdigit(static_cast<unsigned char>(code[index + 1]))) {
                ++index;
                while (index < code.size() && std::isdigit(static_cast<unsigned char>(code[index]))) {
                    ++index;
                }
            }
            tokens.push_back(code.substr(start, index - start));
            continue;
        }

        if (ch == '"') {
            const std::size_t start = index++;
            if (index + 1 < code.size() && code[index] == '"' && code[index + 1] == '"') {
                index += 2;
                while (index + 2 < code.size()) {
                    if (code[index] == '"' && code[index + 1] == '"' && code[index + 2] == '"') {
                        index += 3;
                        break;
                    }
                    ++index;
                }
                tokens.push_back(code.substr(start, index - start));
                continue;
            }

            bool escaped = false;
            while (index < code.size()) {
                const char current = code[index++];
                if (escaped) {
                    escaped = false;
                    continue;
                }
                if (current == '\\') {
                    escaped = true;
                    continue;
                }
                if (current == '"') {
                    break;
                }
            }
            tokens.push_back(code.substr(start, index - start));
            continue;
        }

        if (index + 1 < code.size()) {
            const std::string pair = code.substr(index, 2);
            if (pair == "==" || pair == "!=" || pair == ">=" || pair == "<=") {
                tokens.push_back(pair);
                index += 2;
                continue;
            }
        }

        tokens.push_back(std::string(1, ch));
        ++index;
    }

    return tokens;
}

void appendToken(std::string& output, const std::vector<std::string>& tokens, std::size_t index) {
    const std::string& token = tokens[index];
    const std::string previous = index == 0 ? "" : tokens[index - 1];

    if (token == ",") {
        output = trim(output);
        output += ", ";
        return;
    }

    if (token == ":") {
        output = trim(output);
        output += ": ";
        return;
    }

    if (token == ".") {
        output = trim(output);
        output += ".";
        return;
    }

    if (token == ")" || token == "]") {
        output = trim(output);
        output += token;
        return;
    }

    if (token == "(") {
        output = trim(output);
        output += token;
        return;
    }

    if (token == "[") {
        output = trim(output);
        if (!output.empty() && (isOperator(previous) || previous == ":" || previous == ",")) {
            output += ' ';
        }
        output += "[";
        return;
    }

    if (token == "{") {
        output = trim(output);
        if (!output.empty()) {
            output += ' ';
        }
        output += "{";
        return;
    }

    if (token == "}") {
        output = trim(output);
        output += "}";
        return;
    }

    if (token == "!" || token == "not") {
        output = trim(output);
        if (!output.empty() && previous != "(" && previous != "[" && previous != "{") {
            output += ' ';
        }
        output += token;
        if (token == "not") {
            output += ' ';
        }
        return;
    }

    if (token == "-" && isUnaryMinus(tokens, index)) {
        output = trim(output);
        output += "-";
        return;
    }

    if (isOperator(token)) {
        output = trim(output);
        if (!output.empty()) {
            output += ' ';
        }
        output += token;
        output += ' ';
        return;
    }

    const bool needsSpace =
        !output.empty() &&
        previous != "(" &&
        previous != "[" &&
        previous != "{" &&
        previous != "." &&
        previous != "-" &&
        previous != "!" &&
        previous != "not" &&
        (isWordLike(previous) || isWordLike(token) || previous == "}" || token == "else" || token == "catch");

    output = trim(output);
    if (needsSpace) {
        output += ' ';
    }
    output += token;
}

std::string formatCode(const std::string& code) {
    const auto tokens = tokenizeCode(code);
    std::string output;

    for (std::size_t index = 0; index < tokens.size(); ++index) {
        appendToken(output, tokens, index);
    }

    return trim(output);
}

int braceDelta(const std::string& text) {
    int delta = 0;
    bool inString = false;
    bool escaped = false;

    for (char ch : text) {
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
        } else if (ch == '{') {
            ++delta;
        } else if (ch == '}') {
            --delta;
        }
    }

    return delta;
}

int tripleQuoteCount(const std::string& text) {
    int count = 0;
    for (std::size_t index = 0; index + 2 < text.size(); ++index) {
        if (text[index] == '"' && text[index + 1] == '"' && text[index + 2] == '"') {
            ++count;
            index += 2;
        }
    }
    return count;
}

std::string indentText(int indent) {
    return std::string(static_cast<std::size_t>(std::max(0, indent) * 4), ' ');
}

std::vector<std::string> splitLines(std::string source) {
    std::string normalized;
    normalized.reserve(source.size());
    for (std::size_t index = 0; index < source.size(); ++index) {
        if (source[index] == '\r') {
            if (index + 1 < source.size() && source[index + 1] == '\n') {
                continue;
            }
            normalized.push_back('\n');
            continue;
        }
        normalized.push_back(source[index]);
    }

    std::vector<std::string> lines;
    std::istringstream input(normalized);
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }

    if (!normalized.empty() && normalized.back() == '\n') {
        lines.push_back("");
    }

    return lines;
}

} // namespace

std::string formatSource(const std::string& source) {
    const auto lines = splitLines(source);
    std::vector<std::string> formattedLines;
    int indent = 0;
    bool previousBlank = false;
    bool insideTopLevelFunction = false;
    bool justClosedTopLevelFunction = false;
    bool insideMultilineString = false;

    for (const auto& rawLine : lines) {
        if (insideMultilineString) {
            formattedLines.push_back(rawLine);
            previousBlank = false;
            if (tripleQuoteCount(rawLine) % 2 == 1) {
                insideMultilineString = false;
            }
            continue;
        }

        const LineParts parts = splitComment(rawLine);
        const std::string code = trim(parts.code);

        if (code.empty() && parts.comment.empty()) {
            if (!formattedLines.empty() && !previousBlank) {
                formattedLines.push_back("");
                previousBlank = true;
            }
            continue;
        }

        const std::string formattedCode = formatCode(code);
        const bool startsClosingBrace = !formattedCode.empty() && formattedCode.front() == '}';
        const int lineIndent = startsClosingBrace ? std::max(0, indent - 1) : indent;
        const bool topLevelFunctionStart = lineIndent == 0 && formattedCode.rfind("fn ", 0) == 0;

        if (topLevelFunctionStart && justClosedTopLevelFunction && !formattedLines.empty() && !formattedLines.back().empty()) {
            formattedLines.push_back("");
        }

        std::string output = indentText(lineIndent);
        if (!formattedCode.empty()) {
            output += formattedCode;
            if (!parts.comment.empty()) {
                output += ' ';
            }
        }
        output += parts.comment;
        formattedLines.push_back(output);
        previousBlank = false;

        if (topLevelFunctionStart) {
            insideTopLevelFunction = true;
            justClosedTopLevelFunction = false;
        } else if (!topLevelFunctionStart && !formattedCode.empty()) {
            justClosedTopLevelFunction = false;
        }

        indent = std::max(0, indent + braceDelta(formattedCode));
        if (insideTopLevelFunction && indent == 0 && startsClosingBrace) {
            insideTopLevelFunction = false;
            justClosedTopLevelFunction = true;
        }

        if (tripleQuoteCount(rawLine) % 2 == 1) {
            insideMultilineString = true;
        }
    }

    while (!formattedLines.empty() && formattedLines.back().empty()) {
        formattedLines.pop_back();
    }

    std::ostringstream output;
    for (const auto& line : formattedLines) {
        output << line << '\n';
    }
    return output.str();
}

} // namespace zake
