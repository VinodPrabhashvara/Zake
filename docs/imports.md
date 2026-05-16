# Zake Imports

Zake v0.16 supports the original global import style plus namespaced and selective imports.

## Global Imports

```zk
import std.string
import "math_utils.zk"

print(upper("zake"))
print(add(10, 20))
```

Plain imports keep backward compatibility. Functions and variables from the imported module become available in the global environment. Duplicate imports do not execute the same module twice.

## Namespaced Imports

```zk
import std.string as string
import std.math as math
import "math_utils.zk" as utils

print(string.upper("zake"))
print(math.square(5))
print(utils.add(10, 20))
```

The alias is a module namespace value. Access exported members with `namespace.member`.

## Selective Imports

```zk
from std.math import square, cube
from std.string import upper, lower

print(square(5))
print(cube(3))
print(upper("zake"))
```

Selective imports bind only the named symbols. Zake reports a clear runtime error if a requested symbol does not exist.

## Resolution Rules

- `import "file.zk"` resolves relative to the file doing the importing.
- `import "file.zk" as name` uses the same local resolution and binds the module to `name`.
- `import std.module` and `import std.module as name` resolve from the project `stdlib/` folder.
- `from std.module import name` resolves from the project `stdlib/` folder.

## Errors

Zake reports import errors for missing files, circular imports, missing selective symbols, missing namespace members, and alias/name conflicts.

## Limits

Namespaces are represented with the existing map value system in v0.16. Dot access is intended for module namespaces and also works for maps with simple string keys, but `map["key"]` remains the recommended map access form.
