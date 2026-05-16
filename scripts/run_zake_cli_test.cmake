if(NOT DEFINED ZAKE_EXECUTABLE)
    message(FATAL_ERROR "ZAKE_EXECUTABLE is required.")
endif()

if(NOT DEFINED EXPECT_SUCCESS)
    message(FATAL_ERROR "EXPECT_SUCCESS is required.")
endif()

if(NOT DEFINED CLI_ARGS)
    set(CLI_ARGS "")
endif()

execute_process(
    COMMAND "${ZAKE_EXECUTABLE}" ${CLI_ARGS}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

set(combined "${output}${error}")
string(REPLACE "\r\n" "\n" combined "${combined}")
string(REPLACE "\r" "\n" combined "${combined}")

set(expected_text "")
if(DEFINED EXPECTED_TEXT)
    set(expected_text "${EXPECTED_TEXT}")
    string(REPLACE "__NL__" "\n" expected_text "${expected_text}")
endif()

string(TOUPPER "${EXPECT_SUCCESS}" expect_success_upper)
set(expect_success FALSE)
if(expect_success_upper STREQUAL "ON" OR expect_success_upper STREQUAL "TRUE" OR expect_success_upper STREQUAL "1")
    set(expect_success TRUE)
endif()

if(expect_success)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Expected success but got exit code ${result}.\n${combined}")
    endif()
else()
    if(result EQUAL 0)
        message(FATAL_ERROR "Expected failure but command succeeded.\n${combined}")
    endif()
endif()

if(NOT expected_text STREQUAL "")
    string(FIND "${combined}" "${expected_text}" expected_index)
    if(expected_index EQUAL -1)
        message(FATAL_ERROR "Expected output to contain:\n${expected_text}\nActual output was:\n${combined}")
    endif()
endif()
