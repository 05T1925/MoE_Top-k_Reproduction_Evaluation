cmake_minimum_required(VERSION 3.17)
project(m6a_p2_i_allpairs_experiment LANGUAGES CXX)
include(CTest)

# Isolated experiment target; the production source tree is only added as a dependency.
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../VFSS ${CMAKE_CURRENT_BINARY_DIR}/vfss)
add_executable(m6a_p2_i_material_pool_test material_pool_test.cpp)
target_link_libraries(m6a_p2_i_material_pool_test PRIVATE sytorch)
target_compile_features(m6a_p2_i_material_pool_test PRIVATE cxx_std_17)
add_test(NAME m6a_p2_i_material_pool_test
  COMMAND m6a_p2_i_material_pool_test
    ${CMAKE_CURRENT_LIST_DIR}/aav86_n5_r2_test_edges.csv)
