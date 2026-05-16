cmake_minimum_required(VERSION 3.20)

project(Zake VERSION 0.17.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(zake
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/ast.cpp
    src/debug_printer.cpp
    src/formatter.cpp
    src/linter.cpp
    src/project.cpp
    src/repl.cpp
    src/interpreter.cpp
    src/value.cpp
)

target_include_directories(zake PRIVATE include)

if(MSVC)
    target_compile_options(zake PRIVATE /W4)
else()
    target_compile_options(zake PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if(BUILD_TESTING)
    function(add_zake_test name input_file expect_success expected_text)
        add_test(NAME ${name}
            COMMAND ${CMAKE_COMMAND}
                -DZAKE_EXECUTABLE=$<TARGET_FILE:zake>
                -DINPUT_FILE=${CMAKE_SOURCE_DIR}/${input_file}
                -DEXPECT_SUCCESS=${expect_success}
                "-DEXPECTED_TEXT=${expected_text}"
                -P ${CMAKE_SOURCE_DIR}/scripts/run_zake_test.cmake
        )
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endfunction()

    function(add_zake_cli_test name expect_success expected_text)
        add_test(NAME ${name}
            COMMAND ${CMAKE_COMMAND}
                -DZAKE_EXECUTABLE=$<TARGET_FILE:zake>
                -DEXPECT_SUCCESS=${expect_success}
                "-DEXPECTED_TEXT=${expected_text}"
                "-DCLI_ARGS=${ARGN}"
                -P ${CMAKE_SOURCE_DIR}/scripts/run_zake_cli_test.cmake
        )
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endfunction()

    function(add_zake_format_test name input_file expected_file)
        add_test(NAME ${name}
            COMMAND ${CMAKE_COMMAND}
                -DZAKE_EXECUTABLE=$<TARGET_FILE:zake>
                -DINPUT_FILE=${CMAKE_SOURCE_DIR}/${input_file}
                -DEXPECTED_FILE=${CMAKE_SOURCE_DIR}/${expected_file}
                -DTEMP_FILE=${CMAKE_BINARY_DIR}/${name}.zk
                -P ${CMAKE_SOURCE_DIR}/scripts/run_zake_format_test.cmake
        )
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endfunction()

    function(add_zake_format_check_test name input_file expect_success expected_text)
        add_test(NAME ${name}
            COMMAND ${CMAKE_COMMAND}
                -DZAKE_EXECUTABLE=$<TARGET_FILE:zake>
                -DINPUT_FILE=${CMAKE_SOURCE_DIR}/${input_file}
                -DTEMP_FILE=${CMAKE_BINARY_DIR}/${name}.zk
                -DEXPECT_SUCCESS=${expect_success}
                "-DEXPECTED_TEXT=${expected_text}"
                -P ${CMAKE_SOURCE_DIR}/scripts/run_zake_format_check_test.cmake
        )
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endfunction()

    function(add_zake_repl_test name input_text expected_text)
        add_test(NAME ${name}
            COMMAND ${CMAKE_COMMAND}
                -DZAKE_EXECUTABLE=$<TARGET_FILE:zake>
                "-DINPUT_TEXT=${input_text}"
                "-DEXPECTED_TEXT=${expected_text}"
                -DINPUT_FILE=${CMAKE_BINARY_DIR}/${name}.input
                -P ${CMAKE_SOURCE_DIR}/scripts/run_zake_repl_test.cmake
        )
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endfunction()

    function(add_zake_project_test name)
        add_test(NAME ${name}
            COMMAND ${CMAKE_COMMAND}
                -DZAKE_EXECUTABLE=$<TARGET_FILE:zake>
                -DPROJECT_TEST_ROOT=${CMAKE_BINARY_DIR}/${name}
                -P ${CMAKE_SOURCE_DIR}/scripts/run_zake_project_test.cmake
        )
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endfunction()

    add_zake_cli_test(cli_help ON "Usage:__NL__  zake <file.zk>" "--help")
    add_zake_cli_test(cli_short_help ON "Options:" "-h")
    add_zake_cli_test(cli_version ON "Zake 0.17.0-alpha" "--version")
    add_zake_cli_test(cli_short_version ON "Zake 0.17.0-alpha" "-v")
    add_zake_cli_test(cli_check ON "OK: examples/hello.zk" "--check" "examples/hello.zk")
    add_zake_cli_test(cli_tokens ON "type=print lexeme=\"print\"" "--tokens" "examples/hello.zk")
    add_zake_cli_test(cli_ast ON "Program__NL__  PrintStmt" "--ast" "examples/hello.zk")
    add_zake_cli_test(cli_test_help ON "Zake test runner" "test" "--help")
    add_zake_cli_test(cli_test_pass_folder ON "Passed: 3__NL__  Failed: 0__NL__  Total: 3" "test" "tests/pass")
    add_zake_cli_test(cli_test_pass_file ON "PASS tests/pass/hello.zk" "test" "tests/pass/hello.zk")
    add_zake_cli_test(cli_test_verbose ON "Output:__NL__Hello from Zake test" "test" "tests/pass/hello.zk" "--verbose")
    add_zake_cli_test(cli_test_fail_file ON "PASS tests/fail/divide_by_zero.zk expected failure" "test" "tests/fail/divide_by_zero.zk")
    add_zake_cli_test(cli_test_mixed_suite ON "Failed: 0" "test" "tests")
    add_zake_cli_test(cli_fmt_help ON "Zake formatter" "fmt" "--help")
    add_zake_cli_test(cli_fmt_check_formatted ON "Formatted: tests/formatter/expected_formatted.zk" "fmt" "tests/formatter/expected_formatted.zk" "--check")
    add_zake_format_check_test(cli_fmt_check_unformatted "tests/formatter/input_unformatted.raw" OFF "Needs formatting:")
    add_zake_cli_test(cli_lint_help ON "Zake linter" "lint" "--help")
    add_zake_cli_test(cli_lint_clean ON "Warnings: 0__NL__  Errors: 0" "lint" "tests/linter/clean.zk")
    add_zake_cli_test(cli_lint_unused ON "unused variable 'unused'" "lint" "tests/linter/unused_variable.zk")
    add_zake_cli_test(cli_lint_unused_strict OFF "unused variable 'unused'" "lint" "tests/linter/unused_variable.zk" "--strict")
    add_zake_cli_test(cli_lint_divide_by_zero OFF "division by zero" "lint" "tests/linter/divide_by_zero.zk")
    add_zake_cli_test(cli_lint_folder OFF "Files: 5" "lint" "tests/linter")
    add_zake_cli_test(cli_lint_folder_strict OFF "Warnings:" "lint" "tests/linter" "--strict")
    add_zake_cli_test(cli_repl_help ON "Zake REPL" "repl" "--help")
    add_zake_repl_test(cli_repl_scripted "let x = 2__NL__print(x)__NL__fn add(a, b) {__NL__    return a + b__NL__}__NL__print(add(x, 3))__NL__.version__NL__.vars__NL__.exit__NL__" "2__NL__zake> ....> ....> zake> 5__NL__zake> Zake 0.17.0-alpha__NL__zake> add = <fn add>__NL__x = 2")
    add_zake_cli_test(cli_project_help ON "Zake project commands" "project" "--help")
    add_zake_project_test(cli_project_flow)
    add_zake_cli_test(cli_missing_file OFF "file does not exist" "missing_file.zk")
    add_zake_cli_test(cli_unknown_option OFF "unknown option: --unknown" "--unknown")
    add_zake_cli_test(cli_check_error OFF "parser error" "--check" "examples/diagnostics_error.zk")
    add_zake_format_test(format_unformatted "tests/formatter/input_unformatted.raw" "tests/formatter/expected_formatted.zk")

    add_zake_test(example_hello "examples/hello.zk" ON "Hello Zake")
    add_zake_test(example_variables "examples/variables.zk" ON "Vinod__NL__17")
    add_zake_test(example_math "examples/math.zk" ON "50__NL__60")
    add_zake_test(example_control_flow "examples/control_flow.zk" ON "passed__NL__0__NL__1__NL__2")
    add_zake_test(example_functions "examples/functions.zk" ON "Hello Prabhashvara__NL__30__NL__120")
    add_zake_test(example_arrays "examples/arrays.zk" ON "10__NL__20__NL__30__NL__3__NL__4__NL__40__NL__40__NL__3__NL__array__NL__string__NL__number__NL__boolean__NL__Zake__NL__1__NL__true__NL__10")
    add_zake_test(example_maps "examples/maps.zk" ON "Vinod__NL__17__NL__true__NL__Sri Lanka__NL__4__NL__map__NL__true__NL__false__NL__array__NL__4__NL__array__NL__4__NL__false__NL__vinod__NL__cpp__NL__Zake__NL__Vinod")
    add_zake_test(example_modules "examples/modules.zk" ON "Hello Vinod__NL__30__NL__ZAKE__NL__zake__NL__25")
    add_zake_test(example_import_namespaces "examples/import_namespaces.zk" ON "ZAKE__NL__zake__NL__25__NL__27__NL__aGVsbG8=__NL__hello__NL__30")
    add_zake_test(example_strings_advanced "examples/strings_advanced.zk" ON "Hello Vinod, age 17__NL__Math 15, call 8")
    add_zake_test(example_stdlib_basic "examples/stdlib_basic.zk" ON "ZAKE__NL__zake__NL__true__NL__true__NL__true__NL__hello__NL__5__NL__25__NL__27__NL__9__NL__10__NL__10__NL__20__NL__11__NL__aGVsbG8=__NL__hello__NL__7a616b65__NL__zake")
    add_zake_test(example_file_io "examples/file_io.zk" ON "true__NL__true__NL__false__NL__Hello Zake File I/O__NL__Hello Zake File I/O__NL__Second line__NL__31__NL__array")
    add_zake_test(example_error_handling "examples/error_handling.zk" ON "Error__NL__Manual error test__NL__TypeError__NL__Expected number__NL__Caught file error__NL__File does not exist: 'missing_file.txt'.__NL__10__NL__Caught function error__NL__Value cannot be zero")
    add_zake_test(control_if_else "tests/if_else.zk" ON "large__NL__logic-ok")
    add_zake_test(control_while "tests/while.zk" ON "0__NL__1__NL__2__NL__done")
    add_zake_test(functions "tests/functions.zk" ON "Hello Tester__NL__12__NL__24")
    add_zake_test(arrays "tests/arrays.zk" ON "1__NL__9__NL__3__NL__array__NL__string__NL__number__NL__boolean__NL__left")
    add_zake_test(maps "tests/maps.zk" ON "Zake__NL__0.5__NL__true__NL__ZakeLang__NL__123__NL__5__NL__name__NL__extra__NL__ZakeLang__NL__false__NL__4")
    add_zake_test(imports "tests/imports.zk" ON "loaded-once__NL__Hello Tester__NL__5__NL__ZAKE__NL__zake__NL__36__NL__27__NL__true__NL__7")
    add_zake_test(import_namespaces "tests/import_namespaces.zk" ON "ZAKE__NL__zake__NL__16__NL__8__NL__dGVzdA==__NL__test__NL__15")
    add_zake_test(strings_advanced "tests/strings_advanced.zk" ON "Hello Vinod, age 17__NL__Sum 15 call 8__NL__Line A__NL__Line B__NL__Hello__NL__Zake__NL__red__NL__3__NL__hello Vinod__NL__akeL__NL__V__NL__6__NL__5")
    add_zake_test(stdlib_basic "tests/stdlib_basic.zk" ON "true__NL__false__NL__clean__NL__4__NL__16__NL__8__NL__5__NL__text__NL__5__NL__6869__NL__hi__NL__Y29kZQ==__NL__code__NL__string__NL__number__NL__a/b__NL__b.txt__NL__a__NL__.txt")
    add_zake_test(file_io "tests/file_io.zk" ON "true__NL__true__NL__false__NL__abc123__NL__6__NL__array__NL__false")
    add_zake_test(error_handling "tests/error_handling.zk" ON "Something went wrong__NL__Error__NL__Structured__NL__FileError__NL__true__NL__true__NL__returned")
    add_zake_test(error_unterminated_string "tests/fail/unterminated_string.zk" OFF "Unterminated string literal")
    add_zake_test(error_missing_paren "tests/fail/missing_paren.zk" OFF "Expected ')' after expression")
    add_zake_test(error_unknown_variable "tests/fail/unknown_variable.zk" OFF "Unknown variable 'missing'")
    add_zake_test(error_invalid_arithmetic "tests/fail/invalid_arithmetic.zk" OFF "Arithmetic operators require number operands")
    add_zake_test(error_divide_by_zero "tests/fail/divide_by_zero.zk" OFF "Division by zero")
    add_zake_test(error_function_arity "tests/fail/function_arity.zk" OFF "expected 2 argument")
    add_zake_test(error_call_non_function "tests/fail/call_non_function.zk" OFF "Only functions can be called")
    add_zake_test(error_array_index_type "tests/fail/array_index_type.zk" OFF "Array index must be a number")
    add_zake_test(error_array_index_range "tests/fail/array_index_range.zk" OFF "out of range")
    add_zake_test(error_builtin_args "tests/fail/builtin_args.zk" OFF "Built-in push")
    add_zake_test(error_map_key_type "tests/fail/map_key_type.zk" OFF "Map key must be a string")
    add_zake_test(error_map_missing_key "tests/fail/map_missing_key.zk" OFF "Map key 'missing' was not found")
    add_zake_test(error_map_builtin_args "tests/fail/map_builtin_args.zk" OFF "Built-in keys")
    add_zake_test(error_circular_import "tests/fail/circular_a.zk" OFF "Circular import detected")
    add_zake_test(error_import_missing_symbol "tests/fail/import_missing_symbol.zk" OFF "Imported symbol 'missing_symbol' does not exist")
    add_zake_test(error_import_missing_member "tests/fail/import_missing_member.zk" OFF "Member 'missing_member' does not exist")
    add_zake_test(error_string_bad_index "tests/fail/string_bad_index.zk" OFF "string index 4 is out of range")
    add_zake_test(error_string_unterminated_multiline "tests/fail/string_unterminated_multiline.zk" OFF "Unterminated multiline string literal")
    add_zake_test(error_file_missing "tests/fail/file_missing.zk" OFF "File does not exist")
    add_zake_test(error_file_wrong_args "tests/fail/file_wrong_args.zk" OFF "expects argument 1 to be string")
    add_zake_test(error_uncaught_throw "tests/fail/uncaught_throw.zk" OFF "Uncaught thrown value: Error: Uncaught boom")
endif()
