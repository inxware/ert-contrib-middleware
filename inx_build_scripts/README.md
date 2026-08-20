# ert-contrib-middleware build scripts

Cross-compilation build scripts for the third-party libraries consumed by
`ert-components`. Each script produces static artefacts under `../target_libs/`
in the directory layout expected by the ert-components build system.

Use `make chkconfig` in `ert-components` to confirm the `COMPONENT_BASE_TECHNOLOGIES`
path matches what a script produces.

---

## Android

### `build-android-ehs.sh`

Builds all EHS contrib libraries for a given Android ABI and API level.

**Usage**

```bash
cd inx_build_scripts
bash build-android-ehs.sh --abi <abi> --api <level> [--jobs <n>]
```

| Option | Values | Default |
|--------|--------|---------|
| `--abi` | `armeabi-v7a` · `arm64-v8a` · `x86` · `x86_64` | — (required) |
| `--api` | Android API level integer, e.g. `30` | — (required) |
| `--jobs` | Parallel build jobs | `8` |

**Examples**

```bash
# 32-bit ARM (Pine64 H6, Unity armeabi-v7a target)
bash build-android-ehs.sh --abi armeabi-v7a --api 30

# 64-bit ARM (modern devices, Unity AAB builds)
bash build-android-ehs.sh --abi arm64-v8a --api 30
```

**Docker (recommended)**

The script automatically re-launches itself inside
`inxware/ubuntu22.04-android-ndk-build` when Docker is available and the image
exists (via `check_and_run_docker` from `source-scripts/inx-dockersetup-source-me.sh`).
Build the image once from the same directory:

```bash
cd inx_build_scripts
docker build -f Dockerfile.android-ndk-build \
             -t inxware/ubuntu22.04-android-ndk-build .
```

The image provides cmake 3.22, Android NDK r27c at `/opt/android-ndk-r27c`,
and all other prerequisites. After the image is built, subsequent script
invocations use it automatically.

**Prerequisites (when running without Docker)**

| Tool | Min version | Notes |
|------|-------------|-------|
| `cmake` | 3.16 | For the LiteRT static build |
| `git` | 2.25 | Sparse-checkout support |
| `python3` | 3.8 | TFLite CMake code-generation steps |
| `rsync` | any | Header installation |
| `wget`, `unzip` | any | Other component downloads |

The NDK compiler toolchain is taken from
`ert-build-support/toolchains/<host-arch>/armv7a-linux-android/` (this
directory contains compilers for all Android ABIs, not just armv7a).

**NDK discovery order** (cmake and NDK are located in this priority order):

1. `ANDROID_NDK` / `CMAKE` environment variables (manual override)
2. Docker image standard paths (`/opt/android-ndk-r27c`, cmake on `PATH`)
3. `ert-build-support/toolchains/` (stripped NDK)
4. Unity Android Player SDK/NDK (developer workstation fallback)

**Libraries built**

| Library | Version | Type | Notes |
|---------|---------|------|-------|
| libpthread-stubs | 0.3 | static | |
| OpenSSL | 1.0.1u | static | Old but matches existing EHS usage |
| curl | 7.88.1 | static | Built `--without-libidn` (see below) |
| libarchive | 3.6.1 | static | |
| expat | 2.0.1 | static | |
| libxml2 | latest in contrib | static | |
| LiteRT (TFLite C API) | 2.14.0 | static | `liblitert_c.a` — see notes below |

**libidn — not built for Android**

Older scripts (`build-armv7a-linux-android-ehs.sh` etc.) built libidn 1.16 or
1.30 to give curl IDNA (internationalized domain name) support.  This is no
longer built for three reasons:

1. Android's bionic libc provides its own IDNA implementation; libidn is
   redundant on Android.
2. libidn 1.33 requires `gtkdocize` (gtk-doc-tools) during `autoreconf`,
   which is not available in the NDK build container and not worth pulling in.
3. libidn 1.16/1.30 contain inline asm that NDK r27c's clang 18 rejects.

curl is built `--without-libidn`; IDNA hostname resolution falls through to
bionic's built-in support at runtime.

**Output paths** (relative to `ert-contrib-middleware/`)

```
target_libs/
  armv7a-linux-android/build/lib/      ← static .a files
  armv7a-linux-android/build/include/  ← headers
  arm64-linux-android/build/lib/
  arm64-linux-android/build/include/
staging/                               ← local only, gitignored
  src/litert-v2.14.0/                 ← shared LiteRT source (fetched once)
  build/litert-armeabi-v7a-api30/     ← per-ABI CMake build tree
  build/litert-arm64-v8a-api30/
```

**LiteRT / XNNPACK notes**

- Built as a single merged fat static archive (`liblitert_c.a`) containing
  TFLite, flatbuffers, abseil, ruy, and all other dependencies.
- **XNNPACK is currently disabled.**  The XNNPACK commit pinned by TFLite 2.14.0
  (`b9d4073`) uses an `=t` (x87 FPU) inline-asm constraint on the ARM32 target,
  which clang 18 in NDK r27c rejects.  Upgrading to TFLite 2.16+ (which pins a
  fixed XNNPACK commit) will re-enable NEON/SIMD acceleration.
- NNAPI is disabled — deprecated since Android 13 and removed from Android 15.
- GPU delegate support is a future addition.
- The source tree is cloned once into `staging/src/` and reused across ABI
  builds; only the CMake build tree is per-ABI.
- To update LiteRT: change `LITERT_VERSION` in `build-android-ehs.sh` and
  delete the corresponding `staging/src/litert-v<old>/` and
  `staging/build/litert-*/` directories, then re-run.

**Android API level guidance**

The `--api` parameter sets the Android API level used for both the NDK toolchain
and the LiteRT CMake build.  Current and future targets:

| API | Android version | Status (as of 2026) |
|-----|-----------------|---------------------|
| 21  | 5.0 Lollipop    | EOL — do not use |
| 26  | 8.0 Oreo        | Minimum for Google Play new apps (2019 policy) |
| 28  | 9.0 Pie         | Used by some older inxware Android 9 targets |
| 30  | 11.0            | **Current inxware target** — still very widespread |
| 33  | 13.0            | Required target for new Play Store submissions since 2023 |
| 34  | 14.0            | Required for new Play Store submissions since Aug 2024 |
| 35  | 15.0            | Expected Play Store requirement ~2025/2026 |

For **2026 devices**, API 34 (Android 14) is the realistic minimum for new
Google Play submissions; devices shipping in 2026 will run Android 14–15.
API 30 remains a reasonable NDK min-SDK for compatibility with devices from
2020 onwards (Android 11 still has significant install base).

The 32-bit armeabi-v7a ABI continues to run on all Android versions but Google
has required 64-bit APK support since August 2019.  Unity AAB (Android App
Bundle) builds deliver both armeabi-v7a and arm64-v8a slices automatically.

---

*Further platforms (Linux x86\_64, Linux ARM64, etc.) will be documented here
as their scripts are consolidated.*
