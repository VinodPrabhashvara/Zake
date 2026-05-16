# Zake CLI

Zake v0.17 provides a small professional command-line interface around the existing interpreter, frontend diagnostics, built-in test runner, source formatter, static linter, interactive REPL, and basic project system.

## Usage

```text
zake <file.zk>
zake [options] <file.zk>
zake test [path] [--verbose]
zake fmt <path> [--check]
zake lint <path> [--strict]
zake repl
zake init [project-name]
zake run
zake check
```

## Commands

- `zake test` discovers and runs `.zk` tests in `tests/`.
- `zake test <folder>` recursively discovers `.zk` tests in a folder.
- `zake test <file.zk>` runs one test file.
- `zake test --verbose` prints normal test output and expected-fail error messages.
- `zake test --help` shows test runner help.
- `zake fmt <file.zk>` formats one Zake source file.
- `zake fmt <folder>` recursively formats `.zk` files in a folder.
- `zake fmt <path> --check` checks formatting without modifying files.
- `zake fmt --help` shows formatter help.
- `zake lint <file.zk>` lints one Zake source file.
- `zake lint <folder>` recursively lints `.zk` files in a folder.
- `zake lint <path> --strict` treats warnings as command failures.
- `zake lint --help` shows linter help.
- `zake repl` starts an interactive Zake session.
- `zake repl --help` shows REPL help.
- `zake init <project-name>` creates a new project folder.
- `zake init` creates project files in the current folder if safe.
- `zake run` runs the configured main file from `zake.toml`.
- `zake check` parses/checks the configured main file from `zake.toml`.
- `zake project --help` shows project command help.

## Options

- `-h`, `--help` shows help information.
- `-v`, `--version` prints the current Zake version.
- `--tokens file.zk` prints lexer tokens with type, lexeme, line, and column.
- `--ast file.zk` prints a readable parsed AST tree.
- `--check file.zk` lexes and parses a file without executing it.

## Examples

```powershell
.\build\Release\zake.exe examples\hello.zk
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
.\build\Release\zake.exe init sample-project
```

## Diagnostics

`--check` is useful before running a script because it verifies lexer and parser errors without executing imports, file I/O, print statements, or other runtime behavior.

`--tokens` is useful while working on the lexer. Each token prints as:

```text
line=1 column=1 type=print lexeme="print"
```

`--ast` is useful while working on the parser. It prints a simple tree such as:

```text
Program
  PrintStmt @ 1:1
    Literal(string, "Hello Zake") @ 1:7
```

Unknown options, missing file arguments, and missing files produce clean errors and suggest `zake --help`.

## Testing

The built-in test runner uses folder expectations:

- `tests/pass/` contains tests expected to run successfully.
- `tests/fail/` contains tests expected to fail with lexer, parser, or runtime errors.
- Expected-fail tests print as `PASS path expected failure` when they fail correctly.
- The command returns exit code `0` when all discovered tests match their expectation and non-zero when any test does not.

See [testing.md](testing.md) for the full test runner guide.

## Formatting

`zake fmt` formats `.zk` files with 4-space indentation, consistent operator spacing, comma spacing, and clearer block layout. Folder mode skips hidden folders and `build/`.

Use `--check` in scripts or CI when you want to verify formatting without changing files. See [formatter.md](formatter.md) for details.

## Linting

`zake lint` checks parsed Zake files for common static issues such as unused variables, duplicate declarations, unreachable code after `return` or `throw`, empty blocks, bad import paths, known function arity mismatches, variable shadowing, and constant division by zero.

Warnings do not fail the command by default. Errors fail the command. With `--strict`, warnings also produce a non-zero exit code.

Folder mode skips hidden folders and `build/`. See [linter.md](linter.md) for the full linter guide.

## REPL

`zake repl` starts an interactive prompt. Variables and functions stay available until the session exits.

REPL commands:

- `.help` shows REPL commands.
- `.exit` and `.quit` exit the REPL.
- `.clear` clears the visible screen by printing blank lines.
- `.vars` prints global variables and functions.
- `.version` prints the current Zake version.

Multi-line blocks continue with the `....>` prompt while braces are not balanced. See [repl.md](repl.md) for examples.

## Projects

`zake init` creates a small project with `zake.toml`, `src/main.zk`, `tests/hello_test.zk`, and `README.md`.

Inside a folder containing `zake.toml`, `zake run` executes the configured `main` file and `zake check` lexes/parses it without running.

See [project-system.md](project-system.md) for the full project guide.
