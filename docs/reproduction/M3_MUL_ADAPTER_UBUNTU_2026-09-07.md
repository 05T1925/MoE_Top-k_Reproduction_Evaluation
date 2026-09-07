# M3 masked multiplication adapter validation

Date:
2026-09-07

Branch:
m3-protocol-iii-mul-adapter

Revision:
9a944fc

## Environment

OS:

Ubuntu 24.04

Kernel:

Linux fss-VMware-Virtual-Platform

Compiler:

gcc 13.3.0

CMake:

3.28.3


## Build

Command:

```bash
cmake -S VFSS -B /tmp/moe-m3-mul-9a944fc \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF
