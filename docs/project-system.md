# Zake Project System

Zake v0.15 adds a basic project system powered by `zake.toml`.

## Create A Project

```powershell
.\build\Release\zake.exe init hello-zake
```

This creates:

```text
hello-zake/
  zake.toml
  src/
    main.zk
  tests/
    hello_test.zk
  README.md
```

You can also run `zake init` inside an empty or safe folder. Zake refuses to overwrite important files such as `zake.toml`, `src/main.zk`, `tests/hello_test.zk`, or `README.md`.

## Config Format

Only a minimal TOML subset is supported in v0.15:

```toml
name = "my-zake-project"
version = "0.1.0"
main = "src/main.zk"

[paths]
source = "src"
tests = "tests"
stdlib = "stdlib"
```

Supported syntax:

- `key = "value"`
- `[paths]`
- comments with `#`

## Run A Project

From inside a project folder:

```powershell
.\build\Release\zake.exe run
```

`zake run` finds `zake.toml`, reads the `main` path, and runs that file.

## Check A Project

```powershell
.\build\Release\zake.exe check
```

`zake check` finds `zake.toml`, reads the `main` path, and lexes/parses that file without executing it.

## Limitations

The v0.15 project system is intentionally small. It does not include a package manager, dependency resolution, full TOML support, or automatic test discovery from `[paths]` yet.
