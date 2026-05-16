#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

namespace zake {

class FunctionStmt;
struct ArrayValue;
struct MapValue;

enum class ValueType {
    Nil,
    Number,
    String,
    Boolean,
    Function,
    Array,
    Map
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
    static Value array(std::shared_ptr<ArrayValue> value);
    static Value map(std::shared_ptr<MapValue> value);
    static Value nil();

    ValueType type() const;
    bool isNumber() const;
    bool isString() const;
    bool isBoolean() const;
    bool isFunction() const;
    bool isArray() const;
    bool isMap() const;
    bool isNil() const;

    double asNumber() const;
    const std::string& asString() const;
    bool asBoolean() const;
    const std::shared_ptr<FunctionValue>& asFunction() const;
    const std::shared_ptr<ArrayValue>& asArray() const;
    const std::shared_ptr<MapValue>& asMap() const;

    std::string toString() const;
    std::string typeName() const;

private:
    using Storage = std::variant<std::monostate, double, std::string, bool, std::shared_ptr<FunctionValue>, std::shared_ptr<ArrayValue>, std::shared_ptr<MapValue>>;

    explicit Value(Storage data);

    Storage data_;
};

struct ArrayValue {
    std::vector<Value> elements;
};

struct MapValue {
    std::vector<std::string> keys;
    std::unordered_map<std::string, Value> entries;
};

} // namespace zake
