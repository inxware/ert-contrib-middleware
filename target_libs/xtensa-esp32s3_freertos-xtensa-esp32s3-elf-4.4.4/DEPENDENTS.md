# DEPENDENTS.md — manual record (utility / build scripts)

This directory is **not** selected by any platform config in ert-components
(`target/platform/*/config.mk`), so the automatic reverse-dependency mechanism
in `target/platform/sbom.mk` never records it. It is referenced directly by the
scripts below and **must not be deleted as "unused"** without checking them.

Recorded 2026-08-19 by a dependency-usage sweep of ert-components,
ert-contrib-middleware, ert-build-support and EHS-kernel.

| Referenced by                                                          |
|:-----------------------------------------------------------------------|
| `ert-components/scripts/build-deploy/esp32/esp32_flash.sh` |

Note: matching is by directory-name substring, so a reference may belong to a
longer name that contains this one. Verify before acting on this file.
