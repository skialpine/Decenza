# Phase 6: Built-in Profile BLE Fidelity

> **For Claude:** Load every built-in JSON profile, generate BLE bytes, verify header/frame/tail format. Decode BLE bytes back and compare against original values. Data-driven with one row per profile.

**Rationale:** The TargetEspressoVol=36 bug was in the BLE encoding layer. Testing encode→decode round-trip for all 93 profiles catches silent corruption that affects machine behavior.

## Test File: `tests/tst_blefidelity.cpp`

**Source deps:** PROFILE_SOURCES + CODEC_SOURCES. Needs Qt resources (`resources/profiles/`) or filesystem access.

### Test Cases (~25)

**Per-profile BLE validation (data-driven, one row per profile):**
- [ ] `toHeaderBytes()` returns exactly 5 bytes
- [ ] Header byte[0] = 1 (HeaderV)
- [ ] Header byte[1] = steps.size() (NumberOfFrames)
- [ ] Header byte[2] = preinfuseFrameCount (NumberOfPreinfuseFrames)
- [ ] Header byte[3] = 0 (MinimumPressure, de1app default)
- [ ] Header byte[4] = encodeU8P4(6.0) = 96 (MaximumFlow, de1app default)

**Per-frame BLE validation:**
- [ ] Each frame is exactly 8 bytes
- [ ] Frame[0] = frame index (0, 1, 2, ...)
- [ ] Frame flags match computeFlags() for the frame's properties
- [ ] SetVal (U8P4) decodes back within 1/16 of original getSetVal()
- [ ] Temp (U8P1) decodes back within 0.5 of original temperature
- [ ] FrameLen (F8_1_7) decodes back within tolerance of original seconds
- [ ] Extension frames present when maxFlowOrPressure > 0

**Tail frame:**
- [ ] Last frame has FrameToWrite = steps.size()
- [ ] Tail is exactly 8 bytes

**Specific encode→decode checks:**
- [ ] D-Flow default: 3 frames + extensions + tail
- [ ] Profile with limiter: extension frame at index+32
- [ ] Profile with volume > 0: U10P0 encodes/decodes correctly
