# Zake Language Spec v0.2

Zake v0.2 is a small interpreter focused on expressions and control flow.

## File Extension

- Zake source files use the `.zk` extension.

## Statements

Zake v0.2 supports these statement forms:

```zk
print(expression)
let name = expression
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
- comparison: `==`, `!=`, `>`, `>=`, `<`, `<=`
- logical operations: `and`, `or`

Operator precedence:

1. parentheses
2. unary minus and `not`
3. multiplication and division
4. addition and subtraction
5. comparisons
6. equality
7. `and`
8. `or`

Arithmetic and ordered comparisons are numeric-only. Equality works across values and returns `false` when the two values have different types. Conditions for `if`, `while`, `and`, `or`, and `not` must be booleans.

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

Blocks create scopes for new names. In v0.2, `let name = value` updates the nearest existing variable with that name when one exists; otherwise it defines a new variable in the current scope. This keeps loops usable before Zake adds a separate assignment operator.

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

- no functions
- no string concatenation
- no modules
- no separate assignment operator
- no `break` or `continue`
- no standard-library auto-loading
