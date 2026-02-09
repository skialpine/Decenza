import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import DecenzaDE1
import "../components"

Page {
    id: settingsPage
    objectName: "settingsPage"
    background: Rectangle { color: Theme.backgroundColor }

    Component.onCompleted: root.currentPageTitle = TranslationManager.translate("settings.title", "Settings")
    StackView.onActivated: root.currentPageTitle = TranslationManager.translate("settings.title", "Settings")

    // Requested tab to switch to (set before pushing page)
    property int requestedTabIndex: -1

    // Timer to switch tab after page is fully loaded
    Timer {
        id: tabSwitchTimer
        interval: 500  // Wait for all tabs to be created
        repeat: false
        onTriggered: {
            if (settingsPage.requestedTabIndex >= 0) {
                tabBar.currentIndex = settingsPage.requestedTabIndex
                settingsPage.requestedTabIndex = -1
            }
        }
    }

    StackView.onActivating: {
        if (requestedTabIndex >= 0) {
            tabSwitchTimer.start()
        }
    }

    // Tab bar at top
    TabBar {
        id: tabBar
        anchors.top: parent.top
        anchors.topMargin: Theme.pageTopMargin
        anchors.left: parent.left
        anchors.leftMargin: Theme.standardMargin
        z: 2

        property bool accessibilityCustomHandler: true

        onCurrentIndexChanged: {
            if (typeof AccessibilityManager !== "undefined" && AccessibilityManager.enabled) {
                // Build tab names based on which tabs are visible
                var tabNames = ["Bluetooth", "Preferences", "Options", "Screensaver", "Visualizer", "AI", "Accessibility", "Themes", "Layout", "Language", "History", "Data", "MQTT"]
                if (MainController.updateChecker.canCheckForUpdates) tabNames.push("Update")
                tabNames.push("About")
                if (Settings.isDebugBuild) tabNames.push("Debug")
                if (currentIndex >= 0 && currentIndex < tabNames.length) {
                    AccessibilityManager.announce(tabNames[currentIndex] + " tab")
                }
            }
        }

        // Override Material contentItem to remove the accent-colored highlight indicator
        contentItem: ListView {
            model: tabBar.contentModel
            currentIndex: tabBar.currentIndex
            spacing: tabBar.spacing
            orientation: ListView.Horizontal
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.AutoFlickIfNeeded
            snapMode: ListView.SnapToItem
            highlightMoveDuration: 0
            highlight: Item {}  // No Material indicator
        }

        background: Rectangle {
            color: "transparent"
            // Bottom border line (active tab extends below to cover its portion)
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.borderColor
            }
        }

        StyledTabButton {
            id: bluetoothTab
            text: TranslationManager.translate("settings.tab.bluetooth", "Bluetooth")
            tabLabel: TranslationManager.translate("settings.tab.bluetooth", "Bluetooth")
        }

        StyledTabButton {
            id: preferencesTabButton
            text: TranslationManager.translate("settings.tab.preferences", "Preferences")
            tabLabel: TranslationManager.translate("settings.tab.preferences", "Preferences")
        }

        StyledTabButton {
            id: optionsTabButton
            text: TranslationManager.translate("settings.tab.options", "Options")
            tabLabel: TranslationManager.translate("settings.tab.options", "Options")
        }

        StyledTabButton {
            id: screensaverTab
            text: TranslationManager.translate("settings.tab.screensaver", "Screensaver")
            tabLabel: TranslationManager.translate("settings.tab.screensaver", "Screensaver")
        }

        StyledTabButton {
            id: visualizerTabButton
            text: TranslationManager.translate("settings.tab.visualizer", "Visualizer")
            tabLabel: TranslationManager.translate("settings.tab.visualizer", "Visualizer")
        }

        StyledTabButton {
            id: aiTabButton
            text: TranslationManager.translate("settings.tab.ai", "AI")
            tabLabel: TranslationManager.translate("settings.tab.ai", "AI")
        }

        StyledTabButton {
            id: accessibilityTabButton
            text: TranslationManager.translate("settings.tab.accessibility", "Access")
            tabLabel: TranslationManager.translate("settings.tab.accessibility.full", "Accessibility")
        }

        StyledTabButton {
            id: themesTabButton
            text: TranslationManager.translate("settings.tab.themes", "Themes")
            tabLabel: TranslationManager.translate("settings.tab.themes", "Themes")
        }

        StyledTabButton {
            id: layoutTabButton
            text: TranslationManager.translate("settings.tab.layout", "Layout")
            tabLabel: TranslationManager.translate("settings.tab.layout", "Layout")
        }

        // Language tab with badge for untranslated strings
        StyledTabButton {
            id: languageTabButton
            text: TranslationManager.translate("settings.tab.language", "Language")
            tabLabel: TranslationManager.translate("settings.tab.language", "Language")

            // Override contentItem to add badge
            contentItem: Row {
                spacing: Theme.scaled(4)
                Text {
                    text: languageTabButton.text
                    font: languageTabButton.font
                    color: languageTabButton.checked ? Theme.textColor : Theme.textSecondaryColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
                Rectangle {
                    visible: TranslationManager.currentLanguage !== "en" && TranslationManager.untranslatedCount > 0
                    width: badgeText.width + 8
                    height: Theme.scaled(16)
                    radius: Theme.scaled(8)
                    color: Theme.warningColor
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: TranslationManager.untranslatedCount > 99 ? "99+" : TranslationManager.untranslatedCount
                        font.pixelSize: Theme.scaled(10)
                        font.bold: true
                        color: "white"
                    }
                }
            }
        }

        StyledTabButton {
            id: historyTabButton
            text: TranslationManager.translate("settings.tab.history", "History")
            tabLabel: TranslationManager.translate("settings.tab.history", "History")
        }

        StyledTabButton {
            id: dataTabButton
            text: TranslationManager.translate("settings.tab.data", "Data")
            tabLabel: TranslationManager.translate("settings.tab.data", "Data")
        }

        StyledTabButton {
            id: mqttTabButton
            text: TranslationManager.translate("settings.tab.mqtt", "MQTT")
            tabLabel: TranslationManager.translate("settings.tab.mqtt", "MQTT")
        }

        StyledTabButton {
            id: updateTabButton
            visible: MainController.updateChecker.canCheckForUpdates
            text: TranslationManager.translate("settings.tab.update", "Update")
            tabLabel: TranslationManager.translate("settings.tab.update", "Update")
            width: visible ? implicitWidth : 0
        }

        StyledTabButton {
            id: aboutTabButton
            text: TranslationManager.translate("settings.tab.about", "About")
            tabLabel: TranslationManager.translate("settings.tab.about", "About")
        }

        StyledTabButton {
            id: debugTabButton
            visible: Settings.isDebugBuild
            text: TranslationManager.translate("settings.tab.debug", "Debug")
            tabLabel: TranslationManager.translate("settings.tab.debug", "Debug")
            width: visible ? implicitWidth : 0
        }
    }

    // Tab content area - all tabs preload in background
    StackLayout {
        id: tabContent
        anchors.top: tabBar.bottom
        anchors.topMargin: Theme.spacingMedium
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomBar.top
        anchors.bottomMargin: Theme.spacingMedium
        anchors.leftMargin: Theme.standardMargin
        anchors.rightMargin: Theme.standardMargin

        currentIndex: tabBar.currentIndex

        // Tab 0: Bluetooth - loads synchronously (first tab appears instantly)
        Loader {
            id: bluetoothLoader
            active: true
            asynchronous: false
            source: "settings/SettingsBluetoothTab.qml"
        }

        // Tab 1: Preferences - preloads async in background
        Loader {
            id: preferencesLoader
            active: true
            asynchronous: true
            source: "settings/SettingsPreferencesTab.qml"
        }

        // Tab 2: Options - preloads async in background
        Loader {
            id: optionsLoader
            active: true
            asynchronous: true
            source: "settings/SettingsOptionsTab.qml"
        }

        // Tab 3: Screensaver/Network - preloads async in background
        Loader {
            id: screensaverLoader
            active: true
            asynchronous: true
            source: "settings/SettingsScreensaverTab.qml"
        }

        // Tab 4: Visualizer - preloads async in background
        Loader {
            id: visualizerLoader
            active: true
            asynchronous: true
            source: "settings/SettingsVisualizerTab.qml"
        }

        // Tab 5: AI - preloads async in background
        Loader {
            id: aiLoader
            active: true
            asynchronous: true
            source: "settings/SettingsAITab.qml"
        }

        // Tab 6: Accessibility - preloads async in background
        Loader {
            id: accessibilityLoader
            active: true
            asynchronous: true
            source: "settings/SettingsAccessibilityTab.qml"
        }

        // Tab 7: Themes - preloads async in background
        Loader {
            id: themesLoader
            active: true
            asynchronous: true
            source: "settings/SettingsThemesTab.qml"
            onLoaded: {
                item.openSaveThemeDialog.connect(function() {
                    saveThemeDialog.open()
                })
            }
        }

        // Tab 8: Layout - preloads async in background
        Loader {
            id: layoutLoader
            active: true
            asynchronous: true
            source: "settings/SettingsLayoutTab.qml"
        }

        // Tab 9: Language - preloads async in background
        Loader {
            id: languageLoader
            active: true
            asynchronous: true
            source: "settings/SettingsLanguageTab.qml"
        }

        // Tab 9: History - preloads async in background
        Loader {
            id: historyLoader
            active: true
            asynchronous: true
            source: "settings/SettingsShotHistoryTab.qml"
        }

        // Tab 10: Data - preloads async in background
        Loader {
            id: dataLoader
            active: true
            asynchronous: true
            source: "settings/SettingsDataTab.qml"
        }

        // Tab 11: Home Automation - preloads async in background
        Loader {
            id: homeAutomationLoader
            active: true
            asynchronous: true
            source: "settings/SettingsHomeAutomationTab.qml"
        }

        // Tab 12: Update - preloads async in background (not on iOS - App Store handles updates)
        Loader {
            id: updateLoader
            active: MainController.updateChecker.canCheckForUpdates
            asynchronous: true
            source: "settings/SettingsUpdateTab.qml"
        }

        // Tab 12: About
        Loader {
            id: aboutLoader
            active: true
            asynchronous: true
            source: "settings/SettingsAboutTab.qml"
        }

        // Tab 13: Debug - only in debug builds
        Loader {
            id: debugLoader
            active: Settings.isDebugBuild
            asynchronous: true
            source: "settings/SettingsDebugTab.qml"
        }
    }

    // Save Theme Dialog
    Dialog {
        id: saveThemeDialog
        modal: true
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2 - keyboardOffset
        width: Theme.scaled(300)
        padding: 20

        property string themeName: ""
        property real keyboardOffset: 0

        Behavior on y {
            NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
        }

        Connections {
            target: Qt.inputMethod
            function onVisibleChanged() {
                if (Qt.inputMethod.visible && saveThemeDialog.visible) {
                    saveThemeDialog.keyboardOffset = parent.height * 0.25
                } else {
                    saveThemeDialog.keyboardOffset = 0
                }
            }
        }

        background: Rectangle {
            color: Theme.surfaceColor
            radius: Theme.cardRadius
            border.color: Theme.borderColor
            border.width: 1
        }

        onOpened: {
            themeName = ""
            themeNameInput.text = ""
            themeNameInput.forceActiveFocus()
        }

        onClosed: {
            keyboardOffset = 0
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacingMedium

            Tr {
                key: "settings.themes.saveTheme"
                fallback: "Save Theme"
                color: Theme.textColor
                font: Theme.subtitleFont
                Layout.alignment: Qt.AlignHCenter
            }

            TextField {
                id: themeNameInput
                Layout.fillWidth: true
                color: Theme.textColor
                placeholderTextColor: Theme.textSecondaryColor
                leftPadding: Theme.scaled(12)
                rightPadding: Theme.scaled(12)
                topPadding: Theme.scaled(12)
                bottomPadding: Theme.scaled(12)
                background: Rectangle {
                    color: Theme.backgroundColor
                    radius: Theme.buttonRadius
                    border.color: themeNameInput.activeFocus ? Theme.primaryColor : Theme.borderColor
                    border.width: 1
                }
                onTextChanged: saveThemeDialog.themeName = text
                onAccepted: {
                    if (saveThemeDialog.themeName.trim().length > 0) {
                        Settings.saveCurrentTheme(saveThemeDialog.themeName.trim())
                        if (themesLoader.item) themesLoader.item.refreshPresets()
                        saveThemeDialog.close()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                AccessibleButton {
                    Layout.fillWidth: true
                    text: TranslationManager.translate("common.cancel", "Cancel")
                    accessibleName: TranslationManager.translate("settingsPage.cancelSavingTheme", "Cancel saving theme")
                    onClicked: saveThemeDialog.close()
                }

                AccessibleButton {
                    Layout.fillWidth: true
                    primary: true
                    text: TranslationManager.translate("common.save", "Save")
                    accessibleName: TranslationManager.translate("settingsPage.saveThemeWithName", "Save current theme with entered name")
                    enabled: saveThemeDialog.themeName.trim().length > 0
                    onClicked: {
                        var name = saveThemeDialog.themeName.trim()
                        if (name.length > 0 && name !== "Default") {
                            Settings.saveCurrentTheme(name)
                            if (themesLoader.item) themesLoader.item.refreshPresets()
                            saveThemeDialog.close()
                        }
                    }
                }
            }
        }
    }

    // Bottom bar with back button
    BottomBar {
        id: bottomBar
        title: TranslationManager.translate("settings.title", "Settings")
        onBackClicked: root.goBack()
    }
}
