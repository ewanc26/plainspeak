cmake_minimum_required(VERSION 3.16)
project(plainspeak CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra)

include(FetchContent)
FetchContent_Declare(
    catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.0.tar.gz
)
FetchContent_MakeAvailable(catch2)

enable_testing()

add_executable(plainspeak
    src/lexer/tokenizer.cpp
    src/lexer/alias_table.cpp
    src/parser/parser.cpp
    src/sema/sema.cpp
    src/codegen/mangling.cpp
    src/codegen/c_emitter.cpp
    src/ast/ast_printer.cpp
    src/cli/main.cpp
)
target_include_directories(plainspeak PRIVATE src)

target_compile_definitions(plainspeak PRIVATE
    PLAINSPEAK_RUNTIME_C="${CMAKE_SOURCE_DIR}/runtime/plainspeak_runtime.c"
    PLAINSPEAK_RUNTIME_DIR="${CMAKE_SOURCE_DIR}/runtime"
)

add_executable(unit_tests
    tests/unit/test_lexer.cpp
    tests/unit/test_parser.cpp
    tests/unit/test_sema.cpp
    tests/unit/test_lists.cpp
    tests/unit/test_types.cpp
    tests/unit/test_c_type_queries.cpp
    tests/unit/test_native_objects.cpp
    tests/unit/test_native_arrays.cpp
    tests/unit/test_typed_procedures.cpp
    tests/unit/test_native_structures.cpp
    tests/unit/test_native_unions.cpp
    tests/unit/test_aggregate_initializers.cpp
    tests/unit/test_native_enumerations.cpp
    tests/unit/test_aggregate_layout_members.cpp
    tests/unit/test_type_qualifiers.cpp
    tests/unit/test_integer_promotions.cpp
    src/lexer/tokenizer.cpp
    src/lexer/alias_table.cpp
    src/parser/parser.cpp
    src/sema/sema.cpp
)
target_include_directories(unit_tests PRIVATE src)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)
add_test(NAME unit_tests COMMAND unit_tests)

add_executable(runtime_tests
    tests/unit/test_runtime.c
    runtime/plainspeak_runtime.c
)
target_include_directories(runtime_tests PRIVATE runtime)
target_link_libraries(runtime_tests PRIVATE m)
add_test(NAME runtime_tests COMMAND runtime_tests)
