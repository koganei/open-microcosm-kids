# Complex candidate division tests fail

| Field | Value |
|---|---|
| **ID** | `candidate-division-regressions` |
| **Type** | bug |
| **Status** | investigating |
| **Severity** | major |
| **Area** | [Mechanics](../mechanics/README.md) |
| **Affects** | Ceratium, Gonium, Micrasterias, Pandorina, Ochromonas, Staurastrum candidate division tests |
| **Code** | `tests/test_candidates.cpp`, division lifecycle and geometry |
| **Reported** | 23/10/2026 |
| **Last verified** | 23/10/2026, `49759ee` plus kids UI worktree; biological source unchanged from fork base |
| **Fixed in** | None |

## Summary

The complete suite reports missing daughter cells or non-simple daughter boundary
polygons in six candidate division cases. The full run has not yet terminated;
this inventory may grow. These species are not offered by the kids interface.

## Steps to reproduce

Build `microcosm_tests` in release and run:

```sh
OMP_NUM_THREADS=1 ctest --test-dir build/release --output-on-failure -j 2
```

For a focused reproduction use the case name, for example:

```sh
./build/release/microcosm_tests --test-case='candidatos: Ceratium se divide'
```

## Expected behaviour

The existing regression contract requires at least two cells after forced
reproduction within 180 simulation seconds, followed by non-self-intersecting
daughter boundaries. These are test expectations, not new literature claims.

## Actual behaviour

Ceratium and Staurastrum retain one cell at the timeout. Gonium, Micrasterias,
Pandorina and Ochromonas produce a failed `simplePolygon` check after division.

## Evidence

CTest targets `microcosm_largos_20`, `_40`, `_47`, `_50`, `_49` and `_62`
respectively fail at `test_candidates.cpp:57` or `:64`. The ongoing workstation
run is logged at `/tmp/microcosm-kids-all-tests.log`. This file records observed
failures rather than relying on an unavailable CI badge.

## Cause

Unknown. Complex boundaries, rest-shape reconstruction and closure constraints
need investigation. No causal conclusion has been established from these tests.

## Proposed fix

Trace division progress and daughter topology at the failing seeded steps.
Separate numerical geometry errors from absent species-specific division
mechanisms before changing code. Do not relax assertions or tune physiology to
obtain a green suite.

## Workaround

Keep these cases out of curated child journeys. They remain available in the
original research app, and their failures remain visible in the test suite.

## References

`tests/test_candidates.cpp`; [kids review](../design/kids-review.md).

## History

- 23/10/2026: recorded from the complete fork acceptance run; investigation open.
