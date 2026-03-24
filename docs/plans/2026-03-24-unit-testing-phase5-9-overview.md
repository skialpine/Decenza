# Unit Testing Phases 5-9: Bug-Finding Coverage

> **For Claude:** Work through each phase sequentially. Build and run `ctest --output-on-failure` after each test file. All expected values must be derived from de1app upstream Tcl source. Only move to the next phase when all tests pass.

**Goal:** Add ~120 tests across 5 test files targeting areas most likely to contain undiscovered bugs: cross-system integration points, data fidelity, and protocol parsing.

**Source of truth:** de1app upstream at `/Users/jeffreyh/Documents/GitHub/de1app` (submodules updated) for behavioral expectations. UI is Decenza's own design.

## Phase Documents

| Phase | File | Tests | Focus |
|-------|------|:-----:|-------|
| 5 | [Phase 5: TCL Profile Import](2026-03-24-unit-testing-phase5.md) | ~30 | Load all de1app TCL profiles, round-trip through Decenza parser |
| 6 | [Phase 6: BLE Fidelity](2026-03-24-unit-testing-phase6.md) | ~25 | Built-in profile → BLE bytes → decode → compare |
| 7 | [Phase 7: DB Migrations](2026-03-24-unit-testing-phase7.md) | ~20 | Schema v0→v9 chain, data preservation, FTS |
| 8 | [Phase 8: WeightProcessor](2026-03-24-unit-testing-phase8.md) | ~25 | LSLR, de-jitter, oscillation recovery, edge cases |
| 9 | [Phase 9: Scale Protocols](2026-03-24-unit-testing-phase9.md) | ~20 | BLE packet parsing for Decent, Bookoo, Acaia scales |

## Why These Areas

| Area | Bug Signal | What Tests Will Find |
|------|-----------|---------------------|
| TCL import | 5 bug-fix PRs at 83% fix rate | Format mismatches between de1app TCL and Decenza parser |
| BLE fidelity | TargetEspressoVol=36 bug class | Silent corruption in encode→decode path |
| DB migrations | 4+ rounds of migration fixes | Data loss, schema drift, FTS breakage |
| WeightProcessor | 75-80% fix rate, LSLR rework | Timing edge cases, de-jitter calibration, oscillation |
| Scale parsing | Bookoo tare bug, Acaia protocol | Malformed packets, boundary values, sign handling |
