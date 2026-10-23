#!/usr/bin/env python3
"""Native-input smoke review; see tools/kids_native_review.py."""
import argparse
from pathlib import Path
from kids_native_review import NativeReview

parser = argparse.ArgumentParser()
parser.add_argument('--captures', type=Path, required=True)
args = parser.parse_args()
review = NativeReview(args.captures)
click, snapshot = review.click, review.snapshot
import time

snapshot('01-home', landing=1, activity=0)
click(100, 265)
snapshot('02-habitats', route=0, activity=0, landing=0)
click(1200, 729)
snapshot('03-moss-live', route=0, activity=1)
click(319, 774)
click(530, 196)
snapshot('04-paused-diagram', paused=1, diagram=1)
click(1040, 774)
click(850, 624)
click(870, 774)
time.sleep(.5)
state = snapshot('05-photo-saved')
assert state['photos'] >= 1
click(105, 740)
snapshot('06-notebook', notebook=1)
click(105, 429)
click(1200, 410)
snapshot('07-amoeba', route=2, activity=1, selected=1)
click(105, 347)
click(970, 650)
snapshot('08-detectives', route=1, activity=1)
click(105, 593)
click(970, 650)
snapshot('09-story', route=4, activity=1, selected=0, chapter=0)
click(105, 511)
click(970, 650)
snapshot('10-onion-before', route=3, activity=1, salty=0)
print('Initial journeys verified. Review captures before continuing experiment actions.')
