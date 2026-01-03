#include "variaakuscale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>

// Logging macros for easy filtering
#define VARIA_LOG qDebug() << "[Varia]:"
#define VARIA_WARN qWarning() << "[Varia]:"

VariaAkuScale::VariaAkuScale(QObject* parent)
    : ScaleDevice(parent)
{
    VARIA_LOG << "Creating VariaAkuScale instance";

    // Create watchdog timer (fires if no updates received after enable)
    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setSingleShot(true);
    connect(m_watchdogTimer, &QTimer::timeout, this, &VariaAkuScale::onWatchdogTimeout);

    // Create tickle timer (fires if updates stop arriving)
    m_tickleTimer = new QTimer(this);
    m_tickleTimer->setSingleShot(true);
    connect(m_tickleTimer, &QTimer::timeout, this, &VariaAkuScale::onTickleTimeout);

    VARIA_LOG << "Timers created - watchdog:" << WATCHDOG_TIMEOUT_MS << "ms, tickle:" << TICKLE_TIMEOUT_MS << "ms";
}

VariaAkuScale::~VariaAkuScale() {
    VARIA_LOG << "Destroying VariaAkuScale instance";
    stopWatchdog();
    disconnectFromScale();
}

void VariaAkuScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    VARIA_LOG << "connectToDevice() called for:" << device.name();
    VARIA_LOG << "  Address:" << device.address().toString();
    VARIA_LOG << "  RSSI:" << device.rssi();

    if (m_controller) {
        VARIA_LOG << "  Existing controller found, disconnecting first";
        disconnectFromScale();
    }

    m_name = device.name();
    m_controller = QLowEnergyController::createCentral(device, this);
    VARIA_LOG << "  Created BLE controller";

    connect(m_controller, &QLowEnergyController::connected,
            this, &VariaAkuScale::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &VariaAkuScale::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &VariaAkuScale::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &VariaAkuScale::onServiceDiscovered);

    VARIA_LOG << "  Initiating BLE connection...";
    m_controller->connectToDevice();
}

void VariaAkuScale::onControllerConnected() {
    VARIA_LOG << "onControllerConnected() - BLE connection established";
    VARIA_LOG << "  Starting service discovery...";
    m_controller->discoverServices();
}

void VariaAkuScale::onControllerDisconnected() {
    VARIA_WARN << "onControllerDisconnected() - BLE connection lost!";
    stopWatchdog();
    setConnected(false);
}

void VariaAkuScale::onControllerError(QLowEnergyController::Error error) {
    VARIA_WARN << "onControllerError() - Error code:" << error;
    switch (error) {
        case QLowEnergyController::NoError:
            VARIA_WARN << "  NoError (unexpected)";
            break;
        case QLowEnergyController::UnknownError:
            VARIA_WARN << "  UnknownError";
            break;
        case QLowEnergyController::UnknownRemoteDeviceError:
            VARIA_WARN << "  UnknownRemoteDeviceError - device not found";
            break;
        case QLowEnergyController::NetworkError:
            VARIA_WARN << "  NetworkError - BLE adapter issue";
            break;
        case QLowEnergyController::InvalidBluetoothAdapterError:
            VARIA_WARN << "  InvalidBluetoothAdapterError - no BLE adapter";
            break;
        case QLowEnergyController::ConnectionError:
            VARIA_WARN << "  ConnectionError - connection failed or dropped";
            break;
        case QLowEnergyController::AdvertisingError:
            VARIA_WARN << "  AdvertisingError";
            break;
        case QLowEnergyController::RemoteHostClosedError:
            VARIA_WARN << "  RemoteHostClosedError - scale closed connection";
            break;
        case QLowEnergyController::AuthorizationError:
            VARIA_WARN << "  AuthorizationError - pairing required?";
            break;
        case QLowEnergyController::MissingPermissionsError:
            VARIA_WARN << "  MissingPermissionsError - check app permissions";
            break;
        case QLowEnergyController::RssiReadError:
            VARIA_WARN << "  RssiReadError";
            break;
        default:
            VARIA_WARN << "  Unknown error code:" << static_cast<int>(error);
            break;
    }
    emit errorOccurred("Varia Aku scale connection error");
    setConnected(false);
}

void VariaAkuScale::onServiceDiscovered(const QBluetoothUuid& uuid) {
    VARIA_LOG << "onServiceDiscovered() - UUID:" << uuid.toString();

    if (uuid == Scale::VariaAku::SERVICE) {
        VARIA_LOG << "  Found Varia service (FFF0)!";
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            VARIA_LOG << "  Service object created, connecting signals...";
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &VariaAkuScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &VariaAkuScale::onCharacteristicChanged);
            connect(m_service, &QLowEnergyService::errorOccurred,
                    this, &VariaAkuScale::onServiceError);
            VARIA_LOG << "  Starting characteristic discovery...";
            m_service->discoverDetails();
        } else {
            VARIA_WARN << "  Failed to create service object!";
        }
    } else {
        VARIA_LOG << "  (not our service, ignoring)";
    }
}

void VariaAkuScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    VARIA_LOG << "onServiceStateChanged() - State:" << state;

    switch (state) {
        case QLowEnergyService::InvalidService:
            VARIA_WARN << "  InvalidService state";
            break;
        case QLowEnergyService::RemoteService:
            VARIA_LOG << "  RemoteService - service discovered but details not yet loaded";
            break;
        case QLowEnergyService::RemoteServiceDiscovering:
            VARIA_LOG << "  RemoteServiceDiscovering - loading characteristics...";
            break;
        case QLowEnergyService::RemoteServiceDiscovered:
            VARIA_LOG << "  RemoteServiceDiscovered - characteristics loaded!";
            break;
        case QLowEnergyService::LocalService:
            VARIA_LOG << "  LocalService (unexpected for peripheral)";
            break;
    }

    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_statusChar = m_service->characteristic(Scale::VariaAku::STATUS);
        m_cmdChar = m_service->characteristic(Scale::VariaAku::CMD);

        VARIA_LOG << "  STATUS characteristic (FFF1) valid:" << m_statusChar.isValid();
        VARIA_LOG << "  CMD characteristic (FFF2) valid:" << m_cmdChar.isValid();

        if (m_statusChar.isValid()) {
            VARIA_LOG << "  STATUS properties:" << m_statusChar.properties();
        }
        if (m_cmdChar.isValid()) {
            VARIA_LOG << "  CMD properties:" << m_cmdChar.properties();
        }

        // Delay notification enable by 200ms (matching de1app pattern)
        // The Varia Aku scale needs time to stabilize after service discovery
        VARIA_LOG << "  Scheduling notification enable in 200ms...";
        QTimer::singleShot(200, this, [this]() {
            // Check if still connected (scale may have disconnected during delay)
            if (!m_service || !m_controller) {
                VARIA_WARN << "  Service/controller gone before notification enable!";
                return;
            }

            VARIA_LOG << "  200ms delay complete, enabling notifications...";

            // Enable notifications and start watchdog
            enableNotifications();
            startWatchdog();

            setConnected(true);
            VARIA_LOG << "  Scale marked as connected";
        });
    }
}

void VariaAkuScale::enableNotifications() {
    if (!m_service || !m_statusChar.isValid()) {
        VARIA_WARN << "enableNotifications() - service or characteristic invalid!";
        VARIA_WARN << "  m_service:" << (m_service ? "valid" : "null");
        VARIA_WARN << "  m_statusChar.isValid():" << m_statusChar.isValid();
        return;
    }

    VARIA_LOG << "enableNotifications() - writing CCCD descriptor...";

    QLowEnergyDescriptor notification = m_statusChar.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    if (notification.isValid()) {
        VARIA_LOG << "  CCCD descriptor found, writing 0x0100 (enable notifications)";
        m_service->writeDescriptor(notification, QByteArray::fromHex("0100"));
    } else {
        VARIA_WARN << "  CCCD descriptor NOT found! Cannot enable notifications.";
    }
}

void VariaAkuScale::startWatchdog() {
    m_watchdogRetries = 0;
    m_updatesReceived = false;
    m_watchdogTimer->start(WATCHDOG_TIMEOUT_MS);
    VARIA_LOG << "startWatchdog() - waiting for first weight update (timeout:" << WATCHDOG_TIMEOUT_MS << "ms)";
}

void VariaAkuScale::tickleWatchdog() {
    // First update received - watchdog succeeded
    if (!m_updatesReceived) {
        m_updatesReceived = true;
        m_watchdogTimer->stop();
        VARIA_LOG << "tickleWatchdog() - FIRST weight update received! Watchdog stopped.";
    }

    // Reset the tickle timer - if no updates for TICKLE_TIMEOUT_MS, log warning
    m_tickleTimer->start(TICKLE_TIMEOUT_MS);
}

void VariaAkuScale::stopWatchdog() {
    VARIA_LOG << "stopWatchdog() - stopping all timers";
    m_watchdogTimer->stop();
    m_tickleTimer->stop();
    m_updatesReceived = false;
    m_watchdogRetries = 0;
}

void VariaAkuScale::onWatchdogTimeout() {
    if (m_updatesReceived) {
        VARIA_LOG << "onWatchdogTimeout() - updates already received, ignoring";
        return;  // Updates started arriving, no need to retry
    }

    m_watchdogRetries++;

    if (m_watchdogRetries >= MAX_WATCHDOG_RETRIES) {
        VARIA_WARN << "onWatchdogTimeout() - NO weight updates after" << MAX_WATCHDOG_RETRIES << "retries, GIVING UP";
        emit errorOccurred("Varia Aku scale not sending weight updates");
        return;
    }

    VARIA_WARN << "onWatchdogTimeout() - no weight updates, retry" << m_watchdogRetries << "of" << MAX_WATCHDOG_RETRIES;

    // Re-enable notifications
    enableNotifications();

    // Schedule next retry
    m_watchdogTimer->start(WATCHDOG_TIMEOUT_MS);
}

void VariaAkuScale::onTickleTimeout() {
    VARIA_WARN << "onTickleTimeout() - no weight updates for" << TICKLE_TIMEOUT_MS << "ms!";
    VARIA_WARN << "  Scale may have stopped sending data. Re-enabling notifications...";

    // Try re-enabling notifications
    m_updatesReceived = false;
    m_watchdogRetries = 0;
    enableNotifications();
    m_watchdogTimer->start(WATCHDOG_TIMEOUT_MS);
}

void VariaAkuScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::VariaAku::STATUS) {
        // Varia Aku format: header command length payload xor
        // Weight notification: command 0x01, length 0x03, payload w1 w2 w3 xor
        if (value.size() >= 4) {
            const uint8_t* d = reinterpret_cast<const uint8_t*>(value.constData());

            uint8_t command = d[1];
            uint8_t length = d[2];

            // Weight notification
            if (command == 0x01 && length == 0x03 && value.size() >= 7) {
                // Tickle watchdog on every weight update
                tickleWatchdog();

                uint8_t w1 = d[3];
                uint8_t w2 = d[4];
                uint8_t w3 = d[5];

                // Sign is in highest nibble of w1 (0x10 means negative)
                bool isNegative = (w1 & 0x10) != 0;

                // Weight is 3 bytes big-endian in hundredths of gram
                // Strip sign nibble from w1
                uint32_t weightRaw = ((w1 & 0x0F) << 16) | (w2 << 8) | w3;
                double weight = weightRaw / 100.0;

                if (isNegative) {
                    weight = -weight;
                }

                // Log every 50th weight update to avoid spam (roughly every 10 seconds at 5Hz)
                static int weightLogCounter = 0;
                if (++weightLogCounter >= 50) {
                    VARIA_LOG << "Weight update:" << weight << "g (raw:" << value.toHex(' ') << ")";
                    weightLogCounter = 0;
                }

                setWeight(weight);
            }
            // Battery notification
            else if (command == 0x85 && length == 0x01 && value.size() >= 5) {
                uint8_t battery = d[3];
                VARIA_LOG << "Battery update:" << battery << "%";
                setBatteryLevel(battery);
            }
            // Unknown command
            else {
                VARIA_LOG << "Unknown notification - cmd:" << Qt::hex << command
                          << "len:" << length << "data:" << value.toHex(' ');
            }
        } else {
            VARIA_WARN << "Notification too short:" << value.size() << "bytes, data:" << value.toHex(' ');
        }
    } else {
        VARIA_LOG << "Notification from unexpected characteristic:" << c.uuid().toString();
    }
}

void VariaAkuScale::onServiceError(QLowEnergyService::ServiceError error) {
    VARIA_WARN << "onServiceError() - Error code:" << error;
    switch (error) {
        case QLowEnergyService::NoError:
            VARIA_WARN << "  NoError (unexpected)";
            break;
        case QLowEnergyService::OperationError:
            VARIA_WARN << "  OperationError - operation attempted in wrong state";
            break;
        case QLowEnergyService::CharacteristicWriteError:
            VARIA_WARN << "  CharacteristicWriteError - write to characteristic failed";
            break;
        case QLowEnergyService::DescriptorWriteError:
            VARIA_WARN << "  DescriptorWriteError - write to descriptor failed";
            break;
        case QLowEnergyService::UnknownError:
            VARIA_WARN << "  UnknownError";
            break;
        case QLowEnergyService::CharacteristicReadError:
            VARIA_WARN << "  CharacteristicReadError";
            break;
        case QLowEnergyService::DescriptorReadError:
            VARIA_WARN << "  DescriptorReadError";
            break;
        default:
            VARIA_WARN << "  Unknown error code:" << static_cast<int>(error);
            break;
    }

    if (error == QLowEnergyService::CharacteristicWriteError ||
        error == QLowEnergyService::DescriptorWriteError) {
        VARIA_LOG << "  Will retry enabling notifications in 500ms...";
        // Try re-enabling notifications on write errors
        QTimer::singleShot(500, this, [this]() {
            if (m_service) {
                VARIA_LOG << "  Retrying notification enable after error...";
                enableNotifications();
            }
        });
    }
}

void VariaAkuScale::sendCommand(const QByteArray& cmd) {
    if (!m_service || !m_cmdChar.isValid()) {
        VARIA_WARN << "sendCommand() - cannot send, service or characteristic invalid";
        return;
    }
    VARIA_LOG << "sendCommand() - sending:" << cmd.toHex(' ');
    // Use WriteWithoutResponse like Beanconqueror does
    m_service->writeCharacteristic(m_cmdChar, cmd, QLowEnergyService::WriteWithoutResponse);
}

void VariaAkuScale::tare() {
    VARIA_LOG << "tare() - sending tare command";
    sendCommand(QByteArray::fromHex("FA82010182"));
}
