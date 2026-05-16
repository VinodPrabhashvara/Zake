if(NOT DEFINED ZAKE_EXECUTABLE)
    message(FATAL_ERROR "ZAKE_EXECUTABLE is required.")
endif()

if(NOT DEFINED PROJECT_TEST_ROOT)
    message(FATAL_ERROR "PROJECT_TEST_ROOT is required.")
endif()

file(REMOVE_RECURSE "${PROJECT_TEST_ROOT}")
file(MAKE_DIRECTORY "${PROJECT_TEST_ROOT}")

execute_process(
    COMMAND "${ZAKE_EXECUTABLE}" init temp_project
    WORKING_DIRECTORY "${PROJECT_TEST_ROOT}"
    RESULT_VARIABLE init_result
    OUTPUT_VARIABLE init_output
    ERROR_VARIABLE init_error
)

set(init_combined "${init_output}${init_error}")
if(NOT init_result EQUAL 0)
    message(FATAL_ERROR "zake init failed with exit code ${init_result}.\n${init_combined}")
endif()

foreach(required_file
    "${PROJECT_TEST_ROOT}/temp_project/zake.toml"
    "${PROJECT_TEST_ROOT}/temp_project/src/main.zk"
    "${PROJECT_TEST_ROOT}/temp_project/tests/hello_test.zk"
    "${PROJECT_TEST_ROOT}/temp_project/README.md")
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Expected generated file does not exist: ${required_file}")
    endif()
endforeach()

execute_process(
    COMMAND "${ZAKE_EXECUTABLE}" run
    WORKING_DIRECTORY "${PROJECT_TEST_ROOT}/temp_project"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_output
    ERROR_VARIABLE run_error
)

set(run_combined "${run_output}${run_error}")
string(REPLACE "\r\n" "\n" run_combined "${run_combined}")
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "zake run failed with exit code ${run_result}.\n${run_combined}")
endif()

string(FIND "${run_combined}" "Hello from Zake project" run_index)
if(run_index EQUAL -1)
    message(FATAL_ERROR "zake run output did not contain expected text.\n${run_combined}")
endif()

execute_process(
    COMMAND "${ZAKE_EXECUTABLE}" check
    WORKING_DIRECTORY "${PROJECT_TEST_ROOT}/temp_project"
    RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output
    ERROR_VARIABLE check_error
)

set(check_combined "${check_output}${check_error}")
string(REPLACE "\r\n" "\n" check_combined "${check_combined}")
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR "zake check failed with exit code ${check_result}.\n${check_combined}")
endif()

string(FIND "${check_combined}" "OK: src/main.zk" check_index)
if(check_index EQUAL -1)
    message(FATAL_ERROR "zake check output did not contain expected text.\n${check_combined}")
endif()
