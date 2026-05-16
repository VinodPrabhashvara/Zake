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

Value Value::function(std::shared_ptr<FunctionValue> value) {
    return Value(std::move(value));
}

Value Value::array(std::shared_ptr<ArrayValue> value) {
    return Value(std::move(value));
}

Value Value::map(std::shared_ptr<MapValue> value) {
    return Value(std::move(value));
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
    if (std::holds_alternative<std::shared_ptr<FunctionValue>>(data_)) {
        return ValueType::Function;
    }
    if (std::holds_alternative<std::shared_ptr<ArrayValue>>(data_)) {
        return ValueType::Array;
    }
    if (std::holds_alternative<std::shared_ptr<MapValue>>(data_)) {
        return ValueType::Map;
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

bool Value::isFunction() const {
    return type() == ValueType::Function;
}

bool Value::isArray() const {
    return type() == ValueType::Array;
}

bool Value::isMap() const {
    return type() == ValueType::Map;
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

const std::shared_ptr<FunctionValue>& Value::asFunction() const {
    if (!std::holds_alternative<std::shared_ptr<FunctionValue>>(data_)) {
        throw std::logic_error("Value is not a function.");
    }
    return std::get<std::shared_ptr<FunctionValue>>(data_);
}

const std::shared_ptr<ArrayValue>& Value::asArray() const {
    if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(data_)) {
        throw std::logic_error("Value is not an array.");
    }
    return std::get<std::shared_ptr<ArrayValue>>(data_);
}

const std::shared_ptr<MapValue>& Value::asMap() const {
    if (!std::holds_alternative<std::shared_ptr<MapValue>>(data_)) {
        throw std::logic_error("Value is not a map.");
    }
    return std::get<std::shared_ptr<MapValue>>(data_);
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

    if (isFunction()) {
        return "<fn " + asFunction()->name + ">";
    }

    if (isArray()) {
        std::ostringstream stream;
        stream << "[";
        const auto& elements = asArray()->elements;
        for (std::size_t index = 0; index < elements.size(); ++index) {
            if (index > 0) {
                stream << ", ";
            }
            stream << elements[index].toString();
        }
        stream << "]";
        return stream.str();
    }

    if (isMap()) {
        std::ostringstream stream;
        stream << "{";
        const auto& map = asMap();
        for (std::size_t index = 0; index < map->keys.size(); ++index) {
            if (index > 0) {
                stream << ", ";
            }
            const std::string& key = map->keys[index];
            stream << key << ": " << map->entries.at(key).toString();
        }
        stream << "}";
        return stream.str();
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
    case ValueType::Function:
        return "function";
    case ValueType::Array:
        return "array";
    case ValueType::Map:
        return "map";
    case ValueType::Nil:
        return "nil";
    }

    return "unknown";
}

} // namespace zake
