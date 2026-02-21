# Factory Reset Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a "Remove All Data & Uninstall/Quit" button with two-stage confirmation that wipes all app data and triggers uninstall (Android) or quit (other platforms).

**Architecture:** The wipe logic lives in `Settings::factoryReset()` since Settings already has access to `QSettings` and knows all storage paths. `MainController::factoryResetAndQuit()` orchestrates: close the shot database, call factoryReset, then trigger platform-specific exit (JNI uninstall intent on Android, `QCoreApplication::quit()` elsewhere). QML provides the button and two confirmation dialogs.

**Tech Stack:** Qt 6 C++17, QML, JNI (Android only), QSettings, QStandardPaths, QDir

**Design doc:** `docs/plans/2026-02-21-factory-reset-design.md`

---

### Task 1: Add `requestUninstall()` to StorageHelper.java (Android)

**Files:**
- Modify: `android/src/io/github/kulitorum/decenza_de1/StorageHelper.java`

**Step 1: Add the `requestUninstall` method**

Add this method at the end of the `StorageHelper` class (before the closing `}`), after the existing `zipDirectory` method:

```java
    /**
     * Request the system to uninstall this app.
     * Shows the native Android uninstall confirmation dialog.
     */
    public static void requestUninstall() {
        if (sActivity == null) {
            Log.e(TAG, "Activity not initialized, cannot request uninstall");
            return;
        }

        try {
            Intent intent = new Intent(Intent.ACTION_DELETE);
            intent.setData(Uri.parse("package:" + sActivity.getPackageName()));
            sActivity.startActivity(intent);
            Log.i(TAG, "Launched uninstall dialog");
        } catch (Exception e) {
            Log.e(TAG, "Failed to launch uninstall dialog: " + e.getMessage());
        }
    }
```

**Step 2: Commit**

```bash
git add android/src/io/github/kulitorum/decenza_de1/StorageHelper.java
git commit -m "feat: add requestUninstall() to StorageHelper for factory reset"
```

---

### Task 2: Add `factoryReset()` to Settings

**Files:**
- Modify: `src/core/settings.h` (add method declaration)
- Modify: `src/core/settings.cpp` (add method implementation)

**Step 1: Add declaration to settings.h**

Add after `Q_INVOKABLE void resetLayoutToDefault();` (line 659):

```cpp
    Q_INVOKABLE void factoryReset();
```

**Step 2: Add implementation to settings.cpp**

Add at the end of the file, before any closing comments. This method wipes all QSettings and deletes all data directories:

```cpp
void Settings::factoryReset()
{
    qWarning() << "Settings::factoryReset() - WIPING ALL DATA";

    // 1. Clear primary QSettings (favorites, presets, theme, all preferences)
    m_settings.clear();
    m_settings.sync();

    // 2. Clear secondary QSettings store (used by AI, location, profilestorage)
    QSettings defaultSettings;
    defaultSettings.clear();
    defaultSettings.sync();

    // 3. Delete all data directories under AppDataLocation
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QStringList dataDirs = {
        "profiles",
        "library",
        "skins",
        "translations",
        "screensaver_videos"
    };

    for (const QString& subdir : dataDirs) {
        QDir dir(appDataDir + "/" + subdir);
        if (dir.exists()) {
            qWarning() << "  Removing:" << dir.absolutePath();
            dir.removeRecursively();
        }
    }

    // 4. Delete shot database files
    QStringList dbFiles = {"shots.db", "shots.db-wal", "shots.db-shm"};
    for (const QString& dbFile : dbFiles) {
        QString path = appDataDir + "/" + dbFile;
        if (QFile::exists(path)) {
            qWarning() << "  Removing:" << path;
            QFile::remove(path);
        }
    }

    // 5. Delete log files in AppDataLocation
    QStringList logFiles = {"debug.log", "crash.log", "steam_debug.log"};
    for (const QString& logFile : logFiles) {
        QString path = appDataDir + "/" + logFile;
        if (QFile::exists(path)) {
            QFile::remove(path);
        }
    }

    // 6. Delete public Documents directories
    QString docsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QStringList publicDirs = {"Decenza", "Decenza Backups", "ai_logs"};
    for (const QString& pubDir : publicDirs) {
        QDir dir(docsDir + "/" + pubDir);
        if (dir.exists()) {
            qWarning() << "  Removing:" << dir.absolutePath();
            dir.removeRecursively();
        }
    }

    // 7. Delete visualizer debug files in Documents
    QStringList debugFiles = {
        docsDir + "/last_upload.json",
        docsDir + "/last_upload_debug.txt",
        docsDir + "/last_upload_response.txt"
    };
    for (const QString& debugFile : debugFiles) {
        if (QFile::exists(debugFile)) {
            QFile::remove(debugFile);
        }
    }

    // 8. Clear cache directory
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir cache(cacheDir);
    if (cache.exists()) {
        qWarning() << "  Clearing cache:" << cache.absolutePath();
        cache.removeRecursively();
    }

    qWarning() << "Settings::factoryReset() - COMPLETE";
}
```

You will also need to add `#include <QDir>` at the top of `settings.cpp` if it's not already there. Check first — it likely is already included.

**Step 3: Commit**

```bash
git add src/core/settings.h src/core/settings.cpp
git commit -m "feat: add factoryReset() to Settings - wipes all app data"
```

---

### Task 3: Add `factoryResetAndQuit()` to MainController

**Files:**
- Modify: `src/controllers/maincontroller.h` (add method declaration)
- Modify: `src/controllers/maincontroller.cpp` (add method implementation)

**Step 1: Add declaration to maincontroller.h**

Add after the `clearCrashLog()` declaration (line 241):

```cpp
    Q_INVOKABLE void factoryResetAndQuit();
```

**Step 2: Add implementation to maincontroller.cpp**

Add the method implementation. This method must:
1. Close the shot database (so the files can be deleted)
2. Call `Settings::factoryReset()`
3. On Android: call JNI `StorageHelper.requestUninstall()`
4. Quit the app

```cpp
void MainController::factoryResetAndQuit()
{
    qWarning() << "MainController::factoryResetAndQuit() - Starting factory reset";

    // 1. Close the shot database so files can be deleted
    if (m_shotHistory) {
        m_shotHistory->close();
    }

    // 2. Wipe all data
    m_settings->factoryReset();

    // 3. Platform-specific exit
#ifdef Q_OS_ANDROID
    // Launch system uninstall dialog
    QJniObject::callStaticMethod<void>(
        "io/github/kulitorum/decenza_de1/StorageHelper",
        "requestUninstall",
        "()V");
#endif

    // 4. Quit the app
    QCoreApplication::quit();
}
```

You will need these includes at the top of `maincontroller.cpp` (check if already present — `QJniObject` is likely already included behind `#ifdef Q_OS_ANDROID`):

```cpp
#include <QCoreApplication>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif
```

**Step 3: Check that `ShotHistoryStorage` has a `close()` method**

Look at `src/history/shothistorystorage.h` for a `close()` method. If it doesn't exist, you need to add one that calls `m_db.close()`. The `performDatabaseCopy()` method already does checkpoint + close + reopen, so look at that pattern. If there's no standalone `close()`, add:

In `shothistorystorage.h`:
```cpp
    void close();
```

In `shothistorystorage.cpp`:
```cpp
void ShotHistoryStorage::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(DB_CONNECTION_NAME);
}
```

**Step 4: Commit**

```bash
git add src/controllers/maincontroller.h src/controllers/maincontroller.cpp src/history/shothistorystorage.h src/history/shothistorystorage.cpp
git commit -m "feat: add factoryResetAndQuit() to MainController"
```

---

### Task 4: Add Factory Reset button and confirmation dialogs to SettingsDataTab.qml

**Files:**
- Modify: `qml/pages/settings/SettingsDataTab.qml`

**Step 1: Add the button to the left column**

In `SettingsDataTab.qml`, the left column's `ColumnLayout` ends with `Item { Layout.fillHeight: true }` at line 98, just before the "Your Data" section. Add the factory reset button **after** the `GridLayout` that shows shot/profile counts (after line 138), before the closing `}` of the left column's `ColumnLayout`:

```qml
                Item { Layout.fillHeight: true }

                // Factory reset button
                AccessibleButton {
                    Layout.fillWidth: true
                    destructive: true
                    text: Qt.platform.os === "android" ?
                          TranslationManager.translate("settings.data.resetuninstall", "Remove All Data & Uninstall") :
                          TranslationManager.translate("settings.data.resetquit", "Remove All Data & Quit")
                    accessibleName: Qt.platform.os === "android" ?
                          TranslationManager.translate("settings.data.resetuninstallAccessible",
                              "Remove all app data and uninstall the application") :
                          TranslationManager.translate("settings.data.resetquitAccessible",
                              "Remove all app data and quit the application")
                    onClicked: factoryResetDialog1.open()
                }
```

Replace the existing `Item { Layout.fillHeight: true }` at line 98 with the button above (which includes its own `Item { Layout.fillHeight: true }` spacer before it).

**Step 2: Add the two confirmation dialogs**

Add these two `Dialog` components at the bottom of the file, just before the final closing `}` of `KeyboardAwareContainer` (before line 1334). Place them alongside the existing `restoreConfirmDialog`:

```qml
    // Factory Reset - Confirmation Dialog 1
    Dialog {
        id: factoryResetDialog1
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Theme.scaled(400)
        padding: 0
        modal: true

        background: Rectangle {
            color: Theme.surfaceColor
            radius: Theme.cardRadius
            border.width: 1
            border.color: Theme.borderColor
        }

        contentItem: ColumnLayout {
            spacing: 0

            // Header
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.scaled(50)

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.scaled(20)
                    anchors.verticalCenter: parent.verticalCenter
                    text: TranslationManager.translate("settings.data.factoryresettitle", "Remove All Data?")
                    font: Theme.titleFont
                    color: Theme.errorColor
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 1
                    color: Theme.borderColor
                }
            }

            // Body
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.scaled(20)
                spacing: Theme.scaled(15)

                Text {
                    Layout.fillWidth: true
                    text: TranslationManager.translate("settings.data.factoryresetbody",
                        "This will permanently delete ALL your data: settings, favourites, profiles, shot history, themes, and everything else. This cannot be undone.")
                    color: Theme.textColor
                    font: Theme.bodyFont
                    wrapMode: Text.WordWrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.scaled(10)

                    Item { Layout.fillWidth: true }

                    AccessibleButton {
                        text: TranslationManager.translate("common.cancel", "Cancel")
                        accessibleName: TranslationManager.translate("settings.data.cancelFactoryReset", "Cancel factory reset")
                        onClicked: factoryResetDialog1.close()
                    }

                    AccessibleButton {
                        destructive: true
                        text: TranslationManager.translate("settings.data.factoryresetcontinue", "Continue")
                        accessibleName: TranslationManager.translate("settings.data.factoryresetcontinueAccessible",
                            "Continue with factory reset, shows final confirmation")
                        onClicked: {
                            factoryResetDialog1.close()
                            factoryResetDialog2.open()
                        }
                    }
                }
            }
        }
    }

    // Factory Reset - Confirmation Dialog 2 (the fun one)
    Dialog {
        id: factoryResetDialog2
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Theme.scaled(400)
        padding: 0
        modal: true

        background: Rectangle {
            color: Theme.surfaceColor
            radius: Theme.cardRadius
            border.width: 1
            border.color: Theme.borderColor
        }

        contentItem: ColumnLayout {
            spacing: 0

            // Header
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.scaled(50)

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.scaled(20)
                    anchors.verticalCenter: parent.verticalCenter
                    text: TranslationManager.translate("settings.data.factoryresettitle2", "Are you REALLY sure?")
                    font: Theme.titleFont
                    color: Theme.errorColor
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 1
                    color: Theme.borderColor
                }
            }

            // Body
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.scaled(20)
                spacing: Theme.scaled(15)

                Text {
                    Layout.fillWidth: true
                    text: TranslationManager.translate("settings.data.factoryresetbody2",
                        "This is your last chance. All your espresso data, your carefully dialled-in profiles, your shot history \u2014 gone. Poof. Like that time you forgot to put the drip tray back.")
                    color: Theme.textColor
                    font: Theme.bodyFont
                    wrapMode: Text.WordWrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.scaled(10)

                    Item { Layout.fillWidth: true }

                    AccessibleButton {
                        text: TranslationManager.translate("settings.data.factoryresetchangedmind", "I changed my mind")
                        accessibleName: TranslationManager.translate("settings.data.factoryresetchangedmindAccessible",
                            "Cancel factory reset and keep all data")
                        onClicked: factoryResetDialog2.close()
                    }

                    AccessibleButton {
                        destructive: true
                        text: TranslationManager.translate("settings.data.factoryreset.nuke", "Yes, nuke everything")
                        accessibleName: TranslationManager.translate("settings.data.factoryreset.nukeAccessible",
                            "Confirm: permanently delete all data and exit the app")
                        onClicked: {
                            factoryResetDialog2.close()
                            MainController.factoryResetAndQuit()
                        }
                    }
                }
            }
        }
    }
```

**Step 3: Commit**

```bash
git add qml/pages/settings/SettingsDataTab.qml
git commit -m "feat: add factory reset button with two-stage confirmation dialogs"
```

---

### Task 5: Build and verify

**Step 1: Build the project**

Don't build automatically — let the user build in Qt Creator.

**Step 2: Manual test checklist**

- [ ] Button appears at bottom of left column in Settings > Data
- [ ] Button text shows "Remove All Data & Uninstall" on Android, "Remove All Data & Quit" on desktop
- [ ] Clicking button shows first confirmation dialog
- [ ] Clicking Cancel on first dialog dismisses it
- [ ] Clicking Continue on first dialog shows second dialog with funny text
- [ ] Clicking "I changed my mind" on second dialog dismisses it
- [ ] Clicking "Yes, nuke everything" wipes data and triggers uninstall/quit
- [ ] After uninstall+reinstall on Android, app starts completely fresh (no favourites, no settings)
- [ ] On desktop, after quit + relaunch, app starts with default settings

**Step 3: Commit final state**

```bash
git add -A
git commit -m "feat: factory reset - remove all data and uninstall"
```
