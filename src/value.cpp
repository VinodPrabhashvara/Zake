#include "zake/value.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace zake {

Value::Value()
    : data_(std::monostate {}) {}

Value::Value(Storage data)
    : data_(std::move(data)) {}

Value Value::number(double value) {
    return Value(value);
}

Value Value::string(std::string value) {
    return Value(std::move(value));
}

Value Value::boolean(bool value) {
    return Value(value);
}

Value Value::nil() {
    return Value();
}

ValueType Value::type() const {
    if (std::holds_alternative<double>(data_)) {
        return ValueType::Number;
    }
    if (std::holds_alternative<std::string>(data_)) {
        return ValueType::String;
    }
    if (std::holds_alternative<bool>(data_)) {
        return ValueType::Boolean;
    }
    return ValueType::Nil;
}

bool Value::isNumber() const {
    return type() == ValueType::Number;
}

bool Value::isString() const {
    return type() == ValueType::String;
}

bool Value::isBoolean() const {
    return type() == ValueType::Boolean;
}

bool Value::isNil() const {
    return type() == ValueType::Nil;
}

double Value::asNumber() const {
    if (!std::holds_alternative<double>(data_)) {
        throw std::logic_error("Value is not a number.");
    }
    return std::get<double>(data_);
}

const std::string& Value::asString() const {
    if (!std::holds_alternative<std::string>(data_)) {
        throw std::logic_error("Value is not a string.");
    }
    return std::get<std::string>(data_);
}

bool Value::asBoolean() const {
    if (!std::holds_alternative<bool>(data_)) {
        throw std::logic_error("Value is not a boolean.");
    }
    return std::get<bool>(data_);
}

std::string Value::toString() const {
    if (isNumber()) {
        std::ostringstream stream;
        stream << std::setprecision(15) << asNumber();
        std::string text = stream.str();

        if (text.find('.') != std::string::npos) {
            while (!text.empty() && text.back() == '0') {
                text.pop_back();
            }
            if (!text.empty() && text.back() == '.') {
                text.pop_back();
            }
        }

        return text;
    }

    if (isString()) {
        return asString();
    }

    if (isBoolean()) {
        return asBoolean() ? "true" : "false";
    }

    return "nil";
}

std::string Value::typeName() const {
    switch (type()) {
    case ValueType::Number:
        return "number";
    case ValueType::String:
        return "string";
    case ValueType::Boolean:
        return "boolean";
    case ValueType::Nil:
        return "nil";
    }

    return "unknown";
}

} // namespace zake
