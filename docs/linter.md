# Zake Linter

Zake v0.13 includes a basic static linter for catching common issues before running a script.

## Commands

```powershell
.\build\Release\zake.exe lint examples\hello.zk
.\build\Release\zake.exe lint tests\linter
.\build\Release\zake.exe lint tests\linter --strict
.\build\Release\zake.exe lint --help
```

## Checks

- unused variables
- duplicate variable declarations in the same scope
- unreachable code after `return`
- unreachable code after `throw`
- empty blocks
- bad import paths when the target can be checked
- calls with the wrong argument count when the function declaration is known
- variable shadowing
- constant division by zero

## Output

```text
Linting Zake files...

WARN examples/test.zk:3:5: unused variable 'x'
ERROR examples/test.zk:12:10: division by zero

Result:
  Warnings: 1
  Errors: 1
  Files: 1
```

## Exit Codes

- Exit code `0` means no lint errors were found.
- Exit code non-zero means at least one lint error was found.
- Warnings alone do not fail by default.
- With `--strict`, warnings also cause a non-zero exit code.

## Folder Mode

Folder mode recursively lints `.zk` files. Hidden folders and `build/` are skipped.

## Limitations

The v0.13 linter is intentionally simple. It does not perform full type inference, runtime path evaluation, or deep data-flow analysis. It focuses on useful static checks that can be detected from the current AST.
