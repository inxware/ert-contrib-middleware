# eRT-contrib-middleware
Contains contributed open surce middleware libraries used for some configirations of 
the inxware eRT (event driven Run-Time). 

https://www.inx-systems.com/platform/

# To Use This Repo to build a platfrom 

First clone the ert-components repo from https://github.com/inxware/ert-components (or the private repo)

## XMOS SDK submodules

The XMOS SDK components under `contrib/xmos-sdk/` are pulled in as git submodules
from `github.com/xmos/*`. After cloning this repo, initialise them with:

```bash
git submodule update --init contrib/xmos-sdk/*
```

Treat them as read-only third-party sources — do not commit changes back to them.
See `contrib/xmos-sdk/README.md` for the full component list and build steps.



# To Add to or Modify This Repository 

cd to inx-build scripts
./build_all [targetname]

where target name is $OS_$ARCH

e.g.

linux_x86

For each component the following build directories are created:

built libs, includes and pkconfigs (where appropriate) in $COMPOMENT/target_lib_builds

The target manefest (compnent support) is created in $COMPOMENT/target_cs_pckages 

An aggregation of $COMPOMENT/target_lib_builds is built in ../ target_libs
This is for building against and contains all normall install info with this prefix.

# LiteRT / TFLite — why staging/ not contrib/

All other middleware packages live permanently in `contrib/<name>/<name><version>/` and are
committed to the repo.  TFLite is handled differently:

- **Source is not checked in.**  The TFLite sparse checkout is several hundred megabytes, and
  its cmake build pulls further dependencies (XNNPACK, abseil, flatbuffers, cpuinfo, …) at
  configure time — gigabytes in total.  Committing any of this is impractical.
- **`staging/` is gitignored** (see `.gitignore`).  `inx_build_scripts/source-scripts/litert-build.sh`
  clones the pinned version tag into `staging/src/litert-v<VERSION>/` on first run and reuses it
  across ABI builds.  CMake build trees land in `staging/build/litert-<ABI>-api<LEVEL>/`.
- **Artefacts are committed.**  The final output (`liblitert_c.a` + C API headers) is installed
  to `target_libs/<ABI>-linux-android/build/` and committed like any other package.
- **Cleanup.**  `staging/build/` (cmake intermediates, ~1.5 GB per ABI) should be deleted after a
  successful build.  `staging/src/` (~200 MB sparse checkout) can be kept to skip re-cloning on
  the next ABI build; delete it only to upgrade `LITERT_VERSION`.

# Using Claude Code in this repository

`CLAUDE.md` at the repo root is the primary context file for Claude Code.  If you are running
Claude from `ert-components` but need to make changes here, open a separate Claude Code session
rooted at `ert-contrib-middleware/`, or navigate here with `cd ../ert-contrib-middleware` in the
terminal pane.  The `CLAUDE.md` in this repo will be picked up automatically.
