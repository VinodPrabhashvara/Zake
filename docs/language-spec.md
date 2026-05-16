# Zake Language Spec v0.17

Zake v0.17 is a small interpreter focused on expressions, control flow, functions, arrays, maps, upgraded strings, imports with namespaces and selective imports, basic standard-library modules, safe basic File I/O, real error handling, a professional CLI, a test runner, formatter, linter, REPL, and basic project system.

## File Extension

- Zake source files use the `.zk` extension.

## Statements

Zake v0.17 supports these statement forms:

```zk
import "file.zk"
import "file.zk" as module
import std.module
import std.module as module
from std.module import name, other_name
print(expression)
let name = expression
fn name(parameter, parameter) {
    statement
}
return expression
throw expression
array[index] = expression
map["key"] = expression
if condition {
    statement
} else {
    statement
}
while condition {
    statement
}
try {
    statement
} catch err {
    statement
}
{
    statement
}
```

Statements are separated by newlines or the end of the file.

## Literals

Supported literal types:

- numbers: `0`, `17`, `3.14`
- strings: `"hello"`, `"Hello ${name}"`, and triple-quoted multiline strings
- booleans: `true`, `false`
- arrays: `[10, "hello", true]`
- maps: `{"name": "Vinod", "age": 17}`

## Expressions

Supported expression features:

- variable references: `name`
- grouping with parentheses: `(1 + 2) * 3`
- unary minus: `-5`
- logical not: `not ready`
- arithmetic: `+`, `-`, `*`, `/`
- string concatenation with `+`
- comparison: `==`, `!=`, `>`, `>=`, `<`, `<=`
- logical operations: `and`, `or`
- function calls: `name(argument, argument)`
- array indexing: `array[index]`
- map indexing: `map["key"]`
- module namespace member access: `module.member`

Operator precedence:

1. parentheses
2. unary minus and `not`
3. multiplication and division
4. addition and subtraction
5. comparisons
6. equality
7. `and`
8. `or`

Arithmetic and ordered comparisons are numeric-only. The `+` operator also joins two strings. Equality works across values and returns `false` when the two values have different types. Conditions for `if`, `while`, `and`, `or`, and `not` must be booleans.

## Strings

```zk
let name = "Vinod"
let age = 17
print("Hello ${name}, age ${age}")

let text = """
Hello
Zake
"""
print(text)
```

Strings support interpolation with `${expression}`. Variables, function calls, and basic expressions can be interpolated. Supported escape sequences are `\n`, `\t`, `\r`, `\"`, and `\\`.

Triple-quoted strings can span multiple lines. When the opening triple quote is followed by a newline, Zake skips that first newline. When the closing triple quote is on its own line, Zake trims the final newline before it.

## Control Flow

```zk
let score = 85

if score >= 75 {
    print("passed")
} else {
    print("failed")
}

let i = 0
while i < 3 {
    print(i)
    let i = i + 1
}
```

Blocks create scopes for new names. `let name = value` updates the nearest existing variable with that name when one exists; otherwise it defines a new variable in the current scope. This keeps loops usable before Zake adds a separate assignment operator.

## Functions

```zk
fn add(a, b) {
    return a + b
}

print(add(10, 20))

fn factorial(n) {
    if n <= 1 {
        return 1
    }

    return n * factorial(n - 1)
}

print(factorial(5))
```

Functions are declared with `fn`, receive positional arguments, and run with a fresh local parameter scope. Nested block scopes still work inside functions. Recursive calls are supported through normal variable lookup.

## Error Handling

Zake v0.9 supports `throw` and `try` / `catch`.

```zk
try {
    throw "Something went wrong"
} catch err {
    print(err)
}
```

`throw expression` stops the current execution path and sends the evaluated value to the nearest surrounding `catch`. The catch variable is bound only inside the catch block. Thrown strings, maps, arrays, and other values are catchable.

Structured error maps are the recommended pattern:

```zk
import std.error

try {
    throw error("File missing")
} catch err {
    print(err["type"])
    print(err["message"])
}
```

Many runtime errors are catchable too. When caught, runtime errors become maps with these keys:

- `type`: usually `RuntimeError`, `TypeError`, or `FileError`
- `message`: readable error text
- `line`: source line where the runtime error happened
- `column`: source column where the runtime error happened

Uncaught thrown values still become clean runtime errors at the command line. `return` inside a `try` or `catch` block still returns from the surrounding function.

## Arrays

```zk
let nums = [10, 20, 30]
print(nums[0])
push(nums, 40)
print(nums[3])

nums[1] = 25
print(nums[1])
```

Arrays can hold mixed values. Indexes are zero-based whole numbers. Indexing a non-array, using a non-number index, or reading outside the array length raises a runtime error.

## Maps

```zk
let user = {
    "name": "Vinod",
    "age": 17,
    "verified": true
}

print(user["name"])
user["country"] = "Sri Lanka"
print(user["country"])
```

Maps use string keys. Values can be numbers, strings, booleans, arrays, functions, or nested maps. Reading a missing key raises a runtime error. Assigning `map["key"] = value` updates an existing key or inserts a new key.

## Imports

```zk
import "greetings.zk"
import "math_utils.zk"

greet("Vinod")
print(add(10, 20))

import std.string
import std.math

print(upper("zake"))
print(square(5))

import std.string as string
import std.math as math

print(string.upper("zake"))
print(math.square(5))

from std.encoding import base64_encode, base64_decode

let encoded = base64_encode("hello")
print(base64_decode(encoded))

import "math_utils.zk" as utils

print(utils.add(10, 20))
```

Quoted imports resolve relative to the file doing the importing. Standard-library imports resolve from the project `stdlib/` directory. Plain imports keep the original behavior: exported functions and variables become available globally after import. Namespaced imports bind a module map to the alias, so members are accessed with `alias.member`. Selective imports bind only the named symbols globally. Zake skips duplicate module execution and reports circular imports.

Import errors are reported when a module path cannot be found, a selected symbol does not exist, an alias or imported symbol conflicts with an existing name, or member access references a missing member.

## Built-Ins

- `len(value)` returns the length of an array, map, or string.
- `push(array, value)` appends a value to an array and returns `nil`.
- `pop(array)` removes and returns the last array value.
- `keys(map)` returns an array of map keys in insertion order.
- `values(map)` returns an array of map values in insertion order.
- `has(map, key)` returns whether a string key exists in a map.
- `remove(map, key)` removes a string key from a map and returns `nil`.
- `type(value)` returns `nil`, `number`, `string`, `boolean`, `function`, `array`, or `map`.
- Native helpers beginning with `__` back the standard-library modules and are not intended as the primary user-facing API.

## Standard Library

Import standard-library modules with `import std.module`.

### std.string

- `upper(text)` returns uppercase text.
- `lower(text)` returns lowercase text.
- `contains(text, part)` returns whether `part` appears inside `text`.
- `starts_with(text, part)` returns whether `text` begins with `part`.
- `ends_with(text, part)` returns whether `text` ends with `part`.
- `trim(text)` removes leading and trailing whitespace.
- `length(text)` returns the string length.
- `split(text, sep)` returns an array of pieces split by `sep`.
- `replace(text, old, new)` returns text with all occurrences of `old` replaced by `new`.
- `substring(text, start, end)` returns the text from `start` up to but not including `end`.
- `char_at(text, index)` returns the one-character string at `index`.
- `index_of(text, part)` returns the first index of `part`, or `-1` when not found.

### std.math

- `square(x)` returns `x * x`.
- `cube(x)` returns `x * x * x`.
- `sqrt(x)` returns the square root of `x`.
- `abs(x)` returns the absolute value.
- `min(a, b)` returns the smaller number.
- `max(a, b)` returns the larger number.
- `round(x)` rounds to the nearest whole number.

### std.encoding

- `hex_encode(text)` encodes text as lowercase hex.
- `hex_decode(hex)` decodes hex text.
- `base64_encode(text)` encodes text as Base64.
- `base64_decode(text)` decodes Base64 text.

### std.time

- `now()` returns the current local time as text.
- `timestamp()` returns the current Unix timestamp in seconds.

### std.path

- `join(a, b)` joins two path segments.
- `basename(path)` returns the final path component.
- `dirname(path)` returns the parent path.
- `extension(path)` returns the file extension.

### std.file

`std.file` provides safe basic File I/O helpers. File paths are strings. Directory paths are rejected when a function expects a file.

- `read_file(path)` reads a text file and returns its contents. It errors if the file does not exist, is a directory, or cannot be read.
- `write_file(path, content)` writes string content to a file, replacing existing contents. It errors if the path is a directory or cannot be written.
- `append_file(path, content)` appends string content to a file. It errors if the path is a directory or cannot be written.
- `exists(path)` returns whether a path exists.
- `is_file(path)` returns whether a path exists and is a regular file.
- `is_dir(path)` returns whether a path exists and is a directory.
- `list_dir(path)` returns an array of file and folder names in a directory.
- `file_size(path)` returns the file size in bytes. It errors if the file does not exist or the path is a directory.
- `remove_file(path)` removes a file and returns whether removal happened. It errors if the file does not exist or the path is a directory.

### std.error

`std.error` provides helper functions that create structured error maps.

- `error(message)` returns `{"type": "Error", "message": message}`.
- `type_error(message)` returns `{"type": "TypeError", "message": message}`.
- `file_error(message)` returns `{"type": "FileError", "message": message}`.

## Comments

Use `//` for a line comment:

```zk
// This is ignored by the lexer
let x = 42
print(x)
```

## Errors

Zake reports readable errors with a file path, line, and column:

```text
examples\broken.zk:1:7: parser error: Expected ')' after expression.
```

## Current Limits

- no package manager
- no advanced file APIs such as streaming, binary buffers, permissions editing, or directory removal
- no networking
- no cybersecurity scanning modules
- no `finally`
- no typed catch filters
- no separate assignment operator
- no anonymous functions
- no closures as a documented feature
- no non-string map keys
- no `break` or `continue`
- no standard-library auto-loading
