#!/usr/bin/env bash
# User-local installation; versioned folders keep an active process intact.
set -euo pipefail
cd "$(dirname "$0")/.."
cmake --build --preset release --target microcosm_kids -j "${KIDS_BUILD_JOBS:-4}"
python3 - <<'PY'
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

root = Path.cwd()
base = Path.home() / '.local/opt/open-microcosm-kids'
base.mkdir(parents=True, exist_ok=True)
revision = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], text=True).strip()
destination = Path(tempfile.mkdtemp(prefix=revision + '-', dir=base))
shutil.copy2(root / 'build/release/microcosm_kids', destination)
shutil.copytree(root / 'assets', destination / 'assets')
for name in ('LICENSE', 'README.md'):
    shutil.copy2(root / name, destination / name)
licenses = destination / 'licenses'
licenses.mkdir()
sources = {
    'raylib.txt': 'raylib-src/LICENSE',
    'glfw.txt': 'raylib-src/src/external/glfw/LICENSE.md',
    'entt.txt': 'entt-src/LICENSE',
    'tomlplusplus.txt': 'tomlplusplus-src/LICENSE',
}
for name, source in sources.items():
    shutil.copy2(root / 'build/release/_deps' / source, licenses / name)
current = base / 'current'
if current.exists() and not current.is_symlink():
    raise SystemExit(f'Refusing to replace an ordinary directory: {current}')
temporary_link = base / (destination.name + '.link')
temporary_link.symlink_to(destination.name)
temporary_link.replace(current)
binary = Path.home() / '.local/bin/open-microcosm-kids'
binary.parent.mkdir(parents=True, exist_ok=True)
if not binary.exists():
    binary.write_text('#!/bin/sh\nexec ' + shlex.quote(str(current / 'microcosm_kids')) + ' "$@"\n')
    binary.chmod(0o755)
data = Path(os.environ.get('XDG_DATA_HOME', str(Path.home() / '.local/share')))
desktop = data / 'applications/open-microcosm-kids.desktop'
desktop.parent.mkdir(parents=True, exist_ok=True)
if not desktop.exists():
    # Desktop Entry Exec quoting differs from shell quoting.
    executable = str(binary).replace('\\', '\\\\').replace('"', '\\"').replace('`', '\\`').replace('$', '\\$').replace('%', '%%')
    desktop.write_text('[Desktop Entry]\nType=Application\nName=Open Microcosm Kids\n'
                       'Comment=Explore tiny living worlds\nExec="' + executable + '"\n'
                       'Icon=' + str(current / 'assets/kids/icon.png') + '\n'
                       'Terminal=false\nCategories=Education;Science;Biology;\n')
print(f'Installed: {destination}\nLauncher: {binary}\nMenu: {desktop}')
PY
