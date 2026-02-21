import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DecenzaDE1
import "../../components"

Item {
    id: themesTab

    // Keep signal so SettingsPage.qml connection doesn't break
    signal openSaveThemeDialog()
    function refreshPresets() {}

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingMedium
        spacing: Theme.spacingLarge

        // Title
        Text {
            text: TranslationManager.translate("settings.themes.title", "Theme Editor")
            color: Theme.textColor
            font: Theme.subtitleFont
        }

        // Explanation card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: infoColumn.height + Theme.scaled(32)
            color: Theme.surfaceColor
            radius: Theme.cardRadius
            border.color: Theme.borderColor
            border.width: 1

            ColumnLayout {
                id: infoColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.scaled(16)
                spacing: Theme.scaled(12)

                Text {
                    text: TranslationManager.translate("settings.themes.useWeb",
                        "The theme editor is available through the web interface, which provides a full color picker, " +
                        "CRT shader controls, and live preview.")
                    color: Theme.textColor
                    font: Theme.bodyFont
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                }

                // Server toggle row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.scaled(8)

                    Text {
                        text: TranslationManager.translate("settings.themes.webServer", "Web Server")
                        color: Theme.textColor
                        font: Theme.bodyFont
                    }

                    StyledSwitch {
                        checked: Settings.shotServerEnabled
                        accessibleName: TranslationManager.translate("settings.themes.enableServer", "Enable web server")
                        onToggled: Settings.shotServerEnabled = checked
                    }

                    // Status indicator
                    Rectangle {
                        visible: Settings.shotServerEnabled
                        width: Theme.scaled(8)
                        height: Theme.scaled(8)
                        radius: Theme.scaled(4)
                        color: MainController.shotServer && MainController.shotServer.running ?
                               Theme.successColor : Theme.errorColor
                    }

                    Text {
                        visible: Settings.shotServerEnabled
                        text: MainController.shotServer && MainController.shotServer.running ?
                              TranslationManager.translate("settings.themes.running", "Running") :
                              TranslationManager.translate("settings.themes.starting", "Starting...")
                        color: MainController.shotServer && MainController.shotServer.running ?
                               Theme.successColor : Theme.textSecondaryColor
                        font: Theme.captionFont
                    }

                    Item { Layout.fillWidth: true }
                }

                // Server URL (clickable)
                Rectangle {
                    visible: Settings.shotServerEnabled && MainController.shotServer && MainController.shotServer.running
                    Layout.fillWidth: true
                    height: Theme.scaled(40)
                    color: Theme.backgroundColor
                    radius: Theme.scaled(6)

                    Text {
                        anchors.centerIn: parent
                        text: MainController.shotServer ? MainController.shotServer.url + "/themes" : ""
                        color: Theme.primaryColor
                        font.family: Theme.bodyFont.family
                        font.pixelSize: Theme.bodyFont.pixelSize
                        font.bold: true
                        font.underline: true

                        TapHandler {
                            onTapped: {
                                if (MainController.shotServer)
                                    Qt.openUrlExternally(MainController.shotServer.url + "/themes")
                            }
                        }
                    }
                }

                // Instructions
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.borderColor
                }

                Text {
                    text: TranslationManager.translate("settings.themes.instructions",
                        "How to connect:\n" +
                        "1. Enable the web server above\n" +
                        "2. Open a browser on any device connected to the same WiFi network\n" +
                        "3. Navigate to the URL shown above (the /themes page)\n" +
                        "4. Changes apply to the device in real-time")
                    color: Theme.textSecondaryColor
                    font: Theme.captionFont
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    lineHeight: 1.4
                }
            }
        }

        // Preset themes (keep this - quick way to switch themes on-device)
        Text {
            text: TranslationManager.translate("settings.themes.presets", "Quick Presets")
            color: Theme.textColor
            font: Theme.subtitleFont
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.scaled(44)
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff
            contentHeight: availableHeight
            clip: true

            Row {
                height: parent.height
                spacing: Theme.spacingSmall

                Repeater {
                    id: presetRepeater
                    model: Settings.getPresetThemes()

                    Rectangle {
                        height: Theme.scaled(36)
                        width: presetRow.width + (modelData.isBuiltIn ? 0 : deleteBtn.width + 4)
                        color: modelData.primaryColor
                        radius: Theme.buttonRadius
                        border.color: Settings.activeThemeName === modelData.name ? "white" : "transparent"
                        border.width: 2

                        Row {
                            id: presetRow
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            leftPadding: Theme.scaled(12)
                            rightPadding: modelData.isBuiltIn ? 12 : 4

                            Text {
                                text: modelData.name
                                color: "white"
                                font: Theme.labelFont
                                anchors.verticalCenter: parent.verticalCenter

                                MouseArea {
                                    anchors.fill: parent
                                    anchors.margins: -8
                                    onClicked: Settings.applyPresetTheme(modelData.name)
                                }
                            }
                        }

                        // Delete button for user themes
                        Rectangle {
                            id: deleteBtn
                            visible: !modelData.isBuiltIn
                            width: Theme.scaled(24)
                            height: Theme.scaled(24)
                            radius: Theme.scaled(12)
                            color: deleteArea.pressed ? Qt.darker(parent.color, 1.3) : Qt.darker(parent.color, 1.15)
                            anchors.right: parent.right
                            anchors.rightMargin: Theme.scaled(6)
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: "x"
                                color: "white"
                                font.pixelSize: Theme.scaled(12)
                                font.bold: true
                                anchors.centerIn: parent
                            }

                            MouseArea {
                                id: deleteArea
                                anchors.fill: parent
                                onClicked: {
                                    Settings.deleteUserTheme(modelData.name)
                                    presetRepeater.model = Settings.getPresetThemes()
                                }
                            }
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
