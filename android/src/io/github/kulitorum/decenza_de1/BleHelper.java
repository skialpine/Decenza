package io.github.kulitorum.decenza_de1;

import android.bluetooth.BluetoothGatt;
import android.util.Log;
import java.lang.reflect.Field;
import java.util.Map;

/**
 * Android-only BLE helper utilities not exposed by Qt's QLowEnergyController.
 *
 * Qt 6 does not expose BluetoothGatt.requestConnectionPriority() through its
 * public API. This class uses reflection on Qt's internal QtBluetoothLE class
 * to access the underlying BluetoothGatt and call Android-specific methods.
 *
 * Reflection targets are best-effort: if Qt changes internal field names the
 * call silently no-ops and logs a warning.
 */
public class BleHelper {
    private static final String TAG = "DecenzaBleHelper";

    /**
     * Request CONNECTION_PRIORITY_HIGH for the GATT connection to the given
     * MAC address. Call this after QLowEnergyController emits connected().
     *
     * CONNECTION_PRIORITY_HIGH reduces the BLE connection interval from the
     * default ~30-50 ms to 7.5-15 ms, which reduces how long Android GC
     * pauses delay BLE notification delivery and BLE write commands.
     *
     * @param macAddress The Bluetooth MAC address (e.g. "AA:BB:CC:DD:EE:FF")
     * @return true if the request was submitted, false if it could not be made
     */
    public static boolean requestHighConnectionPriority(String macAddress) {
        try {
            // Qt 6 stores all LE controller instances in a static HashMap on
            // org.qtproject.qt.android.bluetooth.QtBluetoothLE.
            Class<?> qtBleClass = Class.forName(
                "org.qtproject.qt.android.bluetooth.QtBluetoothLE");

            Field leMapField = qtBleClass.getDeclaredField("leMap");
            leMapField.setAccessible(true);
            Map<?, ?> leMap = (Map<?, ?>) leMapField.get(null);

            if (leMap == null || leMap.isEmpty()) {
                Log.w(TAG, "requestHighConnectionPriority: Qt leMap is null or empty");
                return false;
            }

            Field gattField = qtBleClass.getDeclaredField("mBluetoothGatt");
            gattField.setAccessible(true);

            for (Object instance : leMap.values()) {
                BluetoothGatt gatt = (BluetoothGatt) gattField.get(instance);
                if (gatt == null) continue;
                if (macAddress.equalsIgnoreCase(gatt.getDevice().getAddress())) {
                    gatt.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH);
                    Log.i(TAG, "Requested CONNECTION_PRIORITY_HIGH for " + macAddress);
                    return true;
                }
            }

            Log.w(TAG, "requestHighConnectionPriority: no GATT found for " + macAddress);
        } catch (ClassNotFoundException e) {
            Log.w(TAG, "requestHighConnectionPriority: Qt internal class not found: " + e.getMessage());
        } catch (NoSuchFieldException e) {
            Log.w(TAG, "requestHighConnectionPriority: Qt internal field not found (Qt version mismatch?): " + e.getMessage());
        } catch (Exception e) {
            Log.w(TAG, "requestHighConnectionPriority: unexpected error: " + e.getMessage());
        }
        return false;
    }
}
