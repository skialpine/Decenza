# Unit Testing Plan: Bug-Driven Priority Coverage

> **For Claude:** Work through each phase document sequentially. Each phase is a separate file. Build and run `ctest --output-on-failure` after each test file. All expected values must be derived from de1app Tcl source code as the authoritative reference. All tests must be fully automated — no manual verification steps.

**Goal:** Add automated unit tests prioritized by bug density and code churn from the last 45 days (87 PRs, 82 issues, 63% bug-fix rate).

**Status:** Phases 1-9 complete. 15 test files, ~1,700 tests, all passing.

**Source of truth:** de1app codebase (`/Users/jeffreyh/Documents/GitHub/de1app` on Mac, `C:\code\de1app` on Windows) for all behavioral expectations. Tests must derive expected values independently from de1app Tcl source code (binary.tcl, profile.tcl, machine.tcl, plugin.tcl), NOT from reading Decenza's C++ source. This catches real divergences like the D-Flow fill exit formula bug found in Phase 1. Only behavior/protocol tests reference de1app — UI is Decenza's own design.

## Data-Driven Priority

| Area | Bug-Fix PRs | Fix Rate | Critical/High Bugs | Test Coverage |
|------|:-----------:|:--------:|:-------------------:|:--------------:|
| Machine State / SAW / SAV | 10 | 75-80% | 5 | tst_sav, tst_saw, tst_machinestate, tst_weightprocessor (116 tests) |
| Profile System | 5 | 83% | 2 | tst_profile, tst_profileframe, tst_recipeparams, tst_recipegenerator, tst_tclimport (340 tests) |
| BLE Protocol / Codec | 6 | 52% | 4 | tst_binarycodec, tst_blefidelity, tst_shotsettings (635 tests) |
| Settings | — | 45% | 0 | tst_settings (13 tests) |
| MCP Tools | 4 | 36% | 1 | **None — needs Phase 10** |
| Database / History | 2 | 64% | 0 | tst_dbmigration (17 tests) |
| Scale Protocols | 3 | — | 1 | tst_scaleprotocol (21 tests) |

## Phase Documents

| Phase | File | Tests | Status |
|-------|------|:-----:|:------:|
| 1 | [Phase 1: Pure Logic](2026-03-24-unit-testing-phase1.md) | 130 | Done |
| 2 | [Phase 2: Machine State + Frame Serialization](2026-03-24-unit-testing-phase2.md) | 89 | Done |
| 3 | [Phase 3: Settings & Recipe Validation](2026-03-24-unit-testing-phase3.md) | 44 | Done |
| 4 | [Phase 4: Integration Tests](2026-03-24-unit-testing-phase4.md) | 40 | Done |
| 5 | [Phase 5: TCL Import](2026-03-24-unit-testing-phase5.md) | 182 | Done |
| 6 | [Phase 6: BLE Fidelity](2026-03-24-unit-testing-phase6.md) | 561 | Done |
| 7 | [Phase 7: DB Migrations](2026-03-24-unit-testing-phase7.md) | 17 | Done |
| 8 | [Phase 8: WeightProcessor](2026-03-24-unit-testing-phase8.md) | 20 | Done |
| 9 | [Phase 9: Scale Protocols](2026-03-24-unit-testing-phase9.md) | 21 | Done |
| 10 | Phase 10: MCP Tools | — | **Needed** |

## Phase 10: MCP Tool Tests (Needed)

**Rationale:** PR #561 (defer BLE upload) introduced a regression where MCP tool callers of `uploadProfile()`/`uploadRecipeProfile()` silently failed to upload to the DE1. The bug was caught by code review, not tests. MCP tools are the only major subsystem with zero test coverage.

**Bug signals:**
- `uploadProfile()`/`uploadRecipeProfile()` behavioral change broke MCP callers silently (PR #561)
- MCP response messages can lie ("Profile updated and uploaded to machine" when upload was skipped)
- MCP tool schema caching bug across session reconnects (issue #560 area)
- 4 bug-fix PRs in MCP area at 36% fix rate

**Critical tests (would have caught the PR #561 regression):**
- [ ] `profiles_edit_params` on advanced profile: verify `DE1Device::uploadProfile()` is called after edit
- [ ] `profiles_edit_params` on recipe profile: verify `DE1Device::uploadProfile()` is called after recipe regeneration
- [ ] `settings_set` with espressoTemperature: verify BLE upload happens (not just in-memory update)
- [ ] `settings_set` with targetWeight: verify BLE upload happens
- [ ] Tool response accuracy: verify `result["message"]` matches actual side effects (e.g., if message says "uploaded to machine", assert BLE write occurred)

**Additional coverage:**
- [ ] `profiles_get_params`: verify returned params match current profile for all editor types
- [ ] `settings_get`: verify all returned fields have correct units and format
- [ ] Error paths: invalid params, missing required fields, out-of-range values
- [ ] MCP field naming conventions: units in field names (doseG, pressureBar, etc.)

**Infrastructure needed:**
- Mock or spy for `DE1Device::uploadProfile()` to verify BLE writes happen
- Test helper to call MCP tool handlers directly (they take `QJsonObject` args, return `QJsonObject`)
- May need `friend class` in `McpServer` or tool registration classes

## Test Infrastructure

**Framework:** Qt Test (QTest) with `-DBUILD_TESTS=ON`

**Patterns (established across Phases 1-9):**
- `QTEST_GUILESS_MAIN` — headless, no GUI
- `QSignalSpy` — signal verification
- Data-driven `_data()` methods — parametrized test cases (93 profiles in tst_blefidelity)
- `friend class` via `#ifdef DECENZA_TESTING` — private member access
- `TestFixture` structs — wired object graphs
- `MockScaleDevice` / `MockTransport` — mock via subclass
- Standalone executables per test — compile only needed sources
- `withRawDb()` / `withTempDb()` — scoped SQLite connections for DB tests

## Verification

After each test file:
1. Configure: `cmake -DBUILD_TESTS=ON`
2. Run: `ctest --output-on-failure`
3. All tests pass with 0 failures

After Phase 1 profile tests:
- Cross-check: `python scripts/compare_profiles.py` to verify built-in profiles match de1app TCL sources

## Bugs Found

- **PR #561 MCP regression** (caught by code review, not tests): removing `uploadCurrentProfile()` from `uploadProfile()`/`uploadRecipeProfile()` broke 4 MCP call sites that relied on the BLE upload happening inside those methods. Fixed by adding explicit `uploadCurrentProfile()` calls in MCP handlers. This is the primary motivation for Phase 10 MCP tests.
- **Decent Scale checksum not validated**: cross-referencing the [openscale firmware](https://github.com/decentespresso/openscale) revealed that neither Decenza nor de1app validate XOR checksums on incoming scale packets. Submitted [openscale PR #39](https://github.com/decentespresso/openscale/pull/39) to validate checksums on the scale's command receiver. Tracking in [issue #560](https://github.com/Kulitorum/Decenza/issues/560).

## Lessons Learned

- D-Flow and A-Flow are **submodules** in de1app, not in the main repo. Always fetch from upstream (github.com/Damian-AU/D_Flow_Espresso_Profile, github.com/Jan3kJ/A_Flow) instead of the local clone.
- D-Flow/A-Flow work by **editing existing frames**, not regenerating from scratch. The `update_D-Flow` proc modifies frames in-place. Decenza's RecipeGenerator generates fresh frames but uses the same formulas.
- **Test early-return anti-pattern**: loops that check one item and `return` silently skip the rest. Always add a `checked` counter with a minimum assertion (caught in code review of Phase 6 tests, same pattern as PR #559).
- **Removing behavior from shared methods breaks silent callers**: when `uploadProfile()` stopped doing BLE uploads, MCP callers broke silently. Any behavioral change to a shared method needs a grep of all callers.
- **ShotHistoryStorage background threads**: `requestDistinctCache()` launches a thread during `initialize()` that can outlive the object. Tests using `ShotHistoryStorage` need `QTEST_MAIN` (not GUILESS) and event loop processing after `close()`.

## Test Count Summary

| Test File | Tests | Phase |
|-----------|:-----:|:-----:|
| tst_binarycodec | 61 | 1 |
| tst_profile | 69 | 1 |
| tst_machinestate | 40 | 2 |
| tst_profileframe | 49 | 2 |
| tst_recipeparams | 31 | 3 |
| tst_settings | 13 | 3 |
| tst_recipegenerator | 27 | 4 |
| tst_shotsettings | 13 | 4 |
| tst_tclimport | 182 | 5 |
| tst_blefidelity | 561 | 6 |
| tst_dbmigration | 17 | 7 |
| tst_weightprocessor | 20 | 8 |
| tst_scaleprotocol | 21 | 9 |
| tst_saw | 14 | pre-existing |
| tst_sav | 42 | pre-existing |
| **Grand total** | **~1,160** | |
