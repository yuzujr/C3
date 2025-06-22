add_executable(test_pty
    test_pty.cpp
)

target_link_libraries(test_pty
    PRIVATE
        core
        gtest_main
        gtest
)

# 让 gtest 自动发现测试
include(GoogleTest)
gtest_discover_tests(test_pty)
