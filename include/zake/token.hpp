#pragma once

#include <stdexcept>
#include <string>

namespace zake {

struct SourceLocation {
    int line = 1;
    int column = 1;
};

enum class TokenType {
    LeftParen,
    RightParen,
    Plus,
    Minus,
    Star,
    Slash,
    Equal,
    Identifier,
    Number,
    String,
    Let,
    Print,
    True,
    False,
    Newline,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;
};

class ZakeError : public std::runtime_error {
public:
    ZakeError(std::string message, SourceLocation location);

    const SourceLocation& location() const noexcept;

private:
    SourceLocation location_;
};

class LexerError : public ZakeError {
public:
    LexerError(std::string message, SourceLocation location);
};

class ParseError : public ZakeError {
public:
    ParseError(std::string message, SourceLocation location);
};

class RuntimeError : public ZakeError {
public:
    RuntimeError(std::string message, SourceLocation location);
};

std::string token_type_name(TokenType type);

} // namespace zake
