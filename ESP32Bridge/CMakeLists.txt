cmake_minimum_required(VERSION 3.22)

# Do not silently set IDF_PATH or replace an already selected target/toolchain.
if(NOT DEFINED ENV{IDF_PATH} OR NOT DEFINED ENV{IDF_PYTHON_ENV_PATH})
    message(FATAL_ERROR "Activate the specified ESP-IDF v6.0.0 environment first")
endif()
file(REAL_PATH "$ENV{IDF_PATH}" bridge_idf_path)
file(REAL_PATH "D:/esp/v6.0/esp-idf" bridge_expected_idf_path)
if(NOT bridge_idf_path STREQUAL bridge_expected_idf_path)
    message(FATAL_ERROR "IDF_PATH must select D:/esp/v6.0/esp-idf")
endif()
foreach(bridge_target "${IDF_TARGET}" "$ENV{IDF_TARGET}")
    if(NOT bridge_target STREQUAL "" AND NOT bridge_target STREQUAL "esp32s3")
        message(FATAL_ERROR "ESP32Bridge only supports IDF_TARGET=esp32s3")
    endif()
endforeach()
set(IDF_TARGET "esp32s3" CACHE STRING "ESP32Bridge target")
set(bridge_python "$ENV{IDF_PYTHON_ENV_PATH}/Scripts/python.exe")
if(NOT EXISTS "${bridge_python}")
    message(FATAL_ERROR "Selected IDF Python environment is missing; no tools are installed automatically")
endif()

set(bridge_local "${CMAKE_CURRENT_LIST_DIR}/.local/sdkconfig.local")
set(bridge_check "${CMAKE_CURRENT_LIST_DIR}/scripts/check_config.py")
execute_process(
    COMMAND "${bridge_python}" "${bridge_check}" --local "${bridge_local}" --environment
    RESULT_VARIABLE bridge_result OUTPUT_VARIABLE bridge_output ERROR_VARIABLE bridge_error
)
if(NOT bridge_result STREQUAL "0")
    message(FATAL_ERROR "${bridge_output}${bridge_error}")
endif()
message(STATUS "${bridge_output}")

# Check cached credentials before Kconfig can diagnose a malformed secret line.
if(SDKCONFIG)
    set(bridge_existing_config "${SDKCONFIG}")
else()
    set(bridge_existing_config "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")
endif()
if(EXISTS "${bridge_existing_config}")
    execute_process(
        COMMAND "${bridge_python}" "${bridge_check}" --local "${bridge_local}" --sdkconfig "${bridge_existing_config}"
        RESULT_VARIABLE bridge_result OUTPUT_VARIABLE bridge_output ERROR_VARIABLE bridge_error
    )
    if(NOT bridge_result STREQUAL "0")
        message(FATAL_ERROR "${bridge_output}${bridge_error}")
    endif()
endif()

# Force the documented defaults and secret source. Existing sdkconfig is checked
# after Kconfig so stale credentials cannot silently override the local file.
set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults;${bridge_local}")
include("${bridge_idf_path}/tools/cmake/project.cmake")
idf_build_set_property(MINIMAL_BUILD ON)
set(PROJECT_VER "0.1.0-dev")
project(esp32bridge)

execute_process(
    COMMAND "${bridge_python}" "${bridge_check}" --local "${bridge_local}" --sdkconfig "${SDKCONFIG}"
    RESULT_VARIABLE bridge_result OUTPUT_VARIABLE bridge_output ERROR_VARIABLE bridge_error
)
if(NOT bridge_result STREQUAL "0")
    message(FATAL_ERROR "${bridge_output}${bridge_error}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${bridge_local}" "${bridge_check}")
