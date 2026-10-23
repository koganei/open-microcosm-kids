# Inspecting and exporting notched cells index beyond process tables

| Field | Value |
|---|---|
| **ID** | `inspection-notch-overflow` |
| **Type** | bug |
| **Status** | fixed |
| **Severity** | critical |
| **Area** | [Interface](../mechanics/interface.md) |
| **Affects** | Inspection of notched organisms, including Paramecium |
| **Code** | `src/game/inspect.cpp`, `inspectCell`; `src/game/export.cpp`, `writeSpecies` |
| **Reported** | 23/10/2026 |
| **Last verified** | 23/10/2026, upstream `72e10a234f39` |
| **Fixed in** | `bbecab3` |

## Summary

The inspector counts four protrusion kinds using four-element stack arrays.
`ProcessKind::Notch` is a fifth kind and writes beyond those arrays. The full
catalog inspection test aborts with `free(): invalid pointer`; a debugger places
the invalid free inside `inspectCell`. The JSON exporter has the same four-item
lookup and throws when the fifth entry is read as a null string.

## Steps to reproduce

1. Build the upstream release with `BIO_NATIVE=OFF` on GCC 13/Linux x86-64.
2. Run `OMP_NUM_THREADS=1 ./build/release/microcosm_tests --test-case='fichas: todas*'`.
3. Inspect Paramecium, which has a factory notch process.

## Expected and observed

Expected: a valid card with an indentation described as an indentation.
Observed: stack corruption and an invalid free. A notch is also biologically
incorrectly grouped with protruding horns, spines, arms and setae.

## Diagnosis and fix

Count notches separately, reject unknown protrusion indices before indexing the
arrays, give indentations their own card line, and add the missing export kind
with a compile-time size check. No simulation parameter or
organism geometry changes. Add a focused Paramecium regression and rerun the
catalog test under AddressSanitizer/UndefinedBehaviorSanitizer.

## Verification

The original build reproduced the abort in the full catalog test. UBSan then
reported `index 4 out of bounds for type int[4]` in the inspector. After the
patch, the catalog and focused notch tests pass under AddressSanitizer and
UndefinedBehaviorSanitizer (2 cases, 2,206 assertions; leak checking disabled).
The two export/physiology cases also pass (52,693 assertions). A source audit
found and fixed the corresponding fifth-kind lookup in the JSON exporter.
No biological state, morphology or physiology was changed.

## Workaround

Avoid inspecting notched cells in the unpatched original laboratory. The kids
interface uses its own explanation cards; its Amoeba food action uses the
original inspector to obtain a radius, so the shared helper must still be safe.

## References

`ProcessKind` and `ProcessSpec` in `src/bio/species.hpp`; factory Paramecium data;
`tests/test_inspect.cpp`.

## History

- 23/10/2026: reproduced during the kids fork's broader regression review.
