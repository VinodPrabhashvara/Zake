#include "zake/lexer.hpp"

#include <cctype>
#include <unordered_map>
#include <utility>

namespace zake {

namespace {

const std::unordered_map<std::string, TokenType> kKeywords = {
    {"and", TokenType::And},
    {"as", TokenType::As},
    {"catch", TokenType::Catch},
    {"else", TokenType::Else},
    {"fn", TokenType::Fn},
    {"from", TokenType::From},
    {"if", TokenType::If},
    {"import", TokenType::Import},
    {"let", TokenType::Let},
    {"not", TokenType::Not},
    {"or", TokenType::Or},
    {"print", TokenType::Print},
    {"return", TokenType::Return},
    {"throw", TokenType::Throw},
    {"try", TokenType::Try},
    {"true", TokenType::True},
    {"false", TokenType::False},
    {"while", TokenType::While},
};

} // namespace

ZakeError::ZakeError(std::string message, SourceLocation location)
    : std::runtime_error(std::move(message)), location_(location) {}

const SourceLocation& ZakeError::location() const noexcept {
    return location_;
}

LexerError::LexerError(std::string message, SourceLocation location)
    : ZakeError(std::move(message), location) {}

ParseError::ParseError(std::string message, SourceLocation location)
    : ZakeError(std::move(message), location) {}

RuntimeError::RuntimeError(std::string message, SourceLocation location)
    : ZakeError(std::move(message), location) {}

std::string token_type_name(TokenType type) {
    switch (type) {
    case TokenType::LeftParen:
        return "(";
    case TokenType::RightParen:
        return ")";
    case TokenType::LeftBrace:
        return "{";
    case TokenType::RightBrace:
        return "}";
    case TokenType::LeftBracket:
        return "[";
    case TokenType::RightBracket:
        return "]";
    case TokenType::Comma:
        return ",";
    case TokenType::Colon:
        return ":";
    case TokenType::Dot:
        return ".";
    case TokenType::Plus:
        return "+";
    case TokenType::Minus:
        return "-";
    case TokenType::Star:
        return "*";
    case TokenType::Slash:
        return "/";
    case TokenType::Bang:
        return "!";
    case TokenType::BangEqual:
        return "!=";
    case TokenType::Equal:
        return "=";
    case TokenType::EqualEqual:
        return "==";
    case TokenType::Greater:
        return ">";
    case TokenType::GreaterEqual:
        return ">=";
    case TokenType::Less:
        return "<";
    case TokenType::LessEqual:
        return "<=";
    case TokenType::Identifier:
        return "identifier";
    case TokenType::Number:
        return "number";
    case TokenType::String:
        return "string";
    case TokenType::And:
        return "and";
    case TokenType::As:
        return "as";
    case TokenType::Catch:
        return "catch";
    case TokenType::Else:
        return "else";
    case TokenType::Fn:
        return "fn";
    case TokenType::From:
        return "from";
    case TokenType::If:
        return "if";
    case TokenType::Import:
        return "import";
    case TokenType::Let:
        return "let";
    case TokenType::Not:
        return "not";
    case TokenType::Or:
        return "or";
    case TokenType::Print:
        return "print";
    case TokenType::Return:
        return "return";
    case TokenType::Throw:
        return "throw";
    case TokenType::Try:
        return "try";
    case TokenType::True:
        return "true";
    case TokenType::False:
        return "false";
    case TokenType::While:
        return "while";
    case TokenType::Newline:
        return "newline";
    case TokenType::EndOfFile:
        return "end of file";
    }

    return "unknown";
}

Lexer::Lexer(std::string source)
    : source_(std::move(source)) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start_ = current_;
        token_line_ = line_;
        token_column_ = column_;
        scanToken();
    }

    tokens_.push_back(Token {TokenType::EndOfFile, "", SourceLocation {line_, column_}});
    return tokens_;
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.size();
}

char Lexer::advance() {
    const char ch = source_[current_++];
    ++column_;
    return ch;
}

char Lexer::peek() const {
    if (isAtEnd()) {
        return '\0';
    }
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.size()) {
        return '\0';
    }
    return source_[current_ + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) {
        return false;
    }
    advance();
    return true;
}

void Lexer::scanToken() {
    const char ch = advance();

    switch (ch) {
    case '(':
        addToken(TokenType::LeftParen, "(");
        return;
    case ')':
        addToken(TokenType::RightParen, ")");
        return;
    case '{':
        addToken(TokenType::LeftBrace, "{");
        return;
    case '}':
        addToken(TokenType::RightBrace, "}");
        return;
    case '[':
        addToken(TokenType::LeftBracket, "[");
        return;
    case ']':
        addToken(TokenType::RightBracket, "]");
        return;
    case ',':
        addToken(TokenType::Comma, ",");
        return;
    case ':':
        addToken(TokenType::Colon, ":");
        return;
    case '.':
        addToken(TokenType::Dot, ".");
        return;
    case '+':
        addToken(TokenType::Plus, "+");
        return;
    case '-':
        addToken(TokenType::Minus, "-");
        return;
    case '*':
        addToken(TokenType::Star, "*");
        return;
    case '!':
        addToken(match('=') ? TokenType::BangEqual : TokenType::Bang, source_.substr(start_, current_ - start_));
        return;
    case '=':
        addToken(match('=') ? TokenType::EqualEqual : TokenType::Equal, source_.substr(start_, current_ - start_));
        return;
    case '>':
        addToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater, source_.substr(start_, current_ - start_));
        return;
    case '<':
        addToken(match('=') ? TokenType::LessEqual : TokenType::Less, source_.substr(start_, current_ - start_));
        return;
    case '/':
        if (match('/')) {
            skipComment();
            return;
        }
        addToken(TokenType::Slash, "/");
        return;
    case '"':
        if (peek() == '"' && peekNext() == '"') {
            advance();
            advance();
            readMultilineString();
            return;
        }
        readString();
        return;
    case ' ':
    case '\t':
        return;
    case '\n':
        addToken(TokenType::Newline, "\\n");
        ++line_;
        column_ = 1;
        return;
    case '\r':
        if (peek() == '\n') {
            advance();
        }
        addToken(TokenType::Newline, "\\n");
        ++line_;
        column_ = 1;
        return;
    default:
        break;
    }

    if (std::isdigit(static_cast<unsigned char>(ch))) {
        readNumber();
        return;
    }

    if (isIdentifierStart(ch)) {
        readIdentifier();
        return;
    }

    throw LexerError("Unexpected character '" + std::string(1, ch) + "'.", SourceLocation {token_line_, token_column_});
}

void Lexer::readString() {
    std::string value;

    while (!isAtEnd()) {
        const char ch = advance();

        if (ch == '"') {
            addToken(TokenType::String, value);
            return;
        }

        if (ch == '\n' || ch == '\r') {
            throw LexerError("Unterminated string literal.", SourceLocation {token_line_, token_column_});
        }

        if (ch == '\\') {
            if (isAtEnd()) {
                throw LexerError("Unterminated string literal.", SourceLocation {token_line_, token_column_});
            }

            const char escaped = advance();
            switch (escaped) {
            case 'n':
                value.push_back('\n');
                break;
            case 't':
                value.push_back('\t');
                break;
            case 'r':
                value.push_back('\r');
                break;
            case '"':
                value.push_back('"');
                break;
            case '\\':
                value.push_back('\\');
                break;
            default:
                value.push_back(escaped);
                break;
            }
            continue;
        }

        value.push_back(ch);
    }

    throw LexerError("Unterminated string literal.", SourceLocation {token_line_, token_column_});
}

void Lexer::readMultilineString() {
    std::string value;

    if (peek() == '\r' || peek() == '\n') {
        const char newline = advance();
        if (newline == '\r' && peek() == '\n') {
            advance();
        }
        ++line_;
        column_ = 1;
    }

    while (!isAtEnd()) {
        const char ch = advance();

        if (ch == '"' && peek() == '"' && peekNext() == '"') {
            advance();
            advance();
            if (!value.empty() && value.back() == '\n') {
                value.pop_back();
            }
            addToken(TokenType::String, value);
            return;
        }

        if (ch == '\n' || ch == '\r') {
            if (ch == '\r' && peek() == '\n') {
                advance();
            }
            value.push_back('\n');
            ++line_;
            column_ = 1;
            continue;
        }

        if (ch == '\\') {
            if (isAtEnd()) {
                throw LexerError("Unterminated multiline string literal.", SourceLocation {token_line_, token_column_});
            }

            const char escaped = advance();
            switch (escaped) {
            case 'n':
                value.push_back('\n');
                break;
            case 't':
                value.push_back('\t');
                break;
            case 'r':
                value.push_back('\r');
                break;
            case '"':
                value.push_back('"');
                break;
            case '\\':
                value.push_back('\\');
                break;
            default:
                value.push_back(escaped);
                break;
            }
            continue;
        }

        value.push_back(ch);
    }

    throw LexerError("Unterminated multiline string literal.", SourceLocation {token_line_, token_column_});
}

void Lexer::readNumber() {
    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    const std::string lexeme = source_.substr(start_, current_ - start_);
    addToken(TokenType::Number, lexeme);
}

void Lexer::readIdentifier() {
    while (isIdentifierPart(peek())) {
        advance();
    }

    const std::string lexeme = source_.substr(start_, current_ - start_);
    const auto keyword = kKeywords.find(lexeme);
    if (keyword != kKeywords.end()) {
        addToken(keyword->second, lexeme);
        return;
    }

    addToken(TokenType::Identifier, lexeme);
}

void Lexer::skipComment() {
    while (!isAtEnd() && peek() != '\n' && peek() != '\r') {
        advance();
    }
}

void Lexer::addToken(TokenType type, const std::string& lexeme) {
    tokens_.push_back(Token {type, lexeme, SourceLocation {token_line_, token_column_}});
}

bool Lexer::isIdentifierStart(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool Lexer::isIdentifierPart(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

} // namespace zake
