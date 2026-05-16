if(NOT DEFINED ZAKE_EXECUTABLE)
    message(FATAL_ERROR "ZAKE_EXECUTABLE is required.")
endif()

if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE is required.")
endif()

if(NOT DEFINED EXPECTED_FILE)
    message(FATAL_ERROR "EXPECTED_FILE is required.")
endif()

if(NOT DEFINED TEMP_FILE)
    message(FATAL_ERROR "TEMP_FILE is required.")
endif()

configure_file("${INPUT_FILE}" "${TEMP_FILE}" COPYONLY)

execute_process(
    COMMAND "${ZAKE_EXECUTABLE}" fmt "${TEMP_FILE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Formatter command failed with exit code ${result}.\n${output}${error}")
endif()

file(READ "${TEMP_FILE}" actual)
file(READ "${EXPECTED_FILE}" expected)

string(REPLACE "\r\n" "\n" actual "${actual}")
string(REPLACE "\r" "\n" actual "${actual}")
string(REPLACE "\r\n" "\n" expected "${expected}")
string(REPLACE "\r" "\n" expected "${expected}")

if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "Formatted output did not match expected file.\nFormatter output:\n${output}${error}")
endif()
