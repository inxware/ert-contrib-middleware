# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Purpose

This repository contains contributed open-source middleware libraries used by the inxware eRT (event-driven Run-Time). It holds source code for third-party libraries (in `contrib/`) and pre-built static library artifacts (in `target_libs/`) for multiple target platforms.

The companion repo `ert-components` (https://github.com/inxware/ert-components) must be cloned alongside this one before building.

## Repository Structure

```
ert-contrib-middleware/
├── contrib/              # Source trees for each middleware library (alsa, gstreamer, gtk, aws-*, esp-idf, etc.)
├── inx_build_scripts/    # Build orchestration scripts (run from here)
│   ├── build-<target>.sh # Per-target top-level build scripts
│   └── source-scripts/   # Sourced helper scripts (not run directly)
│       ├── inx-xbuilder-source-me.sh          # Core cross-build environment setup (autotools targets)
│       ├── inx-xbuilder-source-me-espidf-s3.sh # ESP32-S3 IDF build environment
│       ├── inx-xbuilder-source-me-espidf.sh   # ESP32 IDF build environment
│       └── inx-dockersetup-source-me.sh       # Docker launch helper
├── target_libs/          # Output: pre-built libraries per target (committed artifacts)
└── sys-executables/      # Host system executable artifacts
```

## Build Commands

All build scripts must be **run from `inx_build_scripts/`**:

```bash
cd inx_build_scripts/

# Linux x86_64 targets (Debian 12 / clang11)
./build-x86_64-to-x86_64-linux-debian12.sh

# Linux x86_64 (Debian 10 / clang10)
./build-x86_64-to-x86_64-linux-clang.sh

# Linux arm64 cross-compiled on Debian 11
./build-arm64-gtk-gst-greengrass_debian11.sh

# ESP32-S3 (IDF 5.1.x)
./build-esp32s3-freertos-ehs-idf-5-1-x.sh

# ESP32-S3 with 8MB flash/PSRAM variant
./build-esp32s3-n8r2-freertos-ehs-idf-5-1-x.sh
```

Scripts automatically re-launch themselves inside the correct Docker container (via `inx-dockersetup-source-me.sh`) if not already running in Docker.

## Build System Architecture

### How a build target script works

1. Sets environment variables: `ARCH`, `OS`, `TOOLCHAIN_VERSION`, `TOOLCHAIN_BIN_PREFIX`, `INX_PROJECT_NAME`, `INX_CC/CXX/LD`, `PROCESSORS`
2. Sources `inx-dockersetup-source-me.sh` → calls `check_and_run_docker IMAGE_NAME` (re-execs in Docker if not already inside)
3. Sources `inx-xbuilder-source-me.sh` (or the ESP-IDF variant) which:
   - Derives `TARGET=${ARCH}-${OS}`, `OUTTARGET` (the output directory name), and `USRLIB_BUILD_ROOT`
   - Exports compiler env vars (`CC`, `CXX`, `LD`, `AR`, `RANLIB`, `CFLAGS`, `LDFLAGS`, `PKG_CONFIG_*`)
   - Defines `build_component()`, `build_aws_*()`, `build_openssl_component()` functions
4. Calls the build functions to configure and compile each library in dependency order

### Output directory naming

Autotools targets install to:
```
../target_libs/${ARCH}-${OS}-${TOOLCHAIN_VERSION}_${INX_GLIBC_VERSION}_${INX_PROJECT_NAME}/build/
```

ESP-IDF targets install to:
```
../target_libs/xtensa-${OS}-${TOOLCHAIN_VERSION}/build/
```

### Two build system variants

**Autotools (`inx-xbuilder-source-me.sh`)** — for Linux/Android targets:
- `build_component <name> <version> [config_flags] [cache] [extra_install] [make_subdir]`
  - Sources from `../contrib/<name>/<name><version>/`
  - Runs `./configure --prefix=${USRLIB_BUILD_ROOT} --host=${TARGET}` then `make && make install`
- `build_aws_lc`, `build_aws_c_common`, `build_aws_s2n`, `build_aws_c_cal`, `build_aws_c_io`, `build_aws_c_compression`, `build_aws_c_http`, `build_aws_c_mqtt`
  - CMake-based; build from `../contrib/<package>/build/`

**ESP-IDF (`inx-xbuilder-source-me-espidf-s3.sh`)** — for ESP32-S3:
- `build_component esp-idf <idf_version> <subcomponent>`
  - Uses a "dummy project" (blink example) in a staging dir (`../../ERT_CONTRIB_MIDDLEWARE_STAGING/`) to drive the IDF cmake/ninja build
  - Extracts `.a` files per subcomponent and copies headers to `USRLIB_INCLUDE_PATH`
- `build_bootloader`, `build_partition_table` — special functions for bootloader/partition table binaries

### Docker images used

| Target | Docker image |
|--------|-------------|
| x86_64 Debian 10/clang10 | `inxware/inx-debian10-clang10` |
| x86_64 Debian 11/clang11 | `inxware/inx-debian11-clang11` |
| arm64 Debian 11 cross | `inxware/inx-debian11-clang-arm` |
| ESP32-S3 | `inxware/esp32s3_ubuntu22.04-build-essential` |

Docker mounts the workspace root (`../../../`) as `/inxware` and sets working directory to `/inxware/ert-contrib-middleware/inx_build_scripts`.

### Toolchain location

Toolchains are expected at:
```
../../ert-build-support/toolchains/<host_arch>/<TOOLCHAIN_VERSION>/
```
(i.e., the `ert-build-support` repo must also be checked out at the same level)

### Controlling rebuild behavior

`REMAKE=true` (default) — clean + reconfigure + rebuild each component.
`REMAKE=false` — skip configure/clean, only copy existing artifacts.

### Adding a new autotools component

1. Place source tarball/directory under `../contrib/<name>/<name><version>/`
2. In the appropriate `build-<target>.sh`, add: `build_component <name> <version> [optional configure flags]`
3. Order matters: dependencies must be built before dependents

### Adding a new AWS/CMake component

Define a new `build_aws_<component>()` function in `source-scripts/inx-xbuilder-source-me.sh` following the existing pattern (pushd, rm/mkdir build, cmake with `${USRLIB_BUILD_ROOT}`, make, make install, popd).
