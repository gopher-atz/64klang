#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_TYPE="${1:-Release}"

mkdir -p "$REPO_ROOT/build"
cd "$REPO_ROOT/build"
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" "$REPO_ROOT/64klang3"
make -j"$(nproc)"
