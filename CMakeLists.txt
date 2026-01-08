cmake_minimum_required(VERSION 3.10)
project(Merk LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Dir Setup
set(MERK_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(MERK_MAIN "${CMAKE_SOURCE_DIR}/main.cpp")
set(MERK_CODE_DIR "${CMAKE_SOURCE_DIR}/code")
set(MERK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${MERK_INCLUDE_DIR})

file(GLOB_RECURSE MERK_SOURCES "${MERK_SRC_DIR}/*.cpp")
list(APPEND MERK_SOURCES "${CMAKE_SOURCE_DIR}/debugger_config.cpp")
list(APPEND MERK_SOURCES "${CMAKE_SOURCE_DIR}/a_info.cpp")

add_executable(merk ${MERK_MAIN} ${MERK_SOURCES})

message(STATUS "Include directories: ${MERK_INCLUDE_DIR}")
# === Build configurations and Compiler specific flags ===

# Apply global flags for MSVC (Windows compiler)
if(MSVC)
    # Fix the PDB locking errors (/FS) and C++20 macro errors (/Zc:preprocessor)
    target_compile_options(merk PRIVATE "/FS" "/Zc:preprocessor")
    # Use -Wall (for MSVC it's just /W4 or /Wall, we use the simple form)
    target_compile_options(merk PRIVATE -Wall) 
else()
    # Flags for GCC/Clang (Linux/Ubuntu)
    target_compile_options(merk PRIVATE -Wall -Wextra -pedantic)
endif()

# Apply configuration specific flags (Debug/Release)
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT CMAKE_BUILD_TYPE)
    message(STATUS "Configuring Debug build")
    # Sanitizers are Linux/GCC specific and complex on Windows, removed for MSVC compatibility
    if(NOT MSVC)
         target_compile_options(merk PRIVATE -g -fsanitize=address -fno-omit-frame-pointer)
         target_link_options(merk PRIVATE -fsanitize=address)
    endif()
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Configuring Release build")
    target_compile_options(merk PRIVATE -O3 -march=native -DNDEBUG)
endif()


# === Debug macro toggle ===
# set(ENABLE_DEBUG ON BOOL "Enable debug logging")
set(ENABLE_DEBUG ON) 
if(ENABLE_DEBUG)
    target_compile_definitions(merk PRIVATE ENABLE_DEBUG=1)
else()
    target_compile_definitions(merk PRIVATE ENABLE_DEBUG=0)
endif()

message(STATUS "Debug logging: ${ENABLE_DEBUG}")

# === Custom run target ===
add_custom_target(run
    COMMAND ./merk ${MERK_CODE_DIR}/test1.merk
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Run Merk with test1.merk"
)

set_target_properties(merk PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

message(STATUS "Debug logging: ${ENABLE_DEBUG}")

find_package(CURL REQUIRED)
target_link_libraries(merk PRIVATE CURL::libcurl)


# target_sources(merk PRIVATE "${MERK_SRC_DIR}/utilities/lsan_defaults.cpp")
