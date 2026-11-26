enable_language(CXX)

add_executable(test_deos_protocol_cpp test_deos_protocol.cpp)
target_link_libraries(test_deos_protocol_cpp PRIVATE deos_protocol)
set_target_properties(test_deos_protocol_cpp PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_deos_protocol_cpp PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME deos_protocol_cpp COMMAND test_deos_protocol_cpp)

find_program(DEOS_PYTHON3 NAMES python3)
if(DEOS_PYTHON3)
    add_test(
        NAME deos_protocol_python
        COMMAND ${DEOS_PYTHON3} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR} -p "test_*.py" -v
    )
    set_tests_properties(deos_protocol_python PROPERTIES
        ENVIRONMENT "DEOS_PROTOCOL_LIB=$<TARGET_FILE:deos_protocol_shared>"
    )
endif()
