#!/bin/bash
######################################################################################################
### Source this script to get check_and_run_docker().
###
### Usage in a build script:
###
###   IMAGE_NAME=inxware/my-image
###   source ./source-scripts/inx-dockersetup-source-me.sh
###   check_and_run_docker "${IMAGE_NAME}" "$@"
###
### If not already inside a Docker container, check_and_run_docker exec-replaces the
### calling process with a docker run invocation that re-runs the calling script with
### the same arguments ($@) inside the container.  The inxware workspace root is
### mounted at /inxware and the working directory is set to
### /inxware/ert-contrib-middleware/inx_build_scripts, matching the host layout.
###
### If the image is not found locally a docker pull is attempted before giving up
### and continuing on the host.
###
### Arguments:
###   $1   Docker image name (required)
###   $@   Remaining arguments are forwarded to the re-launched script unchanged.
###
### The function is a no-op (returns 0) when:
###   - already running inside Docker (/.dockerenv exists), or
###   - the image cannot be found locally or pulled.
######################################################################################################

check_and_run_docker() {
    local _IMAGE="$1"
    shift   # remaining positional args are the caller's original "$@"

    # Already inside a container — nothing to do.
    if [ -f /.dockerenv ]; then
        echo "Already running in Docker — continuing..."
        return 0
    fi

    # Locate the inxware workspace root.
    # The calling script lives in inx_build_scripts/; two levels up is inxware/.
    local _CALLING_SCRIPT="$0"
    local _SCRIPTS_DIR
    _SCRIPTS_DIR="$(cd "$(dirname "${_CALLING_SCRIPT}")" 2>/dev/null && pwd)"
    local _INXWARE_ROOT
    _INXWARE_ROOT="$(cd "${_SCRIPTS_DIR}/../.." 2>/dev/null && pwd)"

    # Attach a terminal only when stdin is interactive (avoids -it failures in CI/cron).
    local _TTY=""
    [ -t 0 ] && _TTY="-t"

    _do_docker_run() {
        echo "Re-launching inside Docker (${_IMAGE})..."
        exec docker run --rm \
            --network=host \
            --user "$(id -u):$(id -g)" \
            -i ${_TTY} \
            -v "${_INXWARE_ROOT}:/inxware" \
            -w "/inxware/ert-contrib-middleware/inx_build_scripts" \
            "${_IMAGE}" \
            bash "$(basename "${_CALLING_SCRIPT}")" "$@"
    }

    if docker image inspect "${_IMAGE}" &>/dev/null; then
        _do_docker_run "$@"
    else
        echo "Docker image ${_IMAGE} not found locally — attempting pull..."
        if docker pull "${_IMAGE}" &>/dev/null; then
            _do_docker_run "$@"
        else
            echo "NOTE: ${_IMAGE} not available — continuing on host."
        fi
    fi
}
