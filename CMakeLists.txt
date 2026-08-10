cmake_minimum_required(VERSION 3.16)
project(plainspeak CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra)

add_executable(plainspeak
    src/lexer/tokenizer.cpp
    src/parser/parser.cpp
    src/sema/sema.cpp
    src/codegen/mangling.cpp
    src/codegen/c_emitter.cpp
    src/cli/main.cpp
)
target_include_directories(plainspeak PRIVATE src)

# The compiled plainspeak binary shells out to the system C compiler at
# *run time* to build user programs, and needs to find the runtime source
# and header regardless of the working directory it's invoked from.
target_compile_definitions(plainspeak PRIVATE
    PLAINSPEAK_RUNTIME_C="${CMAKE_SOURCE_DIR}/runtime/plainspeak_runtime.c"
    PLAINSPEAK_RUNTIME_DIR="${CMAKE_SOURCE_DIR}/runtime"
)
