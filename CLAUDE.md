# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Purpose

This repository contains contributed open-source middleware libraries used by the inxware eRT (event-driven Run-Time). It holds source code for third-party libraries (in `contrib/`) and pre-built static library artifacts (in `target_libs/`) for multiple target platforms.

The companion repo `ert-components` (https://github.com/inxware/ert-components) must be cloned alongside this one before building.

## Repository Structure

```
ert-contrib-middleware/
├── contrib/              # Source trees for each middleware library
│   ├── <name>/           # One directory per library (e.g. aws-c-common/, curl/, esp-idf/)
│   │   └── <name><version>/  # Versioned source tree (e.g. aws-c-common-v0.12.6/)
│   └── ...
├── inx_build_scripts/    # Build orchestration scripts (must be run from here)
│   ├── build-<target>.sh         # Per-target top-level build scripts
│   ├── toolchain-aarch64.cmake   # CMake cross-compile toolchain for aarch64 with clang
│   └── source-scripts/           # Sourced helper scripts (not run directly)
│       ├── inx-xbuilder-source-me.sh          # Core build environment + build functions (autotools + cmake)
│       ├── inx-xbuilder-source-me-espidf-s3.sh # ESP32-S3 IDF build environment
│       ├── inx-xbuilder-source-me-espidf.sh   # ESP32 IDF build environment (older)
│       └── inx-dockersetup-source-me.sh       # Docker launch helper
├── target_libs/          # Output: pre-built libraries per target (committed artifacts)
└── sys-executables/      # Host system executable artifacts
```

## Contrib Source Directory Convention

All source trees follow the pattern `contrib/<name>/<name><version>/`. Multiple versions can coexist:

```
contrib/aws-c-common/
    aws-c-common-2.2022/    # legacy snapshot
    aws-c-common-v0.12.6/   # upstream release tag
contrib/s2n-tls/
    s2n-tls-2.2022/
    s2n-tls-1.7.1/          # note: s2n-tls tags have no 'v' prefix
```

Current AWS CRT component versions downloaded from upstream:

| Component | Legacy | Latest |
|-----------|--------|--------|
| aws-c-common | -2.2022 | -v0.12.6 |
| aws-c-cal | -2.2022 | -v0.9.13 |
| aws-c-compression | -2.2022 | -v0.3.2 |
| aws-c-http | -2.2022 | -v0.10.11 |
| aws-c-io | -2.2022 | -v0.26.1 |
| aws-c-mqtt | -2.2022 | -v0.14.0 |
| aws-lc | -2.2022 | -v1.69.0 |
| s2n-tls | -2.2022 | -1.7.1 |

## Build Commands

All build scripts must be **run from `inx_build_scripts/`**. They auto-relaunch inside
the correct Docker container via `inx-dockersetup-source-me.sh` if not already in Docker.

```bash
cd inx_build_scripts/

# Linux arm64, Debian 13 / clang19 — new AWS CRT versions
./build-arm64-gtk-gst-greengrass_debian13.sh

# Linux arm64, Debian 11 / clang11 — legacy AWS CRT versions
./build-arm64-gtk-gst-greengrass_debian11.sh

# Linux arm64, Debian 10 / clang10 — legacy AWS CRT versions
./build-arm64-gtk-gst-greengrass_debian10.sh

# Linux x86_64, Debian 12 / clang11
./build-x86_64-to-x86_64-linux-debian12.sh

# Linux x86_64, Debian 10 / clang10
./build-x86_64-to-x86_64-linux-clang.sh

# ESP32-S3 base variant (IDF 5.1.x)
./build-esp32s3-freertos-ehs-idf-5-1-x.sh

# ESP32-S3 8MB PSRAM variant
./build-esp32s3-n8r2-freertos-ehs-idf-5-1-x.sh

# ESP32-S3 8MB PSRAM + 16K stack variant
./build-esp32s3-n8r2_16k-freertos-ehs-idf-5-1-x.sh

# ESP32-S3 UART/USB + 16MB variant
./build-esp32s3-uartusb-n16r2-freertos-ehs-idf-5-1-x.sh
```

## Build System Architecture

### How a build target script works

1. Sets environment variables: `ARCH`, `OS`, `TOOLCHAIN_VERSION`, `TOOLCHAIN_BIN_PREFIX`,
   `INX_PROJECT_NAME`, `INX_CC/CXX/LD`, `PROCESSORS`
2. Sources `inx-dockersetup-source-me.sh` → calls `check_and_run_docker IMAGE_NAME`
   — detects Docker via `/.dockerenv`; re-execs the script inside the container if not already there
3. Sources `inx-xbuilder-source-me.sh` (or the ESP-IDF variant) which:
   - Derives `TARGET=${ARCH}-${OS}`, `OUTTARGET` (output directory name), `USRLIB_BUILD_ROOT`
   - Exports compiler env vars (`CC`, `CXX`, `LD`, `AR`, `RANLIB`, `CFLAGS`, `LDFLAGS`, `PKG_CONFIG_*`)
   - Pauses with `read -n 1` before building (requires interactive TTY or piped stdin)
   - Defines `build_component()`, `build_cmake_component()`, `build_openssl_component()`
4. Calls the build functions to configure and compile each library in dependency order

### Output directory naming

Linux/autotools targets install to:
```
../target_libs/${ARCH}-${OS}-${TOOLCHAIN_VERSION}_${INX_GLIBC_VERSION}_${INX_PROJECT_NAME}/build/
```
e.g. `target_libs/arm64-linux-gnu-clang19_debian13_base/build/`

ESP-IDF targets install to:
```
../target_libs/xtensa-${OS}-${TOOLCHAIN_VERSION}/build/
```

### Build functions in `inx-xbuilder-source-me.sh`

**`build_component <name> <version> [config_flags] [cache] [extra_install] [make_subdir]`**
- For autotools (configure/make) components
- Sources from `../contrib/<name>/<name><version>/`
- Runs `./configure --prefix=${USRLIB_BUILD_ROOT} --host=${TARGET}` then `make && make install`

**`build_cmake_component <name> <version> [cmake_args]`**
- For CMake components (AWS CRT libraries, s2n-tls, etc.)
- Sources from `../contrib/<name>/<name><version>/`
- Creates `build/` subdir inside the source tree, runs cmake + make + make install
- Resolves any relative `CMAKE_TOOLCHAIN_FILE` path to absolute using `${TEMP_PWD}`
- Uses `${PROCESSORS}` for parallel make

**`build_openssl_component <name> <version> <platform>`**
- For OpenSSL (uses `./Configure` instead of `./configure`)

### Adding a new autotools component

1. Place source under `contrib/<name>/<name><version>/`
2. In the appropriate `build-<target>.sh`, add: `build_component <name> <version> [flags]`
3. Order matters — dependencies must be built before dependents

### Adding a new CMake component

1. Place source under `contrib/<name>/<name><version>/`
2. In the appropriate `build-<target>.sh`, add: `build_cmake_component <name> <version> [cmake_flags]`

### `toolchain-aarch64.cmake`

Located at `inx_build_scripts/toolchain-aarch64.cmake`. Sets clang as the compiler with
`aarch64-linux-gnu` as the target triple. Used by arm64 build scripts via
`-DCMAKE_TOOLCHAIN_FILE=toolchain-aarch64.cmake` — `build_cmake_component` automatically
resolves this to an absolute path so cmake can find it from the build subdirectory.

### ESP-IDF build system (`inx-xbuilder-source-me-espidf-s3.sh`)

Used by all `build-esp32s3-*.sh` scripts. Different from the Linux build system:

- `build_component esp-idf <idf_version> <subcomponent>` — drives IDF cmake/ninja via
  a "dummy project" (blink example) in a staging dir (`../../ERT_CONTRIB_MIDDLEWARE_STAGING/`),
  then extracts the compiled `.a` and headers for that subcomponent
- `build_bootloader esp-idf <idf_version> bootloader` — builds and copies bootloader binary
- `build_partition_table esp-idf <idf_version> partition_table` — builds partition table binary
- After all `build_component` calls, each script runs a large set of `cp` commands to collect
  headers and pre-built binary blobs (wifi, phy, coex, bt) into `USRLIB_INCLUDE_PATH` /
  `USRLIB_LIBRARY_PATH`, and combines mbedtls sub-libraries into a single `libmbedtls.a`

### Docker images used

The canonical Docker image name for each platform is defined in:
```
../ert-components/target/platform/<platform-name>/Dockerimagename
```
Each file contains a single line with the image name (e.g. `inxware/inx-debian13-arm64`).
The build scripts read these indirectly — the `IMAGE_NAME` set at the top of each
`build-<target>.sh` must match the corresponding `Dockerimagename` file for that platform.

Images relevant to this repo's build scripts:

| Build script | Platform dir | Docker image |
|---|---|---|
| `build-arm64-gtk-gst-greengrass_debian13.sh` | `linux_arm64_debian13_base` | `inxware/inx-debian13-arm64` |
| `build-arm64-gtk-gst-greengrass_debian11.sh` | `linux_arm64_gtk_gst_gg_debian11` | `inxware/inx-debian11-clang-arm` |
| `build-arm64-gtk-gst-greengrass_debian10.sh` | `linux_arm64_gtk_gst_gg_debian10` | `inxware/inx-debian10-clang-arm` |
| `build-x86_64-to-x86_64-linux-debian12.sh` | `linux_x86_64_clang_gg_debian11` | `inxware/inx-debian11-clang11` |
| `build-x86_64-to-x86_64-linux-clang.sh` | `linux_x86_64_clang_gg_debian10` | `inxware/inx-debian10-clang10` |
| `build-esp32s3-*-idf-5-1-x.sh` | `esp32s3_freertos-xtensa-base` | `inxware/esp32s3_ubuntu22.04-build-essential` |

All images are `amd64`. The arm64 images contain aarch64 cross-compilation toolchains.
Docker mounts the workspace root as `/inxware` and sets working directory to
`/inxware/ert-contrib-middleware/inx_build_scripts`.

### Toolchain location

Toolchains are expected at:
```
../../ert-build-support/toolchains/<host_arch>/<TOOLCHAIN_VERSION>/
```
(`ert-build-support` repo must be checked out alongside this one)

### Controlling rebuild behaviour

`REMAKE=true` (default) — clean + reconfigure + rebuild each component.
`REMAKE=false` — skip configure/clean, only copy existing artifacts.

## Known Issues and Consistency Gaps

### aws-lc requires `-DDISABLE_GO=ON`
`aws-lc` v1.69.0+ requires Go for code generation. Docker images do not include Go.
Pass `-DDISABLE_GO=ON` in cmake args so aws-lc uses its pre-generated sources instead.
The `build-arm64-gtk-gst-greengrass_debian13.sh` script already does this; any other
script targeting the new `aws-lc -v1.69.0` must also include it.

### Bug in `inx-xbuilder-source-me.sh` line 106
`if -z ${INX_PROJECT_NAME}` should be `if [ -z "${INX_PROJECT_NAME}" ]`.
This causes a non-fatal error message on every build but does not break it because
`INX_GLIBC_VERSION` falls back to `OS_VERSION` correctly.

### Legacy `-2.2022` versions still used by most build scripts
Only `build-arm64-gtk-gst-greengrass_debian13.sh` uses the new upstream-tagged versions.
The debian10, debian11, and x86_64 scripts still reference `-2.2022`. When updating
those targets to use the new versions, remember to add `-DDISABLE_GO=ON` for aws-lc.

### ESP32-S3 script duplication
The four `build-esp32s3-*-idf-5-1-x.sh` scripts are nearly identical. They differ only in:
- `OS=` variable (determines output target directory name and dummy project variant)
- Component list: base variant builds `bt` instead of `esp_psram`; n8r2/uartusb/n8r2_16k
  variants build `esp_psram` instead

The entire `cp` section (headers, blobs, linker files, mbedtls combination) is duplicated
verbatim across all four scripts. This should be refactored into a shared sourced script
(e.g. `source-scripts/inx-espidf-s3-copy-artifacts.sh`) called from each variant script.

### `inx-dockersetup-source-me.sh` requires interactive TTY
`docker run` is called with `-it`, which requires a TTY. When running non-interactively
(e.g. from CI), pass `-i` only, or set `DOCKER_RUNNING=true` as an environment variable
and restore the commented-out env-var check at the top of `check_and_run_docker`.
