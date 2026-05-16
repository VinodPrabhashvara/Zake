# Zake REPL

Zake v0.14 includes a basic interactive REPL for quick experiments.

## Start

```powershell
.\build\Release\zake.exe repl
```

Help:

```powershell
.\build\Release\zake.exe repl --help
```

## Example Session

```text
Zake 0.14.0-alpha REPL
Type .help for commands, .exit to quit.

zake> let x = 2
zake> print(x)
2
zake> fn add(a, b) {
....>     return a + b
....> }
zake> print(add(x, 3))
5
zake> .vars
add = <fn add>
x = 2
zake> .exit
```

## Commands

- `.help` shows REPL commands.
- `.exit` exits the REPL.
- `.quit` exits the REPL.
- `.clear` clears the visible screen by printing blank lines.
- `.vars` shows global variables and functions.
- `.version` prints the current Zake version.

## Multi-Line Input

The REPL keeps asking for more lines while braces are not balanced. This supports `fn`, `if` / `else`, `while`, and `try` / `catch` blocks.

```text
zake> fn add(a, b) {
....>     return a + b
....> }
zake> print(add(2, 3))
5
```

## Errors

Lexer, parser, and runtime errors are printed cleanly and the REPL stays open for the next command.

## Limitations

The REPL does not auto-print bare expression results yet. Use `print(...)` when you want to display a value.
