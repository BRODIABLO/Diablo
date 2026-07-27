cmake_minimum_required(VERSION 3.28)
project(RemoteStashLayoutPolicy LANGUAGES CXX)

add_executable(RemoteStashLayoutPolicyTests layout_policy_tests.cpp)
target_compile_features(RemoteStashLayoutPolicyTests PRIVATE cxx_std_20)

enable_testing()
add_test(NAME remote-stash-layout-policy COMMAND RemoteStashLayoutPolicyTests)
