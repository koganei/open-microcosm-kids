"""Small native-input driver for repeatable screenshots of the actual application."""
import json
from pathlib import Path
import subprocess
import time


def xdo(*args):
    return subprocess.check_output(['xdotool', *map(str, args)], text=True).strip()


class NativeReview:
    def __init__(self, captures):
        self.captures = Path(captures)
        windows = xdo('search', '--name', '^Open Microcosm Kids$').splitlines()
        if len(windows) != 1:
            raise RuntimeError('Run exactly one Kids application for this review.')
        self.window = windows[0]
        xdo('windowactivate', '--sync', self.window)

    def click(self, x, y):
        xdo('windowactivate', '--sync', self.window)
        geom = dict(row.split('=', 1) for row in
                    xdo('getwindowgeometry', '--shell', self.window).splitlines())
        width, height = int(geom['WIDTH']), int(geom['HEIGHT'])
        scale = min(width / 1440, height / 900)
        px = round((width - 1440 * scale) / 2 + x * scale)
        py = round((height - 900 * scale) / 2 + y * scale)
        xdo('mousemove', '--window', self.window, px, py)
        xdo('mousedown', 1)
        time.sleep(.4)
        xdo('mouseup', 1)
        time.sleep(.5)

    def snapshot(self, name, **expected):
        prior = set(self.captures.glob('ui-*.json'))
        xdo('windowactivate', '--sync', self.window)
        xdo('keydown', '--window', self.window, 'F10')
        try:
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                found = set(self.captures.glob('ui-*.json')) - prior
                if not found:
                    time.sleep(.1)
                    continue
                path = found.pop()
                try:
                    state = json.loads(path.read_text())
                except json.JSONDecodeError:
                    time.sleep(.1)
                    continue
                for key, value in expected.items():
                    assert state[key] == value, (name, key, state[key], value)
                (self.captures / (name + '.json')).write_text(
                    json.dumps(state, indent=2) + '\n')
                (self.captures / (name + '.png')).write_bytes(
                    path.with_suffix('.png').read_bytes())
                print(name, 'PASS', flush=True)
                return state
            raise RuntimeError('No screenshot emitted by the live application')
        finally:
            xdo('keyup', '--window', self.window, 'F10')

    def close(self):
        # Send the normal window-manager close request. XDestroyWindow leaves a
        # GLFW process alive and can turn an unattended review into a CPU leak.
        subprocess.run(['wmctrl', '-ic', hex(int(self.window))], check=True)
