# Zake

**A small, Python-like programming language for learning, building, and growing into security-focused tooling.**

Zake is a new programming language project built from scratch in C++17. Zake v0.3 focuses on a small but useful interpreter with variables, expressions, control flow, functions, clear code, and friendly errors. Future versions are planned to grow toward cybersecurity-focused standard libraries for ethical hacking, CTF workflows, file analysis, hashing, encoding, reporting, and defensive security.

## Features

- C++17 interpreter built with CMake
- `.zk` source files
- `print(...)` statements
- `let` variables
- number, string, and boolean literals
- arithmetic with `+`, `-`, `*`, `/`
- comparison operators: `==`, `!=`, `>`, `>=`, `<`, `<=`
- logical operations: `not`, `and`, `or`
- `if` / `else` statements
- `while` loops
- block scopes with `{ }`
- function declarations with `fn`
- function calls with parameters
- `return` statements
- recursion support
- parentheses for grouping
- line comments with `//`
- lexer, parser, and runtime errors with line and column information
- simple command-line execution: `zake <file.zk>`

## Example Zake Code

```zk
print("Hello Zake")

let name = "Vinod"
let age = 17
print(name)
print(age)

let x = 10 + 20 * 2
print(x)

if age >= 18 {
    print("adult")
} else {
    print("student")
}

fn add(a, b) {
    return a + b
}

print(add(10, 20))
```

## Build Instructions

Windows is documented primarily for Visual Studio 2022 so the Release binary lands at `build\Release\zake.exe`.

```powershell
cmake -S . -B build
cmake --build build --config Release
```

If your machine has multiple CMake generators installed, you can be explicit:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

## Run Instructions

```powershell
.\build\Release\zake.exe examples\hello.zk
.\build\Release\zake.exe examples\variables.zk
.\build\Release\zake.exe examples\math.zk
.\build\Release\zake.exe examples\control_flow.zk
.\build\Release\zake.exe examples\functions.zk
```

## Project Structure

```text
zake/
├─ assets/
│  └─ branding/
├─ docs/
├─ examples/
├─ include/
│  └─ zake/
├─ scripts/
├─ src/
├─ stdlib/
├─ tests/
├─ CMakeLists.txt
├─ LICENSE
└─ README.md
```

## Roadmap

- v0.1: core interpreter with variables, literals, math, comments, and friendly errors
- v0.2: comparisons, logical operations, block scopes, `if` / `else`, and `while`
- v0.3: function declarations, calls, parameters, returns, local function scope, and recursion
- v0.4: early standard library helpers
- future: cybersecurity-focused standard libraries for ethical hacking and defensive tooling

See [docs/roadmap.md](docs/roadmap.md) for the expanded roadmap and [docs/language-spec.md](docs/language-spec.md) for the current language rules.

## Author

Vinod Prabhashvara
