# Zake Language Spec v0.1

Zake v0.1 is a small, expression-based interpreter focused on a clean first release.

## File Extension

- Zake source files use the `.zk` extension.

## Statements

Zake v0.1 supports two statement forms:

```zk
print(expression)
let name = expression
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
- arithmetic: `+`, `-`, `*`, `/`

Operator precedence:

1. parentheses
2. unary minus
3. multiplication and division
4. addition and subtraction

Arithmetic is numeric-only in v0.1. Trying to use strings or booleans with arithmetic operators raises a runtime error.

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
- no blocks
- no conditionals
- no loops
- no string concatenation
- no module loading
- `stdlib/prelude.zk` is present in the repository but not auto-loaded yet
