cmake_minimum_required(VERSION 3.17)
project(sytorch)
include(CTest)

if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang\$")
    find_path(LIBOMP_INCLUDE_DIR omp.h
        HINTS /opt/homebrew/opt/libomp/include /usr/local/opt/libomp/include)
    find_library(LIBOMP_LIBRARY omp
        HINTS /opt/homebrew/opt/libomp/lib /usr/local/opt/libomp/lib)
    if(NOT LIBOMP_INCLUDE_DIR OR NOT LIBOMP_LIBRARY)
        message(FATAL_ERROR "Apple Clang requires libomp; install it with Homebrew.")
    endif()
    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP_INCLUDE_DIR}")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY "${LIBOMP_LIBRARY}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings -Wno-unused-result -maes -Wno-ignored-attributes -march=native -Wno-deprecated-declarations")
find_package (Eigen3 3.3 REQUIRED NO_MODULE)
find_package(Threads REQUIRED)
find_package(OpenMP COMPONENTS CXX REQUIRED)
# find_package(CUDAToolkit REQUIRED)

add_subdirectory(ext/cryptoTools)
add_subdirectory(ext/bitpack)
add_subdirectory(ext/FSS)
# add_subdirectory(ext/sci)
# add_subdirectory(../SCI ext/sci)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} 
PRIVATE
    src/random.cpp
    src/backend/cleartext.cpp
    src/backend/baseline_cleartext.cpp
    src/backend/float.cpp
    src/moe_topk/protocol_i_ucmp.cpp
    src/moe_topk/protocol_i_cmpagg.cpp
    src/moe_topk/protocol_i_transport.cpp
    src/moe_topk/protocol_i_party_package.cpp
    # src/softmax.cpp
)

target_include_directories(${PROJECT_NAME}
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries (${PROJECT_NAME} Eigen3::Eigen Threads::Threads OpenMP::OpenMP_CXX FSS cryptoTools) # SCI-FloatML CUDA::cudart


# === Benchmarks ===
# DCF，GROTTO，GTDCF 比较
add_executable(
    comparison_primitive
    tests/bench/comparison_primitive.cpp
)
target_link_libraries(comparison_primitive ${PROJECT_NAME})


# 二叉树、四叉树、八叉树的对比，加入AES-IN的工程优化，减少密钥生成时间
add_executable(
    octdpf_test
    tests/bench/octdpf_test.cpp
)
target_link_libraries(octdpf_test ${PROJECT_NAME})


# 多叉树FSS的理论对比，这里没有用AES-IN的工程优化，每次AES加密都是独立实现的
add_executable(
    academic_benchmark
    tests/bench/academic_benchmark.cpp
)
target_link_libraries(academic_benchmark ${PROJECT_NAME})

# 测试VDPF效率
add_executable(
    verdpf_test
    tests/bench/verdpf_test.cpp
)
target_link_libraries(verdpf_test ${PROJECT_NAME})

# 测试DPF和VDPF密钥大小、密钥生成和评估时间
add_executable(
    mp_dpf_verdpf_test
    tests/bench/mp_dpf_verdpf_test.cpp
)
target_link_libraries(mp_dpf_verdpf_test ${PROJECT_NAME})

# IFSS测试
add_executable(
    shark_ifss
    tests/bench/shark_ifss.cpp
)
target_link_libraries(shark_ifss ${PROJECT_NAME})

# half-tree测试
add_executable(
    halftree_fss_test
    tests/bench/halftree_fss_test.cpp
)
target_link_libraries(halftree_fss_test ${PROJECT_NAME})

# DPF、half-tree DPF 和 WDPF的测试
add_executable(
    WDPF_test
    tests/bench/WDPF_test.cpp
)
target_link_libraries(WDPF_test ${PROJECT_NAME})

# DPF、half-tree DPF 、Grotto和 WDPF的测试
add_executable(
    WDCF_test
    tests/bench/WDCF_test.cpp
)
target_link_libraries(WDCF_test ${PROJECT_NAME})

# VDCF
add_executable(
    VDCF_test
    tests/bench/VDCF_test.cpp
)
target_link_libraries(VDCF_test ${PROJECT_NAME})

# VWDCF
add_executable(
    VWDCF_test
    tests/bench/VWDCF_test.cpp
)
target_link_libraries(VWDCF_test ${PROJECT_NAME})

# VIDPF
add_executable(
    VIDPF_test
    tests/bench/VIDPF_test.cpp
)
target_link_libraries(VIDPF_test ${PROJECT_NAME})


# mp_vidpf_spline
add_executable(
    mp_cavern_spline
    tests/bench/mp_cavern_spline.cpp
)
target_link_libraries(mp_cavern_spline ${PROJECT_NAME})

add_executable(
    mp_cavern_relu
    tests/bench/mp_cavern_relu.cpp
)
target_link_libraries(mp_cavern_relu ${PROJECT_NAME})

# DIF测试
add_executable(
    dif
    tests/bench/dif.cpp
)
target_link_libraries(dif ${PROJECT_NAME})



# 协议测试
add_executable(
    GTDCF-ReLU
    tests/bench/GTDCF-ReLU.cpp
)
target_link_libraries(GTDCF-ReLU ${PROJECT_NAME})




# === E2E ===
# add_executable(
#     BPGNN
#     tests/e2e/BPGNN.cpp
# )
# target_link_libraries(BPGNN ${PROJECT_NAME})



# === E2E ===
add_executable(
    CNN
    tests/e2e/CNN.cpp
)
target_link_libraries(CNN ${PROJECT_NAME})

# M1 conformance tests use the repository's existing CMake target structure.
add_executable(
    moe_topk_m1_metrics_test
    tests/moe_topk/metrics_test.cpp
)
target_link_libraries(moe_topk_m1_metrics_test ${PROJECT_NAME})

add_executable(
    moe_topk_m1_dcf_conformance_test
    tests/moe_topk/dcf_conformance_test.cpp
)
target_link_libraries(moe_topk_m1_dcf_conformance_test ${PROJECT_NAME})

add_executable(
    moe_topk_m1_oracle_test
    tests/moe_topk/topk_oracle_test.cpp
)
target_link_libraries(moe_topk_m1_oracle_test ${PROJECT_NAME})

add_executable(
    moe_topk_m1_cmpagg_test
    tests/moe_topk/cmpagg_test.cpp
)
target_link_libraries(moe_topk_m1_cmpagg_test ${PROJECT_NAME})

# M2.4 C/B conformance only: no Protocol I runtime or secure shuffle.
add_executable(moe_topk_m2_priority_key_test tests/moe_topk/protocol_i_priority_key_test.cpp)
target_link_libraries(moe_topk_m2_priority_key_test ${PROJECT_NAME})
add_executable(moe_topk_m2_priority_dcf_conformance_test tests/moe_topk/protocol_i_priority_dcf_conformance_test.cpp)
target_link_libraries(moe_topk_m2_priority_dcf_conformance_test ${PROJECT_NAME})
add_executable(moe_topk_m2_reverse_shuffle_model_test tests/moe_topk/protocol_i_reverse_shuffle_model_test.cpp)
target_link_libraries(moe_topk_m2_reverse_shuffle_model_test ${PROJECT_NAME})
add_executable(moe_topk_m2_ucmp_conformance_test tests/moe_topk/protocol_i_ucmp_conformance_test.cpp)
target_link_libraries(moe_topk_m2_ucmp_conformance_test ${PROJECT_NAME})
add_executable(moe_topk_m2_cmpagg_conformance_test tests/moe_topk/protocol_i_cmpagg_conformance_test.cpp)
target_link_libraries(moe_topk_m2_cmpagg_conformance_test ${PROJECT_NAME})
add_executable(moe_topk_m2_transport_conformance_test tests/moe_topk/protocol_i_transport_conformance_test.cpp)
target_link_libraries(moe_topk_m2_transport_conformance_test ${PROJECT_NAME})
add_executable(moe_topk_m2_cmpagg_process_e2e_test tests/moe_topk/protocol_i_cmpagg_three_process_e2e_test.cpp)
target_link_libraries(moe_topk_m2_cmpagg_process_e2e_test ${PROJECT_NAME})

# Protocol III pre-integration primitive conformance. This validates the
# existing VFSS DPF implementation and transport without starting M3 routing.
add_executable(
    moe_topk_dpf_conformance_test
    tests/moe_topk/dpf_conformance_test.cpp
)
target_link_libraries(moe_topk_dpf_conformance_test FSS)

if(BUILD_TESTING)
    add_test(NAME moe_topk_m1_metrics_test COMMAND moe_topk_m1_metrics_test)
    add_test(NAME moe_topk_m1_dcf_conformance_test COMMAND moe_topk_m1_dcf_conformance_test)
    add_test(NAME moe_topk_m1_oracle_test COMMAND moe_topk_m1_oracle_test)
    add_test(NAME moe_topk_m1_cmpagg_test COMMAND moe_topk_m1_cmpagg_test)
    add_test(NAME moe_topk_m2_priority_key_test COMMAND moe_topk_m2_priority_key_test)
    add_test(NAME moe_topk_m2_priority_dcf_conformance_test COMMAND moe_topk_m2_priority_dcf_conformance_test)
    add_test(NAME moe_topk_m2_reverse_shuffle_model_test COMMAND moe_topk_m2_reverse_shuffle_model_test)
    add_test(NAME moe_topk_m2_ucmp_conformance_test COMMAND moe_topk_m2_ucmp_conformance_test)
    add_test(NAME moe_topk_m2_cmpagg_conformance_test COMMAND moe_topk_m2_cmpagg_conformance_test)
    add_test(NAME moe_topk_m2_transport_conformance_test COMMAND moe_topk_m2_transport_conformance_test)
    add_test(NAME moe_topk_m2_cmpagg_process_e2e_test COMMAND moe_topk_m2_cmpagg_process_e2e_test)
endif()
