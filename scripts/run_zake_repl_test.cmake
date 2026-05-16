if(NOT DEFINED ZAKE_EXECUTABLE)
    message(FATAL_ERROR "ZAKE_EXECUTABLE is required.")
endif()

if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE is required.")
endif()

if(NOT DEFINED INPUT_TEXT)
    message(FATAL_ERROR "INPUT_TEXT is required.")
endif()

set(input_text "${INPUT_TEXT}")
string(REPLACE "__NL__" "\n" input_text "${input_text}")
file(WRITE "${INPUT_FILE}" "${input_text}")

execute_process(
    COMMAND "${ZAKE_EXECUTABLE}" repl
    INPUT_FILE "${INPUT_FILE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

set(combined "${output}${error}")
string(REPLACE "\r\n" "\n" combined "${combined}")
string(REPLACE "\r" "\n" combined "${combined}")

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Expected REPL success but got exit code ${result}.\n${combined}")
endif()

if(DEFINED EXPECTED_TEXT)
    set(expected_text "${EXPECTED_TEXT}")
    string(REPLACE "__NL__" "\n" expected_text "${expected_text}")
    string(FIND "${combined}" "${expected_text}" expected_index)
    if(expected_index EQUAL -1)
        message(FATAL_ERROR "Expected REPL output to contain:\n${expected_text}\nActual output was:\n${combined}")
    endif()
endif()
