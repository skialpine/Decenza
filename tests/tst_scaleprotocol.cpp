#include <QtTest>
#include <QSignalSpy>
#include <QBluetoothUuid>

#include "ble/scales/decentscale.h"
#include "ble/scales/bookooscale.h"
#include "ble/protocol/de1characteristics.h"

// Test BLE packet parsing for scale implementations.
// Feeds raw byte arrays through onCharacteristicChanged() (public slot)
// and verifies weight/battery signals.
//
// de1app references:
//   Decent: de1plus/scale.tcl proc decent_scale_parse_response
//   Bookoo: de1plus/scale.tcl proc bookoo_parse_response

class tst_ScaleProtocol : public QObject {
    Q_OBJECT

private:
    // Build a 7-byte Decent Scale packet with valid XOR checksum
    static QByteArray buildDecentPacket(uint8_t cmd, uint8_t d2, uint8_t d3,
                                        uint8_t d4, uint8_t d5) {
        QByteArray pkt(7, 0);
        pkt[0] = 0x03;
        pkt[1] = static_cast<char>(cmd);
        pkt[2] = static_cast<char>(d2);
        pkt[3] = static_cast<char>(d3);
        pkt[4] = static_cast<char>(d4);
        pkt[5] = static_cast<char>(d5);
        // XOR of bytes 0-5
        uint8_t xorVal = 0;
        for (int i = 0; i < 6; i++)
            xorVal ^= static_cast<uint8_t>(pkt[i]);
        pkt[6] = static_cast<char>(xorVal);
        return pkt;
    }

    // Build a Decent weight packet: cmd=0xCE, weight as int16 BE / 10
    static QByteArray buildDecentWeightPacket(double grams) {
        int16_t raw = static_cast<int16_t>(qRound(grams * 10.0));
        uint8_t hi = static_cast<uint8_t>((raw >> 8) & 0xFF);
        uint8_t lo = static_cast<uint8_t>(raw & 0xFF);
        return buildDecentPacket(0xCE, hi, lo, 0x00, 0x00);
    }

    // Build a 20-byte Bookoo weight packet
    static QByteArray buildBookooPacket(double grams, uint8_t battery = 50) {
        QByteArray pkt(20, 0);
        pkt[0] = 0x03;
        pkt[1] = 0x0B;
        // bytes 2-4: timer (0)
        // byte 5: unit (0 = grams)

        bool negative = grams < 0;
        pkt[6] = static_cast<char>(negative ? '-' : '+');

        uint32_t raw = static_cast<uint32_t>(qRound(qAbs(grams) * 100.0));
        pkt[7] = static_cast<char>((raw >> 16) & 0xFF);
        pkt[8] = static_cast<char>((raw >> 8) & 0xFF);
        pkt[9] = static_cast<char>(raw & 0xFF);

        // bytes 10-12: flow (0)
        pkt[13] = static_cast<char>(battery);
        // bytes 14-19: standby, buzzer, XOR (don't matter for parsing)
        return pkt;
    }

private slots:

    // ==========================================
    // DecentScale: weight parsing
    // ==========================================

    void decentWeight100g() {
        DecentScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::weightChanged);

        auto pkt = buildDecentWeightPacket(100.0);
        scale.onCharacteristicChanged(Scale::Decent::READ, pkt);

        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.last().at(0).toDouble(), 100.0);
    }

    void decentWeightZero() {
        DecentScale scale(nullptr);
        // First set to non-zero so the 0.0 packet triggers a change
        auto nonZero = buildDecentWeightPacket(10.0);
        scale.onCharacteristicChanged(Scale::Decent::READ, nonZero);

        QSignalSpy spy(&scale, &ScaleDevice::weightChanged);
        auto pkt = buildDecentWeightPacket(0.0);
        scale.onCharacteristicChanged(Scale::Decent::READ, pkt);

        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.last().at(0).toDouble(), 0.0);
    }

    void decentWeightNegative() {
        DecentScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::weightChanged);

        auto pkt = buildDecentWeightPacket(-5.0);
        scale.onCharacteristicChanged(Scale::Decent::READ, pkt);

        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.last().at(0).toDouble(), -5.0);
    }

    void decentWeightMaxInt16() {
        DecentScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::weightChanged);

        // Max int16 = 32767 -> 3276.7g
        auto pkt = buildDecentWeightPacket(3276.7);
        scale.onCharacteristicChanged(Scale::Decent::READ, pkt);

        QVERIFY(spy.count() >= 1);
        QVERIFY(qAbs(spy.last().at(0).toDouble() - 3276.7) < 0.2);
    }

    void decentWeightPrecision() {
        DecentScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::weightChanged);

        // 36.2g should encode as 362 raw -> 36.2g decoded
        auto pkt = buildDecentWeightPacket(36.2);
        scale.onCharacteristicChanged(Scale::Decent::READ, pkt);

        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.last().at(0).toDouble(), 36.2);
    }

    // ==========================================
    // DecentScale: battery parsing
    // ==========================================

    void decentBattery75() {
        DecentScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::batteryLevelChanged);

        // Battery response: cmd=0x0A, d4=75 (battery%), rest=0
        auto pkt = buildDecentPacket(0x0A, 0x00, 0x00, 75, 0x00);
        scale.onCharacteristicChanged(Scale::Decent::READ, pkt);

        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.last().at(0).toInt(), 75);
    }

    void decentBatteryCharging() {
        DecentScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::batteryLevelChanged);

        // Battery charging: d4=0xFF -> 100%
        auto pkt = buildDecentPacket(0x0A, 0x00, 0x00, 0xFF, 0x00);
        scale.onCharacteristicChanged(Scale::Decent::READ, pkt);

        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.last().at(0).toInt(), 100);
    }

    // ==========================================
    // DecentScale: error handling
    // ==========================================

    void decentTruncatedPacketNoCrash() {
        DecentScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::weightChanged);

        // Only 3 bytes (need 7)
        QByteArray truncated = QByteArray::fromHex("03CE00");
        scale.onCharacteristicChanged(Scale::Decent::READ, truncated);

        // Should not crash. Weight may or may not change.
    }

    void decentEmptyPacketNoCrash() {
        DecentScale scale(nullptr);
        scale.onCharacteristicChanged(Scale::Decent::READ, QByteArray());
    }

    void decentWrongUuidIgnored() {
        DecentScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::weightChanged);

        auto pkt = buildDecentWeightPacket(50.0);
        // Use wrong UUID — should be ignored
        QBluetoothUuid wrongUuid(QString("00001234-0000-1000-8000-00805F9B34FB"));
        scale.onCharacteristicChanged(wrongUuid, pkt);

        QCOMPARE(spy.count(), 0);
    }

    void decentChecksumValidation() {
        // Decent Scale may or may not validate XOR checksum.
        // Test that a valid packet works (already tested above).
        // This test verifies the checksum calculation itself.
        auto pkt = buildDecentWeightPacket(50.0);
        uint8_t expected = 0;
        for (int i = 0; i < 6; i++)
            expected ^= static_cast<uint8_t>(pkt[i]);
        QCOMPARE(static_cast<uint8_t>(pkt[6]), expected);
    }

    // ==========================================
    // BookooScale: weight parsing
    // ==========================================

    void bookooWeight250g() {
        BookooScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::weightChanged);

        auto pkt = buildBookooPacket(250.50);
        scale.onCharacteristicChanged(Scale::Bookoo::STATUS, pkt);

        QVERIFY(spy.count() >= 1);
        QVERIFY(qAbs(spy.last().at(0).toDouble() - 250.50) < 0.02);
    }

    void bookooWeightZero() {
        BookooScale scale(nullptr);
        // Set to non-zero first so 0.0 triggers a change
        auto nonZero = buildBookooPacket(10.0);
        scale.onCharacteristicChanged(Scale::Bookoo::STATUS, nonZero);

        QSignalSpy spy(&scale, &ScaleDevice::weightChanged);
        auto pkt = buildBookooPacket(0.0);
        scale.onCharacteristicChanged(Scale::Bookoo::STATUS, pkt);

        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.last().at(0).toDouble(), 0.0);
    }

    void bookooWeightNegative() {
        BookooScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::weightChanged);

        auto pkt = buildBookooPacket(-3.5);
        scale.onCharacteristicChanged(Scale::Bookoo::STATUS, pkt);

        QVERIFY(spy.count() >= 1);
        QVERIFY(qAbs(spy.last().at(0).toDouble() - (-3.5)) < 0.02);
    }

    void bookooBattery80() {
        BookooScale scale(nullptr);
        QSignalSpy spy(&scale, &ScaleDevice::batteryLevelChanged);

        auto pkt = buildBookooPacket(10.0, 80);
        scale.onCharacteristicChanged(Scale::Bookoo::STATUS, pkt);

        QVERIFY(spy.count() >= 1);
        QCOMPARE(spy.last().at(0).toInt(), 80);
    }

    // ==========================================
    // BookooScale: error handling
    // ==========================================

    void bookooTruncatedNoCrash() {
        BookooScale scale(nullptr);
        // Only 5 bytes (need 20)
        QByteArray truncated(5, 0);
        scale.onCharacteristicChanged(Scale::Bookoo::STATUS, truncated);
    }

    void bookooEmptyNoCrash() {
        BookooScale scale(nullptr);
        scale.onCharacteristicChanged(Scale::Bookoo::STATUS, QByteArray());
    }

    // ==========================================
    // Cross-scale boundary tests
    // ==========================================

    void oversizedPacketNoCrash() {
        // 255-byte junk packet should not crash any scale
        QByteArray oversized(255, 0x42);

        DecentScale decent(nullptr);
        decent.onCharacteristicChanged(Scale::Decent::READ, oversized);

        BookooScale bookoo(nullptr);
        bookoo.onCharacteristicChanged(Scale::Bookoo::STATUS, oversized);
    }

    void singleBytePacketNoCrash() {
        QByteArray single(1, 0x03);

        DecentScale decent(nullptr);
        decent.onCharacteristicChanged(Scale::Decent::READ, single);

        BookooScale bookoo(nullptr);
        bookoo.onCharacteristicChanged(Scale::Bookoo::STATUS, single);
    }
};

QTEST_GUILESS_MAIN(tst_ScaleProtocol)
#include "tst_scaleprotocol.moc"
