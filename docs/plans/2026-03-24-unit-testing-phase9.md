# Phase 9: Scale Protocol Parsing

> **For Claude:** Construct raw BLE byte arrays and feed them to scale onCharacteristicChanged methods. Verify parsed weight, battery, and command responses. Needs friend class access behind DECENZA_TESTING.

**Rationale:** 13 scale implementations parse BLE packets independently. The Bookoo tare oscillation bug (#341) and Acaia protocol issues show this layer has parsing bugs. Boundary values, truncated packets, and sign handling are the most likely bug sources.

## Test File: `tests/tst_scaleprotocol.cpp`

**Source deps:** Scale source files + BLE_SOURCES. Needs friend class or test helper on scale classes.

**Prerequisites:**
- Add `friend class tst_ScaleProtocol;` to DecentScale, BookooScale, AcaiaScale under `#ifdef DECENZA_TESTING`
- Or add a public `testParsePacket(const QByteArray&)` method behind `#ifdef DECENZA_TESTING`

### Test Cases (~20)

**DecentScale (7-byte packets with XOR checksum):**
- [ ] Valid weight packet (0xCE): 100.0g → parsed correctly
- [ ] Negative weight packet: -5.0g → parsed as negative
- [ ] Zero weight: 0.0g
- [ ] Max weight: 3276.7g (int16 max / 10)
- [ ] Battery packet (0x0A): 75% → batteryLevel=75
- [ ] Battery charging (0xFF) → batteryLevel=100
- [ ] Truncated packet (< 7 bytes) → no crash, weight unchanged
- [ ] Invalid checksum → packet rejected

**BookooScale (20-byte notifications):**
- [ ] Valid weight: 250.50g → parsed from bytes [6-9]
- [ ] Negative weight (sign char '-' at byte 6) → negative value
- [ ] Battery at byte 13 → batteryLevel set
- [ ] Truncated packet (< 20 bytes) → no crash
- [ ] Zero weight → 0.0g

**AcaiaScale (variable-length protocol):**
- [ ] Valid weight response → weight parsed
- [ ] Truncated packet → no crash
- [ ] Protocol negotiation: ident response → sets connected state

**Cross-scale boundary tests:**
- [ ] Empty QByteArray → no crash for all 3 scales
- [ ] Single-byte packet → no crash for all 3 scales
- [ ] 255-byte packet (oversized) → no crash for all 3 scales
