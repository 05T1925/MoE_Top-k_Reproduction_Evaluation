# M2.8 EMP chosen-OT Ubuntu reproduction record

Implementation label: `m2_emp_iknp_chosen_ot_conformance`.

## Result

**NOT_RUN / blocked before configuration.** The controlling Windows host has
no installed/registered `Ubuntu` WSL distribution (`Wsl/Service/WSL_E_DISTRO_NOT_FOUND`).
Its available GNU toolchain is MinGW/MSYS, while the pinned `emp-tool` CMake
source explicitly accepts only Linux or Darwin. No Windows configure, mock
transport, direct selection, or borrowed performance result was used.

This record is deliberately not an Ubuntu validation claim. It records the
exact failed prerequisite so a supported host can repeat the qualification
without ambiguity.

## Pinned input and observed archive hashes

| archive | codeload commit | SHA-256 |
| --- | --- | --- |
| `emp-tool.tar.gz` | `97f335927dd7d38caaf5e80d93fca70edddd5423` | `7f4a2cb169ba0b7fc48ffe89b7615288c41f9377bb0a4d56a3178fe20b66ab46` |
| `emp-ot.tar.gz` | `03acb042b98e82fd5fd0da33babd44801f8ec082` | `8cfffc340a2014e5ac3b90c6659c0a812a6748e7e24c6ea2b0f5b3e58ba66183` |

## Required supported-host command sequence

```bash
set -eu
root=/tmp/m28_emp_qualification
prefix="$root/prefix"
curl -fsSL https://codeload.github.com/emp-toolkit/emp-tool/tar.gz/97f335927dd7d38caaf5e80d93fca70edddd5423 -o "$root/emp-tool.tar.gz"
curl -fsSL https://codeload.github.com/emp-toolkit/emp-ot/tar.gz/03acb042b98e82fd5fd0da33babd44801f8ec082 -o "$root/emp-ot.tar.gz"
sha256sum "$root/emp-tool.tar.gz" "$root/emp-ot.tar.gz"
tar -xzf "$root/emp-tool.tar.gz" -C "$root"
tar -xzf "$root/emp-ot.tar.gz" -C "$root"
cmake -S "$root/emp-tool-97f335927dd7d38caaf5e80d93fca70edddd5423" -B "$root/tool-build" \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="$prefix" -DEMP_TOOL_NATIVE_ARCH=OFF
cmake --build "$root/tool-build" --parallel
cmake --install "$root/tool-build"
cmake -S "$root/emp-ot-03acb042b98e82fd5fd0da33babd44801f8ec082" -B "$root/ot-build" \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$prefix" -DEMP_OT_AUTO_TUNE=OFF
cmake --build "$root/ot-build" --parallel
ctest --test-dir "$root/ot-build" --output-on-failure -R 'test_(base_ot|iknp)'
cmake --build "$root/ot-build" --verbose > "$root/emp-ot-link.txt" 2>&1
cmake -S VFSS -B "$root/vfss-off" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$root/vfss-off" --parallel
ctest --test-dir "$root/vfss-off" --output-on-failure
cmake -S VFSS -B "$root/vfss-on" -DCMAKE_BUILD_TYPE=Debug -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DEMP_TOOL_DIR="$prefix/lib/cmake/emp-tool" -DEMP_OT_DIR="$prefix/lib/cmake/emp-ot"
cmake --build "$root/vfss-on" --parallel
ctest --test-dir "$root/vfss-on" --output-on-failure
```

Expected only after the source implementation exists: upstream base-OT + IKNP
smoke passes; default-off CTest discovers 11 and passes 11/11; enabled CTest
discovers 12 and passes 12/12. At this revision all three are `NOT_MEASURED`.
Performance, byte counts, rounds, seeds, runtime topology and link output are
also `NOT_MEASURED`; none has been inferred from an EMP upstream test.
