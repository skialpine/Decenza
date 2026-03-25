#include <QtTest>
#include <QSignalSpy>

#include "ble/refractometers/difluidr2.h"
#include "ble/protocol/de1characteristics.h"

// Test DiFluid R2 refractometer BLE packet parsing, checksum validation,
// and device name matching.
//
// Protocol: header 0xDF 0xDF, function byte, data, XOR checksum.
// Package 0 = status, Package 1 = temperature, Package 2 = TDS.

class tst_DiFluidR2 : public QObject {
    Q_OBJECT

private:
    // Build an R2 packet with valid XOR checksum
    static QByteArray buildR2Packet(uint8_t func, const QByteArray& data) {
        QByteArray pkt;
        pkt.append(static_cast<char>(0xDF));  // Header
        pkt.append(static_cast<char>(0xDF));  // Header
        pkt.append(static_cast<char>(func));  // Function byte

        pkt.append(data);

        // XOR checksum of bytes 2..N
        uint8_t checksum = 0;
        for (qsizetype i = 2; i < pkt.size(); ++i) {
            checksum ^= static_cast<uint8_t>(pkt[i]);
        }
        pkt.append(static_cast<char>(checksum));
        return pkt;
    }

    // Build a TDS packet (package type 0x02)
    static QByteArray buildTdsPacket(double tds) {
        uint16_t raw = static_cast<uint16_t>(qRound(tds * 100.0));
        QByteArray data;
        data.append(static_cast<char>((raw >> 8) & 0xFF));
        data.append(static_cast<char>(raw & 0xFF));
        return buildR2Packet(0x02, data);
    }

    // Build a temperature packet (package type 0x01)
    static QByteArray buildTemperaturePacket(double tempC) {
        uint16_t raw = static_cast<uint16_t>(qRound(tempC * 100.0));
        QByteArray data;
        data.append(static_cast<char>((raw >> 8) & 0xFF));
        data.append(static_cast<char>(raw & 0xFF));
        return buildR2Packet(0x01, data);
    }

    // Build a status packet (package type 0x00)
    static QByteArray buildStatusPacket(uint8_t status) {
        QByteArray data;
        data.append(static_cast<char>(status));
        return buildR2Packet(0x00, data);
    }

private slots:

    // === Device name matching ===

    void isR2DeviceMatchesR2Extract() {
        QVERIFY(DiFluidR2::isR2Device("R2 Extract"));
        QVERIFY(DiFluidR2::isR2Device("r2 extract"));
        QVERIFY(DiFluidR2::isR2Device("R2Extract"));
    }

    void isR2DeviceMatchesDiFluidR2() {
        QVERIFY(DiFluidR2::isR2Device("DiFluid R2"));
        QVERIFY(DiFluidR2::isR2Device("difluid r2"));
        QVERIFY(DiFluidR2::isR2Device("DIFLUID R2 EXTRACT"));
    }

    void isR2DeviceRejectsMicrobalance() {
        // DiFluid Microbalance is a scale, not an R2
        QVERIFY(!DiFluidR2::isR2Device("DiFluid"));
        QVERIFY(!DiFluidR2::isR2Device("difluid"));
        QVERIFY(!DiFluidR2::isR2Device("Microbalance"));
        QVERIFY(!DiFluidR2::isR2Device("DiFluid Microbalance"));
    }

    void isR2DeviceRejectsOtherDevices() {
        QVERIFY(!DiFluidR2::isR2Device("Acaia Lunar"));
        QVERIFY(!DiFluidR2::isR2Device("Decent Scale"));
        QVERIFY(!DiFluidR2::isR2Device(""));
    }

    // === Checksum validation ===

    void checksumValidForCorrectPacket() {
        DiFluidR2 r2(nullptr);
        QByteArray pkt = buildTdsPacket(8.50);
        QVERIFY(r2.validateChecksum(pkt));
    }

    void checksumInvalidForCorruptedPacket() {
        DiFluidR2 r2(nullptr);
        QByteArray pkt = buildTdsPacket(8.50);
        // Corrupt one byte
        pkt[3] = static_cast<char>(static_cast<uint8_t>(pkt[3]) ^ 0xFF);
        QVERIFY(!r2.validateChecksum(pkt));
    }

    void checksumInvalidForShortPacket() {
        DiFluidR2 r2(nullptr);
        QByteArray pkt;
        pkt.append(static_cast<char>(0xDF));
        pkt.append(static_cast<char>(0xDF));
        QVERIFY(!r2.validateChecksum(pkt));
    }

    // === TDS packet parsing ===

    void parseTdsPacketEmitsSignal() {
        DiFluidR2 r2(nullptr);
        QSignalSpy spy(&r2, &DiFluidR2::tdsChanged);

        QByteArray pkt = buildTdsPacket(8.50);
        r2.handlePacket(pkt);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toDouble(), 8.50);
        QCOMPARE(r2.tds(), 8.50);
    }

    void parseTdsPacketEmitsMeasurementComplete() {
        DiFluidR2 r2(nullptr);
        QSignalSpy spy(&r2, &DiFluidR2::measurementComplete);

        r2.handlePacket(buildTdsPacket(10.25));

        QCOMPARE(spy.count(), 1);
        QVERIFY(!r2.isMeasuring());
    }

    void parseTdsPacketZero() {
        DiFluidR2 r2(nullptr);
        QSignalSpy spy(&r2, &DiFluidR2::tdsChanged);

        r2.handlePacket(buildTdsPacket(0.0));

        QCOMPARE(spy.count(), 1);
        QCOMPARE(r2.tds(), 0.0);
    }

    void parseTdsPacketHighValue() {
        DiFluidR2 r2(nullptr);
        QSignalSpy spy(&r2, &DiFluidR2::tdsChanged);

        r2.handlePacket(buildTdsPacket(15.75));

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toDouble(), 15.75);
    }

    // === Temperature packet parsing ===

    void parseTemperaturePacket() {
        DiFluidR2 r2(nullptr);
        QSignalSpy spy(&r2, &DiFluidR2::temperatureChanged);

        r2.handlePacket(buildTemperaturePacket(23.50));

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toDouble(), 23.50);
        QCOMPARE(r2.temperature(), 23.50);
    }

    // === Status packet parsing ===

    void parseStatusReady() {
        DiFluidR2 r2(nullptr);
        QSignalSpy spy(&r2, &DiFluidR2::measuringChanged);

        r2.handlePacket(buildStatusPacket(0x00));  // Ready

        QVERIFY(!r2.isMeasuring());
    }

    void parseStatusNoLiquid() {
        DiFluidR2 r2(nullptr);
        QSignalSpy errorSpy(&r2, &DiFluidR2::errorOccurred);

        r2.handlePacket(buildStatusPacket(0x01));  // No liquid

        QCOMPARE(errorSpy.count(), 1);
        QVERIFY(errorSpy.at(0).at(0).toString().contains("liquid"));
    }

    void parseStatusBeyondRange() {
        DiFluidR2 r2(nullptr);
        QSignalSpy errorSpy(&r2, &DiFluidR2::errorOccurred);

        r2.handlePacket(buildStatusPacket(0x02));  // Beyond range

        QCOMPARE(errorSpy.count(), 1);
        QVERIFY(errorSpy.at(0).at(0).toString().contains("range"));
    }

    // === Invalid packets ===

    void rejectsShortPacket() {
        DiFluidR2 r2(nullptr);
        QSignalSpy tdsSpy(&r2, &DiFluidR2::tdsChanged);
        QSignalSpy tempSpy(&r2, &DiFluidR2::temperatureChanged);

        QByteArray shortPkt;
        shortPkt.append(static_cast<char>(0xDF));
        shortPkt.append(static_cast<char>(0xDF));
        r2.handlePacket(shortPkt);

        QCOMPARE(tdsSpy.count(), 0);
        QCOMPARE(tempSpy.count(), 0);
    }

    void rejectsWrongHeader() {
        DiFluidR2 r2(nullptr);
        QSignalSpy tdsSpy(&r2, &DiFluidR2::tdsChanged);

        QByteArray pkt = buildTdsPacket(8.50);
        pkt[0] = 0x00;  // Wrong header
        r2.handlePacket(pkt);

        QCOMPARE(tdsSpy.count(), 0);
    }

    void rejectsBadChecksum() {
        DiFluidR2 r2(nullptr);
        QSignalSpy tdsSpy(&r2, &DiFluidR2::tdsChanged);

        QByteArray pkt = buildTdsPacket(8.50);
        // Corrupt the checksum
        pkt[pkt.size() - 1] = static_cast<char>(static_cast<uint8_t>(pkt[pkt.size() - 1]) ^ 0xFF);
        r2.handlePacket(pkt);

        QCOMPARE(tdsSpy.count(), 0);
    }

    // === UUIDs ===

    void serviceUuidIsCorrect() {
        QCOMPARE(Refractometer::DiFluidR2::SERVICE.toString(),
                 QString("{0000ff00-0000-1000-8000-00805f9b34fb}"));
    }

    void characteristicUuidIsCorrect() {
        QCOMPARE(Refractometer::DiFluidR2::CHARACTERISTIC.toString(),
                 QString("{0000aa01-0000-1000-8000-00805f9b34fb}"));
    }
};

QTEST_GUILESS_MAIN(tst_DiFluidR2)
#include "tst_difluidr2.moc"
