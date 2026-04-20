package io.github.kulitorum.decenza_de1;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

/**
 * Relaunches Decenza after a self-update.
 *
 * The legacy Intent(ACTION_VIEW) install flow used to provide this via the
 * system package-installer activity's "Open" button. PackageInstaller's session
 * API has no equivalent post-install UI, and the dynamic BroadcastReceiver we
 * register in ApkInstaller cannot handle STATUS_SUCCESS for a self-update
 * because Android kills the old process when it replaces the package on disk,
 * before the status broadcast is delivered.
 *
 * ACTION_MY_PACKAGE_REPLACED is delivered by the system to a manifest-
 * registered receiver running in the NEW process after a self-update, which
 * is exactly the right hook to bring the updated app back to the foreground.
 */
public class UpdateLaunchReceiver extends BroadcastReceiver {
    private static final String TAG = "DecenzaUpdateLaunch";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (!Intent.ACTION_MY_PACKAGE_REPLACED.equals(intent.getAction())) {
            return;
        }
        try {
            Intent launch = context.getPackageManager()
                    .getLaunchIntentForPackage(context.getPackageName());
            if (launch != null) {
                launch.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(launch);
                Log.i(TAG, "launched updated app after ACTION_MY_PACKAGE_REPLACED");
            } else {
                Log.w(TAG, "no launch intent for package " + context.getPackageName());
            }
        } catch (Throwable t) {
            Log.w(TAG, "failed to launch updated app: " + t);
        }
    }
}
