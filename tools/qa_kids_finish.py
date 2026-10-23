#!/usr/bin/env python3
"""Native-input completion review; see tools/kids_native_review.py."""
import argparse
from pathlib import Path
from kids_native_review import NativeReview

parser = argparse.ArgumentParser()
parser.add_argument('--captures', type=Path, required=True)
args = parser.parse_args()
review = NativeReview(args.captures)
click, snapshot = review.click, review.snapshot
import time

click(1150, 683)
click(1200, 749)
snapshot('11-onion-salt-start', route=3, salty=1)
# A change is an engine measurement, not a scripted success screen.
deadline = time.monotonic() + 90
while time.monotonic() < deadline:
    time.sleep(3)
    state = snapshot('12-onion-change', salty=1)
    if state['changed']:
        break
assert state['changed'], 'No measured shrinkage within the review window'
click(1200, 774)
snapshot('13-fresh-water', salty=0)
click(1320, 64)
snapshot('14-experiment-reset', salty=0, changed=0)

# Complete the detective case and observe the explanation.
click(105, 347)
click(970, 650)
click(1200, 677)
snapshot('15-detective-answer', route=1, answer=0)

# Complete all story chapters, then enter exploration.
click(105, 593)
click(970, 650)
snapshot('16-story-start', selected=0, chapter=0)
click(1200, 668)
snapshot('17-story-second', chapter=1)
click(1200, 668)
snapshot('18-story-third', chapter=2)
click(1200, 668)
snapshot('19-story-free-explore', route=0, activity=1)

click(105, 740)
snapshot('20-notebook-return', notebook=1)
click(100, 135)
snapshot('21-home-return', landing=1, activity=0, notebook=0)
print('Full journey interactions verified.')
