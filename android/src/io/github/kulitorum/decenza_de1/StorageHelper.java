package io.github.kulitorum.decenza_de1;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

/**
 * Helper for external storage permission.
 * - Android 9-10: Requests WRITE_EXTERNAL_STORAGE at runtime
 * - Android 11+: Opens settings for MANAGE_EXTERNAL_STORAGE
 * This allows direct file access to Documents/Decenza for profile storage.
 */
public class StorageHelper {
    private static final String TAG = "StorageHelper";
    private static final int PERMISSION_REQUEST_CODE = 100;
    private static Activity sActivity;

    public static void init(Activity activity) {
        sActivity = activity;
    }

    /**
     * Check if we have external storage permission.
     * On Android 9-10, checks WRITE_EXTERNAL_STORAGE runtime permission.
     * On Android 11+, checks MANAGE_EXTERNAL_STORAGE.
     */
    public static boolean hasStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11+ (API 30+)
            return Environment.isExternalStorageManager();
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // Android 6-10 (API 23-29) - check runtime permission
            if (sActivity == null) return false;
            return ContextCompat.checkSelfPermission(sActivity,
                Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        } else {
            // Android 5 and below - permission granted at install
            return true;
        }
    }

    /**
     * Request storage permission.
     * On Android 9-10: Shows runtime permission dialog.
     * On Android 11+: Opens system settings.
     */
    public static void requestStoragePermission() {
        if (sActivity == null) {
            Log.e(TAG, "Activity not initialized");
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11+ - open settings for MANAGE_EXTERNAL_STORAGE
            try {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                Uri uri = Uri.fromParts("package", sActivity.getPackageName(), null);
                intent.setData(uri);
                sActivity.startActivity(intent);
                Log.i(TAG, "Opened storage permission settings");
            } catch (Exception e) {
                Log.w(TAG, "Could not open app-specific settings, trying general: " + e.getMessage());
                try {
                    Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                    sActivity.startActivity(intent);
                } catch (Exception e2) {
                    Log.e(TAG, "Could not open storage settings: " + e2.getMessage());
                }
            }
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // Android 6-10 - request runtime permission
            ActivityCompat.requestPermissions(sActivity,
                new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                PERMISSION_REQUEST_CODE);
            Log.i(TAG, "Requested WRITE_EXTERNAL_STORAGE permission");
        }
    }

    /**
     * Get the path to the profiles directory (Documents/Decenza).
     * Creates the directory if it doesn't exist.
     */
    public static String getProfilesPath() {
        // Use public Documents directory
        java.io.File documentsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS);
        java.io.File profilesDir = new java.io.File(documentsDir, "Decenza");

        if (!profilesDir.exists()) {
            if (profilesDir.mkdirs()) {
                Log.i(TAG, "Created profiles directory: " + profilesDir.getAbsolutePath());
            } else {
                Log.e(TAG, "Failed to create profiles directory: " + profilesDir.getAbsolutePath());
            }
        }

        return profilesDir.getAbsolutePath();
    }

    /**
     * Check if we need to request storage permission.
     * Returns true for Android 6+ where runtime permission is needed.
     */
    public static boolean needsStoragePermission() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.M;
    }
}
