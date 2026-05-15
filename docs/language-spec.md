# Zake Language Spec v0.3

Zake v0.3 is a small interpreter focused on expressions, control flow, and functions.

## File Extension

- Zake source files use the `.zk` extension.

## Statements

Zake v0.3 supports these statement forms:

```zk
print(expression)
let name = expression
fn name(parameter, parameter) {
    statement
}
return expression
if condition {
    statement
} else {
    statement
}
while condition {
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
- strings: `"hello"`
- booleans: `true`, `false`

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

- no modules
- no separate assignment operator
- no anonymous functions
- no closures as a documented feature
- no `break` or `continue`
- no standard-library auto-loading
