# Zake Formatter

Zake v0.12 includes a basic formatter command for `.zk` files.

## Usage

```powershell
.\build\Release\zake.exe fmt examples\hello.zk
.\build\Release\zake.exe fmt examples
.\build\Release\zake.exe fmt examples\hello.zk --check
.\build\Release\zake.exe fmt examples --check
```

## Check Mode

`--check` does not modify files. It prints `Formatted: path` when a file is already formatted and `Needs formatting: path` when a file would change.

The command exits with code `0` when every checked file is formatted. It exits non-zero when any file needs formatting or cannot be processed.

## Folder Mode

When a folder is provided, Zake recursively discovers `.zk` files. Hidden folders and `build/` are skipped.

## Formatting Rules

- Indentation uses 4 spaces.
- Operators are spaced consistently.
- Commas are followed by one space.
- Blocks are indented clearly.
- `if` / `else`, `while`, `fn`, and `try` / `catch` blocks are formatted.
- Arrays and maps are spaced reasonably.
- Line comments are preserved when possible.
- One blank line is kept between top-level function declarations.

## Limitations

The v0.12 formatter is intentionally simple. It is a source formatter, not a full AST rewrite engine, so it focuses on common Zake code style and avoids changing language behavior.
