# M3 Environment Baseline Ubuntu 2026-09-07

## Repository

Branch:

m3-env-baseline

Commit:

<填 git rev-parse HEAD>


## Build Environment

OS:

<uname>


Compiler:

<gcc>


CMake:

<cmake>


## Configuration

Build:

build-m3


Options:

- BUILD_TESTING=ON
- CMAKE_BUILD_TYPE=Debug
- MOE_TOPK_ENABLE_EMP_OT=ON


EMP prefix:

<填 CMAKE_PREFIX_PATH>


## M2 Regression

Passed:

- chosen OT
- OPV
- share translation
- permute share
- secret shared shuffle
- modular E2E


## M3 Start Point

M2 frozen.

Beginning:

Secure MoE layer reproduction.

Pipeline:

Secret Input
→ Gate Routing
→ Secure Dispatch
→ Expert Compute
→ Secure Combine
→ Secret Output
