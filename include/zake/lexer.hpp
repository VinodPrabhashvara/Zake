#pragma once

#include <string>
#include <vector>

#include "zake/token.hpp"

namespace zake {

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> scanTokens();

private:
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    void scanToken();
    void readString();
    void readNumber();
    void readIdentifier();
    void skipComment();
    void addToken(TokenType type, const std::string& lexeme);
    static bool isIdentifierStart(char ch);
    static bool isIdentifierPart(char ch);

    std::string source_;
    std::vector<Token> tokens_;
    std::size_t start_ = 0;
    std::size_t current_ = 0;
    int line_ = 1;
    int column_ = 1;
    int token_line_ = 1;
    int token_column_ = 1;
};

} // namespace zake
