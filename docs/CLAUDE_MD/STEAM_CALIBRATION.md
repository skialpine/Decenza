# Steam Calibration

## Overview

The DE1 uses a flash heater (not a boiler) to generate steam on demand. Different models have different heater wattages (Pro/XL: 1.5kW, XXL: 2.2kW, Bengle: 3kW), and voltage (110V vs 220V) further affects actual heating power. Finding optimal steam settings is model- and setup-dependent. The Steam Calibration Tool automates the manual testing process that community members have been doing for years.

## The Physics

### Flash Heater Sweet Spot

The optimal steam flow rate is the **highest rate where the heater can fully vaporize all water passing through it**. This is the sweet spot because:

- **Too low**: The heater overheats and triggers a protective flow increase, creating an oscillating/sawtooth pressure curve. The steam is dry but the vortex is weak.
- **Too high**: More water passes through than the heater can vaporize, producing wet steam (more water added to milk). The vortex is strong but microfoam quality suffers.
- **Sweet spot**: Stable "solid rock" pressure curve, maximum steam dryness, and enough kinetic energy for a strong vortex.

### Approximate Sweet Spots by Model

| Model | Heater | Approx. Sweet Spot |
|-------|--------|-------------------|
| DE1 Pro/XL | 1.5kW | ~0.6-0.8 mL/s |
| DE1 XXL | 2.2kW | ~0.9-1.2 mL/s |
| Bengle | 3.0kW | ~1.2-1.5 mL/s |

These vary by voltage (110V shifts the sweet spot down) and individual machine calibration.

### Dilution Math

To raise 180g of milk by 60 degrees C (e.g., 3 to 63 degrees C), the theoretical minimum water addition from steam condensation is ~11.3% (accounting for pitcher thermal mass). In practice, expect 12-15% due to surface evaporation during aeration, radiative/convective heat losses, and sub-optimal vaporization. The 1-2% difference between machines is negligible in the cup (milk is already ~88% water), but steam speed and kinetic energy noticeably affect microfoam texture and latte art quality.

### Flow Calibration Affects Steam

The Graphical Flow Calibrator (GFC) multiplier affects the machine's internal flow measurement during steaming, not just espresso. A miscalibrated flow multiplier is a common hidden cause of bad steam performance. One community member spent over a year troubleshooting steam issues before discovering his machine's previous owner had set the multiplier to 1.41 (should have been ~0.7).

## Stability Metrics

The calibration tool analyzes the pressure curve from each steam session (after a 2-second initial trim) to compute:

- **Pressure CV** (coefficient of variation = stddev/mean): The primary stability metric. Stable < 0.05, oscillating > 0.15.
- **Oscillation rate**: Zero-crossings of the detrended pressure signal per second. Stable ~0.5-1 Hz, protective-cycling sawtooth shows 2-4 Hz.
- **Peak-to-peak range**: Max minus min pressure. Stable < 0.3 bar, oscillating > 1.0 bar.
- **Pressure slope**: Linear regression slope over time. Near-zero is ideal; positive slope suggests the heater is progressively struggling.

These are combined into a 0-100 stability score for UX simplicity.

## Calibration Workflow

1. User opens the Steam Calibration dialog from the Steam Health card in Settings > Calibration.
2. The tool determines the sweep range based on the connected machine model (and heater voltage if available).
3. For each flow rate step (typically 6 steps):
   - The app sets the steam flow rate
   - The user fills a pitcher with water and physically starts steam
   - The app records pressure/flow/temperature data for at least 20 seconds
   - The user stops steam
   - The app computes stability metrics and shows results for that step
4. After all steps, the app recommends the optimal flow rate and offers to apply it.

## Recommendation Algorithm

1. Filter out any step with stability score below 60 (clearly unstable).
2. Among remaining steps, select the highest flow rate with score >= 75.
3. If no step exceeds 75, recommend the highest-scoring step and suggest checking flow calibration.
4. Secondary warnings: average pressure > 4.0 bar (possible scale buildup), best score at sweep boundary (suggest extending range).

## Drift Detection

After calibration, normal steaming sessions automatically compute a stability score and compare it against the calibration baseline. If the score drops by more than 30% on the same flow setting, the app shows a banner suggesting re-calibration. This catches flow calibration drift, scale buildup affecting steam, or voltage changes.

## Community Research Sources

The design of this feature is informed by extensive community testing and discussion:

- **Eduardo Passoa's enthalpy vs. kinetic energy analysis** (Basecamp, Decent Diaspora, April 2026): Systematic comparison of DE1 XXL vs. commercial Fiamma boiler. Discovered 0.9 mL/s sweet spot for XXL by analyzing pressure curve stability. 54-comment thread with input from Decent staff.
- **Michael Garcia's steam performance testing** (Basecamp, July 2020): Systematic sweep of flow/temp combinations on 120V Pro, documented with pressure curve PDFs. Settled on 165 degrees C @ 0.6 mL/s for smoothest pressure.
- **Sergey Shevtchenko's flow calibration discovery** (Basecamp, July 2024 - September 2025): Diagnosed that a wrong GFC multiplier (1.41 instead of ~0.7) caused bad steam on his XXL. The GFC setting affects steam, not just espresso.
- **Collin Arneson's engineering analysis**: Explained that heater efficiency improves at higher pressures/flow rates, with the sweet spot where dilution starts to drop before increasing sharply at excessive flow.
- **Shinguk Kwon's Bengle confirmation**: Bengle's 3kW heater produces faster/stronger steam with a notably stronger whirlpool effect.
- **Damian's thermodynamic calculations**: Provided the physics showing ~11.3% dilution as the theoretical minimum for standard steaming parameters.
