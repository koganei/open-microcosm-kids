#!/usr/bin/env bash
# Build with system headers, or extract development packages without installing.
set -euo pipefail
cd "$(dirname "$0")/.."
configure_args=(-DBIO_NATIVE=OFF)
if [[ ${1:-} == --local-headers ]]; then
    mkdir -p build/dev-packages build/sysroot
    (
        cd build/dev-packages
        apt-get download libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
            libxi-dev libgl-dev libglx-dev libxext-dev libxrender-dev libxfixes-dev \
            libxau-dev libxdmcp-dev libxcb1-dev x11proto-dev xtrans-dev \
            libopengl-dev libegl-dev libgles-dev
        for package in *.deb; do dpkg-deb -x "$package" ../sysroot; done
    )
    python3 - <<'PY'
from pathlib import Path
for link in Path('build/sysroot/usr/lib/x86_64-linux-gnu').glob('*.so'):
    if link.is_symlink() and not link.exists():
        target = Path('/usr/lib/x86_64-linux-gnu') / link.readlink()
        if target.exists():
            link.unlink()
            link.symlink_to(target.resolve())
PY
    configure_args+=("-DCMAKE_PREFIX_PATH=$PWD/build/sysroot/usr"
        "-DCMAKE_INCLUDE_PATH=$PWD/build/sysroot/usr/include"
        "-DCMAKE_LIBRARY_PATH=$PWD/build/sysroot/usr/lib/x86_64-linux-gnu"
        "-DCMAKE_C_FLAGS=-isystem $PWD/build/sysroot/usr/include"
        "-DCMAKE_CXX_FLAGS=-isystem $PWD/build/sysroot/usr/include")
fi
cmake --preset release "${configure_args[@]}"
cmake --build --preset release --target microcosm_kids microcosm_kids_tests -j "${KIDS_BUILD_JOBS:-4}"
