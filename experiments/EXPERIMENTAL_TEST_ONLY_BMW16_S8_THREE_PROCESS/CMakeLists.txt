cmake_minimum_required(VERSION 3.17)
project(bmw16_s10_experimental_testonly LANGUAGES CXX)

set(MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER ON CACHE BOOL
    "Build opt-in TEST_ONLY BMW16-derived Select adapter" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../VFSS" "${CMAKE_BINARY_DIR}/vfss")

add_executable(bmw16_s10_experimental_backend fss_party_backend.cpp)
target_link_libraries(bmw16_s10_experimental_backend PRIVATE sytorch)
target_compile_features(bmw16_s10_experimental_backend PRIVATE cxx_std_17)
