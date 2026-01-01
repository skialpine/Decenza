#include "ghcsimulator.h"
#include "de1simulator.h"
#include "../ble/de1device.h"
#include <QDebug>

GHCSimulator::GHCSimulator(QObject* parent)
    : QObject(parent)
{
    // Initialize all LEDs to off (dark gray)
    setAllLeds(QColor(30, 30, 30));
}

void GHCSimulator::setDE1Device(DE1Device* device)
{
    if (m_device) {
        disconnect(m_device, nullptr, this, nullptr);
    }

    m_device = device;

    if (m_device) {
        connect(m_device, &DE1Device::shotSampleReceived, this,
                [this](const ShotSample& sample) {
            onShotSample(sample.groupPressure, sample.groupFlow);
        });
        connect(m_device, &DE1Device::stateChanged, this, &GHCSimulator::onStateChanged);
    }
}

void GHCSimulator::setDE1Simulator(DE1Simulator* simulator)
{
    if (m_simulator) {
        disconnect(m_simulator, nullptr, this, nullptr);
    }

    m_simulator = simulator;

    if (m_simulator) {
        connect(m_simulator, &DE1Simulator::shotSampleReceived, this,
                [this](const ShotSample& sample) {
            onShotSample(sample.groupPressure, sample.groupFlow);
        });
        connect(m_simulator, &DE1Simulator::stateChanged, this, [this]() {
            onSimulatorStateChanged();
        });
    }
}

void GHCSimulator::onSimulatorStateChanged()
{
    if (!m_simulator || m_stopPressed) {
        return;
    }

    DE1::State state = m_simulator->state();
    ActiveFunction newFunction = ActiveFunction::None;

    // Reset all LEDs to off
    setAllLeds(QColor(30, 30, 30));

    switch (state) {
    case DE1::State::Espresso:
        newFunction = ActiveFunction::Espresso;
        // Espresso uses special pressure/flow display - handled in onShotSample
        break;

    case DE1::State::Steam:
        newFunction = ActiveFunction::Steam;
        setLedRange(2, 3, QColor(100, 150, 255));
        emit ledColorsChanged();
        break;

    case DE1::State::HotWater:
        newFunction = ActiveFunction::HotWater;
        m_leds[11] = QColor(255, 200, 100);
        m_leds[0] = QColor(255, 200, 100);
        m_leds[1] = QColor(255, 200, 100);
        emit ledColorsChanged();
        break;

    case DE1::State::HotWaterRinse:
        newFunction = ActiveFunction::Flush;
        setLedRange(8, 3, QColor(100, 200, 255));
        emit ledColorsChanged();
        break;

    default:
        break;
    }

    if (newFunction != m_activeFunction) {
        m_activeFunction = newFunction;
        emit activeFunctionChanged();
    }
}

QVariantList GHCSimulator::ledColors() const
{
    QVariantList colors;
    for (int i = 0; i < LED_COUNT; ++i) {
        colors.append(m_leds[i]);
    }
    return colors;
}

void GHCSimulator::setAllLeds(const QColor& color)
{
    for (int i = 0; i < LED_COUNT; ++i) {
        m_leds[i] = color;
    }
    emit ledColorsChanged();
}

void GHCSimulator::setLedRange(int start, int count, const QColor& color)
{
    for (int i = 0; i < count; ++i) {
        int index = (start + i) % LED_COUNT;
        m_leds[index] = color;
    }
}

void GHCSimulator::pressEspresso()
{
    if (m_simulator) {
        m_simulator->startEspresso();
    } else if (m_device) {
        m_device->startEspresso();
    }
}

void GHCSimulator::pressSteam()
{
    if (m_simulator) {
        m_simulator->startSteam();
    } else if (m_device) {
        m_device->startSteam();
    }
}

void GHCSimulator::pressHotWater()
{
    if (m_simulator) {
        m_simulator->startHotWater();
    } else if (m_device) {
        m_device->startHotWater();
    }
}

void GHCSimulator::pressFlush()
{
    if (m_simulator) {
        m_simulator->startFlush();
    } else if (m_device) {
        m_device->startFlush();
    }
}

void GHCSimulator::pressStop()
{
    m_stopPressed = true;
    emit stopPressedChanged();

    // Show red LEDs when stop is pressed
    setAllLeds(QColor(255, 50, 50));

    if (m_simulator) {
        m_simulator->stop();
    } else if (m_device) {
        m_device->stopOperation();
    }
}

void GHCSimulator::releaseStop()
{
    m_stopPressed = false;
    emit stopPressedChanged();

    // Return to normal state
    if (m_simulator) {
        onSimulatorStateChanged();
    } else {
        onStateChanged();
    }
}

void GHCSimulator::onStateChanged()
{
    if (!m_device || m_stopPressed) {
        return;
    }

    DE1::State state = m_device->state();
    ActiveFunction newFunction = ActiveFunction::None;

    // Reset all LEDs to off
    setAllLeds(QColor(30, 30, 30));

    switch (state) {
    case DE1::State::Espresso:
        newFunction = ActiveFunction::Espresso;
        // Espresso uses special pressure/flow display - handled in onShotSample
        break;

    case DE1::State::Steam:
        newFunction = ActiveFunction::Steam;
        // Steam: light up LEDs 2, 3, 4 (right side, near steam button)
        setLedRange(2, 3, QColor(100, 150, 255));  // Light blue for steam
        emit ledColorsChanged();
        break;

    case DE1::State::HotWater:
        newFunction = ActiveFunction::HotWater;
        // Hot water: light up LEDs 11, 0, 1 (top, near hot water button)
        m_leds[11] = QColor(255, 200, 100);  // Warm orange
        m_leds[0] = QColor(255, 200, 100);
        m_leds[1] = QColor(255, 200, 100);
        emit ledColorsChanged();
        break;

    case DE1::State::HotWaterRinse:  // Flush
        newFunction = ActiveFunction::Flush;
        // Flush: light up LEDs 8, 9, 10 (left side, near flush button)
        setLedRange(8, 3, QColor(100, 200, 255));  // Cyan for flush
        emit ledColorsChanged();
        break;

    default:
        // Idle or other states - LEDs stay off
        break;
    }

    if (newFunction != m_activeFunction) {
        m_activeFunction = newFunction;
        emit activeFunctionChanged();
    }
}

void GHCSimulator::onShotSample(double pressure, double flow)
{
    if (m_stopPressed || m_activeFunction != ActiveFunction::Espresso) {
        return;
    }

    updateEspressoLeds(pressure, flow);
}

void GHCSimulator::updateEspressoLeds(double pressure, double flow)
{
    // Reset all LEDs to off
    for (int i = 0; i < LED_COUNT; ++i) {
        m_leds[i] = QColor(30, 30, 30);
    }

    // Calculate how many LEDs to light for pressure (green) and flow (blue)
    // LEDs light up clockwise from 12:00 (LED 0)
    int pressureLeds = qBound(0, static_cast<int>((pressure / MAX_PRESSURE) * LED_COUNT), LED_COUNT);
    int flowLeds = qBound(0, static_cast<int>((flow / MAX_FLOW) * LED_COUNT), LED_COUNT);

    // Light up LEDs for pressure (green) - clockwise from top
    for (int i = 0; i < pressureLeds; ++i) {
        // Green component based on pressure
        m_leds[i].setGreen(qMin(255, m_leds[i].green() + 200));
    }

    // Light up LEDs for flow (blue) - also clockwise from top
    // This creates a blended color where both pressure and flow are present
    for (int i = 0; i < flowLeds; ++i) {
        // Blue component based on flow
        m_leds[i].setBlue(qMin(255, m_leds[i].blue() + 200));
    }

    // Ensure minimum brightness for lit LEDs
    for (int i = 0; i < qMax(pressureLeds, flowLeds); ++i) {
        if (m_leds[i].red() < 30) m_leds[i].setRed(30);
    }

    emit ledColorsChanged();
}
