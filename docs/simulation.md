# DE1 Simulation System

The simulation system allows GUI development and testing without physical hardware. It's enabled automatically on **Windows debug builds**.

## Components

### DE1Simulator (`src/simulator/de1simulator.cpp`)

Simulates the DE1 espresso machine with realistic physics based on research from [Coffee ad Astra](https://coffeeadastra.com/2021/01/16/a-study-of-espresso-puck-resistance-and-how-puck-preparation-affects-it/) and [Barista Hustle](https://www.baristahustle.com/app-pressure-flow/).

#### Physics Model

**Puck Resistance** (Darcy's Law):
```
Flow = k × Pressure / Resistance
```

Resistance changes during extraction:
1. **Swelling phase** (0-5s): Resistance increases ~60% as coffee particles absorb water
2. **Peak phase**: Maximum resistance when puck is fully saturated
3. **Degradation phase**: Resistance declines 2-3.5× as oils are extracted

**System Dynamics**:
- Second-order response (damped spring) for pressure/flow changes
- Pressure inertia: ~0.4s to reach 63% of target (hoses expand, puck compresses)
- Maximum pressure: 12 bar (pump limit)
- Maximum flow: 8 ml/s (vibration pump limit)

**Thermal Model**:
- 75g steel group head provides thermal mass
- Heating: ~6°C/s with hot water flowing
- Cooling: ~0.3°C/s (thermal inertia limits this)

**Noise Generation** (Perlin Noise):
- Coherent noise for natural-looking variations
- Fractal noise (multiple octaves) for multi-frequency variation
- Random channeling events that cause temporary resistance drops

#### Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `BASELINE_RESISTANCE` | 3.5 | Fresh dry puck |
| `PEAK_RESISTANCE` | 5.5 | After coffee swells |
| `MIN_RESISTANCE` | 1.8 | Fully extracted puck |
| `PUCK_FILL_VOLUME` | 8.0 ml | Water to saturate puck before dripping |
| `SWELLING_TIME` | 5.0 s | Time for puck to fully swell |
| `DEGRADATION_RATE` | 0.004 | Resistance drop per ml water |

### SimulatedScale (`src/simulator/simulatedscale.cpp`)

Virtual scale that receives weight data from DE1Simulator. Implements the same `ScaleDevice` interface as physical scales.

Features:
- Tare support with offset tracking
- Flow rate calculation from weight changes
- Integrates with MachineState for stop-at-weight

### GHCSimulator (`src/simulator/ghcsimulator.cpp`)

Visual Group Head Controller with:
- 5 buttons: Espresso, Steam, Hot Water, Flush, Stop
- 12 RGB LEDs showing pressure (green) and flow (blue)
- Window position persistence

## Signal Flow

```
User clicks button in GHC window
    │
    ▼
GHCSimulator → DE1Device::startEspresso()
    │
    ▼
DE1Device::requestState() [simulation mode]
    │
    ▼
DE1Simulator::startEspresso()
    │
    ▼
DE1Simulator runs physics at 10Hz
    │
    ├──► stateChanged/subStateChanged
    │        └──► DE1Device::setSimulatedState()
    │                 └──► MachineState receives
    │
    ├──► shotSampleReceived (5Hz)
    │        └──► DE1Device::emitSimulatedShotSample()
    │                 └──► MainController → Graph
    │
    └──► scaleWeightChanged
             └──► SimulatedScale::setSimulatedWeight()
                      └──► MainController → Yield curve
```

## Enabling Simulation

Simulation is automatically enabled when:
```cpp
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
```

In `main.cpp`:
1. `DE1Device::setSimulationMode(true)` - makes device appear "connected"
2. `DE1Device::setSimulator(&de1Simulator)` - routes commands to simulator
3. SimulatedScale replaces FlowScale for weight data
4. GHCSimulator window opens alongside main app

## Yield Curve Physics

The yield (weight) curve follows an S-curve efficiency model:

```
efficiency = 0.4 + 0.52 × smoothstep(output_volume / 25ml)
```

- Starts at 40% efficiency (much water absorbed by puck)
- Rises to 92% as channels form and solubles extract
- Uses smoothstep (3x² - 2x³) for natural S-curve shape

## References

- [Coffee ad Astra - Puck Resistance Study](https://coffeeadastra.com/2021/01/16/a-study-of-espresso-puck-resistance-and-how-puck-preparation-affects-it/)
- [Espresso Aficionados - Pressure & Flow](https://espressoaf.com/info/flow_and_pressure.html)
- [itstorque - Espresso Simulations](https://www.itstorque.com/blog/2024_08_21_espresso_sims/)
- [Scott Rao - DE1 Pressure Profiling](https://www.scottrao.com/blog/2018/6/3/introduction-to-the-decent-espresso-machine)
