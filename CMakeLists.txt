cmake_minimum_required(VERSION 3.20)

project(Zake VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(zake
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/ast.cpp
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
    endfunction()

    add_zake_test(example_hello "examples/hello.zk" ON "Hello Zake")
    add_zake_test(example_variables "examples/variables.zk" ON "Vinod__NL__17")
    add_zake_test(example_math "examples/math.zk" ON "50__NL__60")
    add_zake_test(error_unterminated_string "tests/unterminated_string.zk" OFF "Unterminated string literal")
    add_zake_test(error_missing_paren "tests/missing_paren.zk" OFF "Expected ')' after expression")
    add_zake_test(error_unknown_variable "tests/unknown_variable.zk" OFF "Unknown variable 'missing'")
    add_zake_test(error_invalid_arithmetic "tests/invalid_arithmetic.zk" OFF "Arithmetic operators require number operands")
    add_zake_test(error_divide_by_zero "tests/divide_by_zero.zk" OFF "Division by zero")
endif()
