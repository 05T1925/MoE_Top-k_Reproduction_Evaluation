cmake_minimum_required(VERSION 3.17)
project(bmw16_s7_test_only LANGUAGES CXX)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../VFSS" "${CMAKE_BINARY_DIR}/vfss")

add_executable(bmw16_s7_fss_testonly fss_testonly.cpp)
target_link_libraries(bmw16_s7_fss_testonly PRIVATE sytorch)
target_compile_features(bmw16_s7_fss_testonly PRIVATE cxx_std_17)
