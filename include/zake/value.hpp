#pragma once

#include <string>
#include <memory>
#include <variant>

namespace zake {

class FunctionStmt;

enum class ValueType {
    Nil,
    Number,
    String,
    Boolean,
    Function
};

struct FunctionValue {
    std::string name;
    const FunctionStmt* declaration = nullptr;
};

class Value {
public:
    Value();

    static Value number(double value);
    static Value string(std::string value);
    static Value boolean(bool value);
    static Value function(std::shared_ptr<FunctionValue> value);
    static Value nil();

    ValueType type() const;
    bool isNumber() const;
    bool isString() const;
    bool isBoolean() const;
    bool isFunction() const;
    bool isNil() const;

    double asNumber() const;
    const std::string& asString() const;
    bool asBoolean() const;
    const std::shared_ptr<FunctionValue>& asFunction() const;

    std::string toString() const;
    std::string typeName() const;

private:
    using Storage = std::variant<std::monostate, double, std::string, bool, std::shared_ptr<FunctionValue>>;

    explicit Value(Storage data);

    Storage data_;
};

} // namespace zake
