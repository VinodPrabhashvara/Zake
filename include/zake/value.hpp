#pragma once

#include <string>
#include <variant>

namespace zake {

enum class ValueType {
    Nil,
    Number,
    String,
    Boolean
};

class Value {
public:
    Value();

    static Value number(double value);
    static Value string(std::string value);
    static Value boolean(bool value);
    static Value nil();

    ValueType type() const;
    bool isNumber() const;
    bool isString() const;
    bool isBoolean() const;
    bool isNil() const;

    double asNumber() const;
    const std::string& asString() const;
    bool asBoolean() const;

    std::string toString() const;
    std::string typeName() const;

private:
    using Storage = std::variant<std::monostate, double, std::string, bool>;

    explicit Value(Storage data);

    Storage data_;
};

} // namespace zake
