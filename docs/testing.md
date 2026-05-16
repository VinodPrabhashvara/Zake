# Zake Testing

Zake v0.11 includes a built-in test runner for `.zk` files.

## Run All Tests

```powershell
.\build\Release\zake.exe test
```

With no path, `zake test` discovers `.zk` files recursively inside `tests/`. Files under `tests/pass/` are expected to run successfully. Files under `tests/fail/` are expected to fail with a lexer, parser, or runtime error.

## Run One Folder

```powershell
.\build\Release\zake.exe test tests\pass
.\build\Release\zake.exe test tests\fail
```

When a folder is provided, Zake recursively discovers every `.zk` file inside that folder.

## Run One File

```powershell
.\build\Release\zake.exe test tests\pass\hello.zk
.\build\Release\zake.exe test tests\fail\divide_by_zero.zk
```

When a `.zk` file is provided, only that file is executed as a test.

## Verbose Output

```powershell
.\build\Release\zake.exe test tests\pass --verbose
```

Without `--verbose`, the runner prints one `PASS` or `FAIL` line per file. With `--verbose`, it also prints each passing test file's captured output and each expected-fail test file's captured error message.

## Pass And Fail Behavior

Tests are judged by their folder:

- `tests/pass/*.zk` passes only when the file runs successfully.
- `tests/pass/*.zk` fails when the file has an uncaught lexer, parser, or runtime error.
- `tests/fail/*.zk` passes only when the file fails with an uncaught lexer, parser, or runtime error.
- `tests/fail/*.zk` fails when the file unexpectedly runs successfully.

A test file cannot pass if:

- the file cannot be loaded
- its actual result does not match its expected result
- it is an expected-pass test with a lexer, parser, or runtime error
- it is an expected-fail test that unexpectedly succeeds

The command returns exit code `0` when every discovered test matches its expected result. It returns a non-zero exit code when any test does not match its expectation.

## Example Output

```text
Running Zake tests...

PASS tests/pass/functions.zk
PASS tests/pass/hello.zk
PASS tests/pass/math.zk
PASS tests/fail/divide_by_zero.zk expected failure

Result:
  Passed: 4
  Failed: 0
  Total: 4
```
