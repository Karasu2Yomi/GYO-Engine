add_executable(input_tests
    InputActionMapTests.cpp
)

target_link_libraries(input_tests PRIVATE
    GYO::Input
    doctest::doctest
)

target_compile_features(input_tests PRIVATE cxx_std_20)

add_test(NAME input_tests COMMAND input_tests)
