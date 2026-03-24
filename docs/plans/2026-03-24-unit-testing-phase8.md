# Phase 8: WeightProcessor Edge Cases

> **For Claude:** Test LSLR flow estimation, de-jitter, oscillation recovery, and SAW gate edge cases. Uses processWeight() directly with controlled timing. Keep existing tst_saw (real-time) tests as integration.

**Rationale:** WeightProcessor had 80% fix rate. The LSLR flow estimation, de-jitter calibration, and oscillation recovery have subtle state-dependent bugs. Current tst_saw uses real-time waits (79s runtime); new tests call processWeight() rapidly with controlled weight series.

## Test File: `tests/tst_weightprocessor.cpp`

**Source deps:** `src/machine/weightprocessor.cpp` only (same as tst_saw).

**Approach:** Feed weight samples via processWeight() at controlled intervals using QTest::qWait() at shorter intervals (50ms) for faster execution. Focus on mathematical correctness of LSLR and gate logic rather than real-time behavior.

### Test Cases (~25)

**LSLR flow estimation:**
- [ ] Constant weight series → flow rate ≈ 0
- [ ] Linear rising weight (1g/s for 3s) → flow rate ≈ 1.0
- [ ] Step change in weight → flow rate reflects new slope after window fills
- [ ] Negative slope (dripping scale) → flow clamped to 0

**De-jitter (synthetic timestamp spreading):**
- [ ] Rapid-fire processWeight() calls (< 20ms apart) → flow still computed (synthetic timestamps spread)
- [ ] Mixed batch/gap pattern → flow rate stable across transitions

**SAW gate conditions:**
- [ ] SAW blocked during first 5s even with valid flow + weight
- [ ] SAW blocked before tare complete
- [ ] SAW blocked during preinfusion frames
- [ ] SAW fires immediately after all gates clear
- [ ] SAW fires only once (m_stopTriggered guard)

**Oscillation recovery (Bookoo tare):**
- [ ] Weight drops to -5g mid-shot → SAW blocked
- [ ] Weight stabilizes at ~0g for 3 samples → SAW re-enabled
- [ ] Weight oscillates between -3g and +2g → recovery doesn't complete

**Per-frame weight exit:**
- [ ] Frame with exitWeight=0.2g, weight=0.3g → skipFrame fires
- [ ] skipFrame fires only once per frame
- [ ] Frame with exitWeight=0, weight=99g → no skip (disabled)

**Untared cup detection:**
- [ ] Weight > 50g at t=0 → untaredCupDetected signal
- [ ] Weight > 50g at t=4s → no signal (past 3s window)
- [ ] Weight = 49g at t=0 → no signal (below threshold)

**Edge cases:**
- [ ] processWeight() before startExtraction() → no crash
- [ ] processWeight() after stopExtraction() → no signals
- [ ] configure() with empty frameExitWeights → no crash on frame check
