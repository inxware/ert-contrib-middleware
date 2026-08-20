#!/bin/bash
# fetch-aws-crt-sources.sh
#
# Downloads and extracts versioned AWS CRT source tarballs into contrib/.
# Run once before build-x86_64-to-x86_64-linux-clang19-debian13.sh
# (and before build-arm64-gtk-gst-greengrass_debian13.sh for consistency).
#
# Safe to re-run: skips any directory that already exists.

set -e

SCRIPTS_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTRIB_ROOT="$(cd "${SCRIPTS_DIR}/.." && pwd)"

fetch() {
    local URL="$1"
    local DEST="$2"
    local TARBALL="${DEST##*/}.tar.gz"

    if [ -d "${DEST}" ]; then
        echo "EXISTS: ${DEST}"
        return
    fi

    local TMP
    TMP=$(mktemp -d)
    wget -q "${URL}" -O "${TMP}/${TARBALL}"
    tar xzf "${TMP}/${TARBALL}" -C "${TMP}"
    rm "${TMP}/${TARBALL}"

    local EXTRACTED
    EXTRACTED=$(ls "${TMP}")
    mv "${TMP}/${EXTRACTED}" "${DEST}"
    rmdir "${TMP}"
    echo "FETCHED: ${DEST}"
}

C="${CONTRIB_ROOT}/contrib"

fetch "https://github.com/aws/aws-lc/archive/refs/tags/v1.69.0.tar.gz" \
      "${C}/aws-lc/aws-lc-v1.69.0"

fetch "https://github.com/awslabs/aws-c-common/archive/refs/tags/v0.12.6.tar.gz" \
      "${C}/aws-c-common/aws-c-common-v0.12.6"

# s2n-tls tags have no 'v' prefix
fetch "https://github.com/aws/s2n-tls/archive/refs/tags/1.7.1.tar.gz" \
      "${C}/s2n-tls/s2n-tls-1.7.1"

fetch "https://github.com/awslabs/aws-c-cal/archive/refs/tags/v0.9.13.tar.gz" \
      "${C}/aws-c-cal/aws-c-cal-v0.9.13"

fetch "https://github.com/awslabs/aws-c-io/archive/refs/tags/v0.26.1.tar.gz" \
      "${C}/aws-c-io/aws-c-io-v0.26.1"

fetch "https://github.com/awslabs/aws-c-compression/archive/refs/heads/main.tar.gz" \
      "${C}/aws-c-compression/aws-c-compression"

fetch "https://github.com/awslabs/aws-c-http/archive/refs/tags/v0.10.11.tar.gz" \
      "${C}/aws-c-http/aws-c-http-v0.10.11"

fetch "https://github.com/awslabs/aws-c-mqtt/archive/refs/tags/v0.14.0.tar.gz" \
      "${C}/aws-c-mqtt/aws-c-mqtt-v0.14.0"

echo "All AWS CRT sources ready."
