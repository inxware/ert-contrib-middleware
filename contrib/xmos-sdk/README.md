# xmos-sdk — XMOS SDK Sources for inxware

This directory holds the MIT-licensed XMOS SDK source repositories used to build
the pre-compiled libraries for xcore.ai FreeRTOS targets.

Pre-built `.a` files and headers are committed to:
```
ert-contrib-middleware/target_libs/xcore_freertos-xcore-xtc-15.x/build/
├── include/    ← headers referenced by ert-components toolchain.mk / target.mk
└── lib/        ← .a files linked by the inxware build
```

## Licensing

All source libraries listed here are **MIT licensed** and publicly hosted on XMOS GitHub.
They can be freely included in this repository. The XMOS XTC Tools compiler (`xcc`) is
proprietary and is **not** stored here — it lives only inside the Docker build container.

## SDK component sources

Clone each of the following into subdirectories of this directory. All are available from
the XMOS GitHub organisation at `https://github.com/xmos/`.

| Directory | Repository | Version / Tag | Purpose |
|-----------|-----------|---------------|---------|
| `fwk_rtos/` | github.com/xmos/fwk_rtos | v3.x (verify latest) | FreeRTOS SMP port for xcore.ai; includes FreeRTOS kernel, lwIP, FatFS |
| `fwk_io/` | github.com/xmos/fwk_io | (latest) | I/O framework; dependency of fwk_rtos (fetched via CMake FetchContent) |
| `xcommon_cmake/` | github.com/xmos/xcommon_cmake | v1.x | CMake build infrastructure (build-time only, not linked) |
| `lib_xcore_math/` | github.com/xmos/lib_xcore_math | v2.x | VPU-accelerated maths library |
| `lib_i2c/` | github.com/xmos/lib_i2c | v6.x | I2C master/slave |
| `lib_uart/` | github.com/xmos/lib_uart | v1.x | Async UART TX/RX |
| `lib_i2s/` | github.com/xmos/lib_i2s | v6.x | I2S audio interface |

### Initialising the submodules

These directories are git submodules of this repo (registered in the top-level
`.gitmodules`). From the repo root:

```bash
git submodule update --init contrib/xmos-sdk/*
```

`fwk_rtos` has its own nested submodules (FreeRTOS-SMP-Kernel, mbedtls, tinyusb,
etc.) which must also be fetched — the build container has no SSH client and
cannot pull them at build time:

```bash
git -C contrib/xmos-sdk/fwk_rtos submodule update --init --recursive
```

## Building the SDK libraries (one-time, per XTC Tools version)

The XMOS SDK libraries use CMake / XCommon CMake and must be built with `xcc` from
inside the Docker container (which has XTC Tools installed).

### Step 1 — Build the Docker image locally (first time only)

The XMOS Docker image contains XTC Tools and **must not be pushed to a public registry**.
Each developer builds it locally:

```bash
./configure xcore_freertos-xcore-base
make build_docker_local
```

### Step 2 — Start the build container interactively

```bash
make target_buildenv
```

This opens a shell inside the container with the full inxware workspace mounted at `/inxware`.

### Step 3 — Run the build script

```bash
cd /inxware/ert-contrib-middleware/contrib/xmos-sdk
./build-xmos-libs.sh
```

This script builds each library against the `XCORE-AI-EXPLORER` reference board target and
installs the resulting `.a` files and headers into:
```
/inxware/ert-contrib-middleware/target_libs/xcore_freertos-xcore-xtc-15.x/build/
```

### Step 4 — Commit the built artifacts

Exit the container and commit the populated `target_libs` directory to `ert-contrib-middleware`:

```bash
cd ert-contrib-middleware
git add target_libs/xcore_freertos-xcore-xtc-15.x/
git commit -m "feat(xmos): add pre-built SDK libs for XTC Tools 15.x"
```

After this, `make all_docker` for any `xcore_freertos-xcore-*` platform will find the
pre-built libraries and build without needing to rebuild the SDK.

## Upgrading the XTC Tools version

1. Update `XTC_VERSION` ARG in `ert-components/target/platform/xcore_freertos-xcore-base/Dockerfile`
2. Rebuild and publish the Docker image: `make publish_docker_image`
3. Update `TOOLCHAIN_NAME` and `COMPONENT_BASE_TECHNOLOGIES` in `config.mk` to match
4. Re-run `build-xmos-libs.sh` inside the new container and commit the new `target_libs/` tree
