# Zake Strings

Zake v0.17 upgrades strings with interpolation, common escape sequences, multiline strings, and a larger `std.string` module.

## Interpolation

Use `${...}` inside a string to insert an expression result.

```zk
let name = "Vinod"
let age = 17

print("Hello ${name}, age ${age}")
print("Math ${10 + 5}")
```

Interpolation supports variables, function calls, and basic expressions. Invalid interpolation syntax reports a clear runtime error.

## Escapes

Supported escapes:

- `\n` newline
- `\t` tab
- `\r` carriage return
- `\"` double quote
- `\\` backslash

## Multiline Strings

Triple-quoted strings can span multiple lines.

```zk
let text = """
Hello
Zake
"""

print(text)
```

When the opening triple quote is followed by a newline, Zake skips that first newline. When the closing triple quote is on its own line, Zake trims the final newline before it.

## std.string

Import with either style:

```zk
import std.string
import std.string as string
```

Functions:

- `upper(text)`
- `lower(text)`
- `contains(text, part)`
- `starts_with(text, part)`
- `ends_with(text, part)`
- `trim(text)`
- `length(text)`
- `split(text, sep)`
- `replace(text, old, new)`
- `substring(text, start, end)`
- `char_at(text, index)`
- `index_of(text, part)`

String indexes are zero-based byte offsets in v0.17. `substring(text, start, end)` uses an exclusive `end` index. `index_of` returns `-1` when the part is not found.
