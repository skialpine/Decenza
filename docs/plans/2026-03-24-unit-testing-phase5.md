# Phase 5: TCL Profile Import Round-Trip

> **For Claude:** Load every de1app TCL profile through Decenza's parser, export to JSON, re-import, compare. Data-driven test with one row per profile. Use de1app at `/Users/jeffreyh/Documents/GitHub/de1app/de1plus/profiles/` as oracle.

**Rationale:** Profile import had 83% fix rate. TCL parsing uses regex which can fail on edge cases (braced values with newlines, empty fields, unusual Tcl syntax). Loading real de1app profiles catches format mismatches that hand-crafted test strings miss.

## Test File: `tests/tst_tclimport.cpp`

**Source deps:** PROFILE_SOURCES + CODEC_SOURCES

**Approach:**
- Read TCL files from disk at test time (use absolute path to de1app profiles dir)
- For each profile: `loadFromTclString()` → verify non-empty title and steps → `toJson()` → `fromJson()` → compare key fields
- Data-driven: enumerate profile files dynamically or list known profiles

### Test Cases (~30)

**Per-profile round-trip (data-driven, ~25 rows):**
- [ ] Load TCL → verify title is non-empty
- [ ] Verify profileType is one of settings_2a, settings_2b, settings_2c, settings_2c2
- [ ] Verify steps count > 0 (no empty profiles)
- [ ] Verify preinfuseFrameCount >= 0
- [ ] Export to JSON → re-import → title matches
- [ ] Export to JSON → re-import → steps count matches
- [ ] Export to JSON → re-import → target_weight matches
- [ ] Export to JSON → re-import → profileType matches

**Specific profile oracle tests:**
- [ ] `default.tcl`: target_weight, espresso_temperature match de1app defaults
- [ ] `Blooming espresso.tcl`: advanced profile with multiple exit conditions
- [ ] Tea profiles: beverage_type = "tea" or similar non-espresso
- [ ] Simple pressure profile: settings_2a frame generation matches de1app

**Edge cases:**
- [ ] Profile with empty advanced_shot block → falls back to simple frame gen
- [ ] Profile with braced multi-word title
- [ ] Profile with temperature stepping enabled
