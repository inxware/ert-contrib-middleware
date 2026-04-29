/*
 * main_stub.c — minimal XMOS entry point for the xcore-ehs-app link wrapper.
 *
 * xcc requires at least one compilation unit when producing a .xe to
 * resolve the XMOS runtime entry point and tile scheduling metadata.
 * All real eRT code lives in the ehs_lib static library (Phase 1 output).
 *
 * The actual main() / par / on tile[] entry is expected to come from
 * ehs_lib (target_main.c in ert-components xcore_freertos-xcore os-arch).
 * This stub exists only to satisfy the linker.
 *
 * TODO: if xcommon_cmake or fwk_rtos requires a specific XC entry point
 * (e.g. an .xc file with on tile[] declarations), replace this stub with
 * the appropriate .xc file.
 */

/* Intentionally empty — eRT entry point is in ehs_lib. */
