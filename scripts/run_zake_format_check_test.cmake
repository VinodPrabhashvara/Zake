if(NOT DEFINED ZAKE_EXECUTABLE)
    message(FATAL_ERROR "ZAKE_EXECUTABLE is required.")
endif()

if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE is required.")
endif()

if(NOT DEFINED TEMP_FILE)
    message(FATAL_ERROR "TEMP_FILE is required.")
endif()

if(NOT DEFINED EXPECT_SUCCESS)
    message(FATAL_ERROR "EXPECT_SUCCESS is required.")
endif()

configure_file("${INPUT_FILE}" "${TEMP_FILE}" COPYONLY)

execute_process(
    COMMAND "${ZAKE_EXECUTABLE}" fmt "${TEMP_FILE}" --check
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

set(combined "${output}${error}")
string(REPLACE "\r\n" "\n" combined "${combined}")
string(REPLACE "\r" "\n" combined "${combined}")

string(TOUPPER "${EXPECT_SUCCESS}" expect_success_upper)
set(expect_success FALSE)
if(expect_success_upper STREQUAL "ON" OR expect_success_upper STREQUAL "TRUE" OR expect_success_upper STREQUAL "1")
    set(expect_success TRUE)
endif()

if(expect_success)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Expected formatter check success but got exit code ${result}.\n${combined}")
    endif()
else()
    if(result EQUAL 0)
        message(FATAL_ERROR "Expected formatter check failure but command succeeded.\n${combined}")
    endif()
endif()

if(DEFINED EXPECTED_TEXT AND NOT EXPECTED_TEXT STREQUAL "")
    set(expected_text "${EXPECTED_TEXT}")
    string(REPLACE "__NL__" "\n" expected_text "${expected_text}")
    string(FIND "${combined}" "${expected_text}" expected_index)
    if(expected_index EQUAL -1)
        message(FATAL_ERROR "Expected output to contain:\n${expected_text}\nActual output was:\n${combined}")
    endif()
endif()
