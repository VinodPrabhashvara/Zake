# Zake Roadmap

## v0.1

- working interpreter in C++17
- `print(...)`
- `let` variables
- numbers, strings, booleans
- arithmetic and parentheses
- line comments with `//`
- readable lexer, parser, and runtime errors

## v0.2

- comparison operators: `==`, `!=`, `>`, `>=`, `<`, `<=`
- logical operations: `not`, `and`, `or`
- `if` and `else` statements
- `while` loops
- block scopes with `{ }`
- focused tests and examples for control flow

## v0.3

- function declarations with `fn`
- function calls with parameters
- `return` statements
- local function scope
- recursion support

## v0.4

- array literals with `[ ]`
- array indexing with `array[index]`
- array index assignment
- simple built-ins: `len`, `push`, `pop`, `type`

## v0.5

- map literals with `{ }`
- string-keyed map indexing with `map["key"]`
- map index assignment
- map built-ins: `keys`, `values`, `has`, `remove`

## v0.6

- local imports with `import "file.zk"`
- standard-library imports with `import std.module`
- shared global environment for imported functions and variables
- duplicate import protection
- circular import detection
- starter stdlib modules for strings and math

## v0.7

- `std.string`
- `std.math`
- `std.encoding`
- `std.time`
- `std.path`
- native helpers for basic stdlib functionality

## v0.8

- `std.file`
- safe basic File I/O helpers
- read, write, append, existence checks, directory listing, file size, and file removal
- clear runtime errors for missing files, directories used as files, permission failures, and wrong argument types

## v0.9

- `throw` statements
- `try` and `catch` statements
- catch variable binding
- structured error maps through `std.error`
- catchable thrown strings, thrown maps, and many runtime errors

## v0.10

- professional command-line help and version output
- `--tokens` lexer diagnostics
- `--ast` parser diagnostics
- `--check` parse-only validation
- cleaner missing-file, missing-argument, and unknown-option errors

## v0.11

- built-in `zake test` command
- default test discovery in `tests/`
- recursive folder discovery
- single-file test execution
- verbose test output mode
- pass/fail summaries and useful exit codes

## v0.12

- `zake fmt` formatter command
- formatter check mode for CI-style validation
- recursive folder formatting
- 4-space indentation and basic whitespace normalization
- formatter documentation and fixtures

## v0.13

- `zake lint` linter command
- strict mode for warnings-as-failures
- recursive folder linting
- checks for unused variables, duplicate declarations, unreachable code, empty blocks, bad import paths, function arity, shadowing, and constant division by zero
- linter documentation and fixtures

## v0.14

- `zake repl` interactive command
- persistent variables and functions inside a REPL session
- multiline input for brace-delimited blocks
- REPL dot commands for help, exit, clear, variables, and version
- clean lexer, parser, and runtime errors without closing the REPL

## v0.15

- basic `zake.toml` project config
- `zake init` project scaffolding
- `zake run` command for configured project main files
- `zake check` command for configured project main files
- project system documentation and CTest coverage

## v0.16

- namespaced standard-library imports such as `import std.string as string`
- namespaced local imports such as `import "utils.zk" as utils`
- selective imports such as `from std.math import square, cube`
- module namespace member access with `namespace.member`
- clear import errors for missing symbols, missing members, and alias conflicts

## v0.17

- string interpolation with `${expression}`
- common escape sequences for strings
- triple-quoted multiline strings
- expanded `std.string` helpers: `split`, `replace`, `substring`, `char_at`, and `index_of`
- string-focused runtime and lexer errors

## v0.18

- loop controls such as `break` and `continue`
- separate assignment syntax

## Long-Term Direction

Future versions of Zake are intended to grow security-focused standard libraries for ethical hacking, CTF tooling, file analysis, hashing, encoding, reporting, and defensive security. The goal is to keep the language approachable while making it genuinely useful for learning and automation.
