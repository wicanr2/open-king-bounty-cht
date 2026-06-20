#!/bin/sh
# 在 docker image 內編譯 openkb。先 fetch-vendor,再 autogen/configure/make。
set -e
cd "$(dirname "$0")/.."

sh docker/fetch-vendor.sh

sh autogen.sh >/dev/null 2>&1
./configure >/dev/null 2>&1

# libhfs / librsrc 子模組需各自 configure
(cd vendor/libhfs && ./configure >/dev/null 2>&1)
(cd vendor/librsrc && ./configure >/dev/null 2>&1)

make "$@"

echo "=== build 完成 ==="
ls -la openkb 2>/dev/null
