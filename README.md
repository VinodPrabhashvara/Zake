# Zake

**A small, Python-like programming language for learning, building, and growing into security-focused tooling.**

Zake is a new programming language project built from scratch in C++17. Zake v0.17 focuses on a small but useful interpreter with variables, expressions, control flow, functions, arrays, maps, upgraded strings, imports with namespaces and selective imports, basic standard-library modules, safe basic file I/O, real error handling, a professional command-line interface, a built-in test runner, a basic source formatter, a basic static linter, an interactive REPL, a basic `zake.toml` project system, clear code, and friendly errors. Future versions are planned to grow toward cybersecurity-focused standard libraries for ethical hacking, CTF workflows, file analysis, hashing, encoding, reporting, and defensive security.

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
- string interpolation with `${expression}`
- escape sequences: `\n`, `\t`, `\r`, `\"`, and `\\`
- triple-quoted multiline strings
- array literals with `[ ]`
- array indexing with `array[index]`
- array index assignment with `array[index] = value`
- map literals with string keys
- map indexing and assignment with `map["key"]`
- built-ins: `len(value)`, `push(array, value)`, `pop(array)`, `type(value)`, `keys(map)`, `values(map)`, `has(map, key)`, `remove(map, key)`
- local imports with `import "file.zk"`
- standard library imports with `import std.module`
- namespaced imports with `import std.module as name` and `import "file.zk" as name`
- selective imports with `from std.module import name, other_name`
- member access for module namespaces with `namespace.member`
- import duplicate and circular import protection
- `throw` statements
- `try` / `catch` statements with catch variable binding
- catchable thrown strings, thrown maps, and many runtime errors
- basic stdlib modules: `std.string`, `std.math`, `std.encoding`, `std.time`, `std.path`, `std.file`, `std.error`
- advanced `std.string` helpers such as `split`, `replace`, `substring`, `char_at`, and `index_of`
- safe basic File I/O through `std.file`
- native helpers backing stdlib modules, including string case, encoding, time, path, math, and file helpers
- parentheses for grouping
- line comments with `//`
- lexer, parser, and runtime errors with line and column information
- simple command-line execution: `zake <file.zk>`
- CLI help and version flags: `--help`, `-h`, `--version`, `-v`
- developer diagnostics: `--tokens`, `--ast`, and `--check`
- built-in test runner: `zake test`
- basic source formatter: `zake fmt`
- basic static linter: `zake lint`
- interactive REPL: `zake repl`
- project basics with `zake.toml`: `zake init`, `zake run`, `zake check`

## Example Zake Code

```zk
print("Hello Zake")

let name = "Vinod"
let age = 17
print(name)
print(age)
print("Hello ${name}, age ${age}")

let text = """
Hello
Zake
"""

print(text)

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

let nums = [10, 20, 30]
push(nums, 40)
print(nums[3])
print(type(nums))

let user = {
    "name": "Vinod",
    "verified": true
}

print(user["name"])
print(has(user, "verified"))

import std.string
import std.math
import std.path
import std.file
import std.error
import std.string as string
from std.encoding import base64_encode

print(upper("zake"))
print(string.lower("ZAKE"))
print(square(5))
print(base64_encode("hello"))
print(basename("examples/hello.zk"))

let path = "examples/tmp_zake_test.txt"
write_file(path, "Hello Zake")
print(read_file(path))
remove_file(path)

try {
    throw error("Something went wrong")
} catch err {
    print(err["type"])
    print(err["message"])
}
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
.\build\Release\zake.exe examples\arrays.zk
.\build\Release\zake.exe examples\maps.zk
.\build\Release\zake.exe examples\modules.zk
.\build\Release\zake.exe examples\import_namespaces.zk
.\build\Release\zake.exe examples\strings_advanced.zk
.\build\Release\zake.exe examples\stdlib_basic.zk
.\build\Release\zake.exe examples\file_io.zk
.\build\Release\zake.exe examples\error_handling.zk
```

## CLI Instructions

```powershell
.\build\Release\zake.exe --help
.\build\Release\zake.exe --version
.\build\Release\zake.exe --tokens examples\hello.zk
.\build\Release\zake.exe --ast examples\hello.zk
.\build\Release\zake.exe --check examples\hello.zk
.\build\Release\zake.exe test
.\build\Release\zake.exe test tests\pass --verbose
.\build\Release\zake.exe fmt examples\hello.zk --check
.\build\Release\zake.exe lint examples\hello.zk
.\build\Release\zake.exe lint tests\linter --strict
.\build\Release\zake.exe repl
.\build\Release\zake.exe project --help
```

`--check` lexes and parses a file without running it. `--tokens` and `--ast` are intended for debugging the language frontend. `zake test` discovers and runs `.zk` files as pass/fail tests. `zake fmt` formats source files or checks formatting without editing. `zake lint` reports common static issues before runtime. `zake repl` starts an interactive session. `zake init`, `zake run`, and `zake check` provide basic project workflows through `zake.toml`.

## Project Instructions

```powershell
.\build\Release\zake.exe init sample-project
Set-Location sample-project
..\build\Release\zake.exe run
..\build\Release\zake.exe check
```

Generated `zake.toml`:

```toml
name = "sample-project"
version = "0.1.0"
main = "src/main.zk"

[paths]
source = "src"
tests = "tests"
stdlib = "stdlib"
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
- v0.4: arrays, indexing, array mutation helpers, and simple built-ins
- v0.5: maps with string keys, map indexing, map assignment, and map built-ins
- v0.6: local imports, standard-library imports, duplicate import protection, and circular import errors
- v0.7: basic standard-library modules for strings, math, encoding, time, and paths
- v0.8: safe basic File I/O with `std.file`
- v0.9: `throw`, `try` / `catch`, catch variable binding, and `std.error`
- v0.10: professional CLI help, version output, frontend diagnostics, and parse-only checks
- v0.11: built-in test runner with discovery, verbose mode, and pass/fail summaries
- v0.12: basic source formatter with check mode and folder support
- v0.13: basic static linter with strict mode and folder support
- v0.14: interactive REPL with persistent variables, functions, multiline blocks, and dot commands
- v0.15: basic `zake.toml` project system with init, run, and check commands
- v0.16: namespaced imports, selective imports, local module aliases, and namespace member access
- v0.17: string interpolation, escape sequences, multiline strings, and advanced `std.string`
- future: cybersecurity-focused standard libraries for ethical hacking and defensive tooling

See [docs/cli.md](docs/cli.md) for CLI usage, [docs/testing.md](docs/testing.md) for the built-in test runner, [docs/formatter.md](docs/formatter.md) for formatter usage, [docs/linter.md](docs/linter.md) for linter usage, [docs/repl.md](docs/repl.md) for REPL usage, [docs/project-system.md](docs/project-system.md) for project usage, [docs/imports.md](docs/imports.md) for import usage, [docs/strings.md](docs/strings.md) for string usage, [docs/roadmap.md](docs/roadmap.md) for the expanded roadmap, and [docs/language-spec.md](docs/language-spec.md) for the current language rules.

## Author

Vinod Prabhashvara
