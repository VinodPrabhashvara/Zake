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
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Comma,
    Colon,
    Dot,
    Plus,
    Minus,
    Star,
    Slash,
    Bang,
    BangEqual,
    Equal,
    EqualEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Identifier,
    Number,
    String,
    And,
    As,
    Catch,
    Else,
    Fn,
    From,
    If,
    Import,
    Let,
    Not,
    Or,
    Print,
    Return,
    Throw,
    Try,
    True,
    False,
    While,
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
