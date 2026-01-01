#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>
#include <QTimer>
#include <QByteArray>

class AcaiaScale : public ScaleDevice {
    Q_OBJECT

public:
    explicit AcaiaScale(QObject* parent = nullptr);
    ~AcaiaScale() override;

    void connectToDevice(const QBluetoothDeviceInfo& device) override;
    QString name() const override { return m_name; }
    QString type() const override { return m_isPyxis ? "acaiapyxis" : "acaia"; }

public slots:
    void tare() override;
    void startTimer() override {}  // Acaia scales don't support remote timer control
    void stopTimer() override {}
    void resetTimer() override {}

private slots:
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid& uuid);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value);
    void onServiceDiscoveryFinished();
    void sendHeartbeat();
    void sendIdent();
    void sendConfig();
    void enableNotifications();

private:
    void parseResponse(const QByteArray& data);
    void decodeWeight(const QByteArray& payload, int payloadOffset);
    QByteArray encodePacket(uint8_t msgType, const QByteArray& payload);
    void sendCommand(const QByteArray& command);
    void setupServiceForProtocol();

    QString m_name = "Acaia";
    bool m_isPyxis = false;  // Auto-detected during service discovery
    bool m_protocolDetected = false;
    bool m_receivingNotifications = false;
    bool m_weightReceived = false;  // Track if we've received weight data
    QLowEnergyCharacteristic m_statusChar;
    QLowEnergyCharacteristic m_cmdChar;
    QTimer* m_heartbeatTimer = nullptr;

    // Services discovered - for auto-detection
    QLowEnergyService* m_ipsService = nullptr;
    QLowEnergyService* m_pyxisService = nullptr;

    // Message parsing state
    QByteArray m_buffer;
    static constexpr int ACAIA_METADATA_LEN = 5;
};
