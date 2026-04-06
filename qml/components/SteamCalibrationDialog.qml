import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Decenza

Dialog {
    id: root

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    padding: 0
    topPadding: 0
    bottomPadding: 0
    closePolicy: Dialog.CloseOnEscape
    header: null
    width: Math.min(parent ? parent.width - Theme.scaled(40) : Theme.scaled(400), Theme.scaled(500))
    height: Math.min(dialogContent.implicitHeight + Theme.scaled(32),
                     parent ? parent.height - Theme.scaled(40) : Theme.scaled(600))

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.cardRadius
        border.color: Theme.borderColor
        border.width: 1
    }

    onOpened: {
        if (SteamCalibrator.state === 0 || SteamCalibrator.state === 5)
            SteamCalibrator.startCalibration()
    }

    onClosed: {
        if (SteamCalibrator.state !== 0 && SteamCalibrator.state !== 5)
            SteamCalibrator.cancelCalibration()
    }

    contentItem: Flickable {
        contentHeight: dialogContent.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: dialogContent
            width: parent.width
            spacing: Theme.spacingMedium

            // Title
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.scaled(16)
                Layout.bottomMargin: 0

                Text {
                    text: TranslationManager.translate("steamCal.title", "Steam Calibration")
                    color: Theme.textColor
                    font.family: Theme.bodyFont.family
                    font.pixelSize: Theme.scaled(18)
                    font.bold: true
                    Layout.fillWidth: true
                    Accessible.ignored: true
                }

                AccessibleButton {
                    text: "\u00D7"
                    accessibleName: TranslationManager.translate("common.accessibility.dismissDialog", "Close dialog")
                    flat: true
                    onClicked: root.close()
                }
            }

            // Phase label
            Text {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.scaled(16)
                Layout.rightMargin: Theme.scaled(16)
                visible: SteamCalibrator.state > 0 && SteamCalibrator.state < 5
                text: SteamCalibrator.phase === 0
                    ? TranslationManager.translate("steamCal.phase1", "Phase 1: Finding optimal flow rate")
                    : TranslationManager.translate("steamCal.phase2", "Phase 2: Optimizing temperature")
                color: Theme.primaryColor
                font.family: Theme.bodyFont.family
                font.pixelSize: Theme.scaled(13)
                font.bold: true
                Accessible.ignored: true
            }

            // Step indicator
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.scaled(16)
                Layout.rightMargin: Theme.scaled(16)
                visible: SteamCalibrator.state > 0 && SteamCalibrator.state < 5
                spacing: Theme.scaled(4)

                Repeater {
                    model: SteamCalibrator.totalSteps
                    Rectangle {
                        Layout.fillWidth: true
                        height: Theme.scaled(4)
                        radius: Theme.scaled(2)
                        color: index < SteamCalibrator.currentStep ? Theme.primaryColor
                             : index === SteamCalibrator.currentStep ? Theme.warningColor
                             : Theme.backgroundColor
                    }
                }
            }

            // Status message
            Text {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.scaled(16)
                Layout.rightMargin: Theme.scaled(16)
                text: SteamCalibrator.statusMessage
                color: Theme.textColor
                font.family: Theme.bodyFont.family
                font.pixelSize: Theme.scaled(14)
                wrapMode: Text.WordWrap
                Accessible.ignored: true
            }

            // Heater recovery indicator (visible when waiting and heater not ready)
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.scaled(16)
                Layout.rightMargin: Theme.scaled(16)
                visible: SteamCalibrator.state === 2 /* WaitingToStart */
                spacing: Theme.scaled(8)

                Text {
                    text: SteamCalibrator.heaterReady
                        ? TranslationManager.translate("steamCal.heaterReady", "Heater ready — start steaming now")
                        : TranslationManager.translate("steamCal.heaterRecovering", "Waiting for heater to recover...")
                    color: SteamCalibrator.heaterReady ? Theme.primaryColor : Theme.warningColor
                    font.family: Theme.bodyFont.family
                    font.pixelSize: Theme.scaled(14)
                    font.bold: true
                    Accessible.ignored: true
                }

                Text {
                    text: SteamCalibrator.currentHeaterTemp.toFixed(0) + "°C"
                    color: Theme.textSecondaryColor
                    font.family: Theme.bodyFont.family
                    font.pixelSize: Theme.scaled(14)
                    Accessible.ignored: true
                }
            }

            // Steaming progress (visible during Steaming state)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.scaled(16)
                Layout.rightMargin: Theme.scaled(16)
                visible: SteamCalibrator.state === 3 /* Steaming */
                spacing: Theme.scaled(8)

                // Countdown text
                Text {
                    Layout.fillWidth: true
                    text: {
                        var remaining = Math.max(0, 22 - SteamCalibrator.steamingElapsed)
                        if (SteamCalibrator.hasEnoughData)
                            return TranslationManager.translate("steamCal.stopping", "Stopping...")
                        else if (remaining > 0)
                            return TranslationManager.translate("steamCal.countdown", "Collecting data: %1s remaining")
                                .arg(Math.ceil(remaining))
                        else
                            return TranslationManager.translate("steamCal.stopping", "Stopping...")
                    }
                    color: SteamCalibrator.hasEnoughData ? Theme.primaryColor : Theme.textColor
                    font.family: Theme.bodyFont.family
                    font.pixelSize: Theme.scaled(16)
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    Accessible.ignored: true
                }

                // Progress bar
                Rectangle {
                    Layout.fillWidth: true
                    height: Theme.scaled(8)
                    radius: Theme.scaled(4)
                    color: Theme.backgroundColor

                    Rectangle {
                        width: parent.width * Math.min(1, SteamCalibrator.steamingElapsed / 22)
                        height: parent.height
                        radius: Theme.scaled(4)
                        color: SteamCalibrator.hasEnoughData ? Theme.primaryColor : Theme.warningColor

                        Behavior on width { NumberAnimation { duration: 200 } }
                    }
                }
            }

            // Instructions (visible during Instructions state)
            Text {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.scaled(16)
                Layout.rightMargin: Theme.scaled(16)
                visible: SteamCalibrator.state === 1
                text: TranslationManager.translate("steamCal.instructions",
                    "This tool finds the best steam flow rate and temperature for your machine by analyzing pressure stability and estimating water dilution.\n\n" +
                    "Phase 1: Tests different flow rates at your current temperature.\n" +
                    "Phase 2: Tests the best flow rates at different temperatures.\n\n" +
                    "Fill a pitcher with water (any amount). For each step, start steaming for at least 15 seconds.")
                color: Theme.textSecondaryColor
                font.family: Theme.bodyFont.family
                font.pixelSize: Theme.scaled(12)
                wrapMode: Text.WordWrap
                Accessible.ignored: true
            }

            // Results view (visible during Results state)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.scaled(16)
                Layout.rightMargin: Theme.scaled(16)
                visible: SteamCalibrator.state === 5
                spacing: Theme.scaled(8)

                // Recommendation banner
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: recLayout.implicitHeight + Theme.scaled(16)
                    color: Qt.rgba(Theme.primaryColor.r, Theme.primaryColor.g, Theme.primaryColor.b, 0.1)
                    radius: Theme.cardRadius
                    visible: SteamCalibrator.hasCalibration

                    ColumnLayout {
                        id: recLayout
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Theme.scaled(12)
                        spacing: Theme.scaled(4)

                        Text {
                            text: TranslationManager.translate("steamCal.recommended", "Recommended Settings")
                            color: Theme.primaryColor
                            font.family: Theme.bodyFont.family
                            font.pixelSize: Theme.scaled(14)
                            font.bold: true
                            Accessible.ignored: true
                        }

                        Text {
                            text: (SteamCalibrator.recommendedFlow / 100).toFixed(2) + " mL/s  ·  " +
                                  SteamCalibrator.recommendedTemp + "°C"
                            color: Theme.textColor
                            font.family: Theme.bodyFont.family
                            font.pixelSize: Theme.scaled(20)
                            font.bold: true
                            Accessible.ignored: true
                        }

                        Text {
                            text: TranslationManager.translate("steamCal.estDilution", "Estimated dilution") +
                                  ": ~" + SteamCalibrator.recommendedDilution.toFixed(1) + "%"
                            color: Theme.textSecondaryColor
                            font.family: Theme.bodyFont.family
                            font.pixelSize: Theme.scaled(12)
                            Accessible.ignored: true
                        }
                    }
                }

                // Results table header
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall

                    Text {
                        Layout.preferredWidth: Theme.scaled(55)
                        text: TranslationManager.translate("steamCal.flowHeader", "Flow")
                        color: Theme.textSecondaryColor
                        font.family: Theme.bodyFont.family
                        font.pixelSize: Theme.scaled(11)
                        font.bold: true
                        Accessible.ignored: true
                    }
                    Text {
                        Layout.preferredWidth: Theme.scaled(40)
                        text: TranslationManager.translate("steamCal.tempHeader", "Temp")
                        color: Theme.textSecondaryColor
                        font.family: Theme.bodyFont.family
                        font.pixelSize: Theme.scaled(11)
                        font.bold: true
                        Accessible.ignored: true
                    }
                    Text {
                        Layout.preferredWidth: Theme.scaled(55)
                        text: TranslationManager.translate("steamCal.dilutionHeader", "Dilution")
                        color: Theme.textSecondaryColor
                        font.family: Theme.bodyFont.family
                        font.pixelSize: Theme.scaled(11)
                        font.bold: true
                        Accessible.ignored: true
                    }
                    Text {
                        Layout.fillWidth: true
                        text: TranslationManager.translate("steamCal.stabilityHeader", "Stability")
                        color: Theme.textSecondaryColor
                        font.family: Theme.bodyFont.family
                        font.pixelSize: Theme.scaled(11)
                        font.bold: true
                        Accessible.ignored: true
                    }
                }

                // Result rows
                Repeater {
                    model: SteamCalibrator.results

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingSmall

                        readonly property bool isRecommended:
                            modelData.flowRate === SteamCalibrator.recommendedFlow &&
                            modelData.steamTemp === SteamCalibrator.recommendedTemp

                        Text {
                            Layout.preferredWidth: Theme.scaled(55)
                            text: (modelData.flowRate / 100).toFixed(2)
                            color: parent.isRecommended ? Theme.primaryColor : Theme.textColor
                            font.family: Theme.bodyFont.family
                            font.pixelSize: Theme.scaled(12)
                            font.bold: parent.isRecommended
                            Accessible.ignored: true
                        }
                        Text {
                            Layout.preferredWidth: Theme.scaled(40)
                            text: modelData.steamTemp + "°"
                            color: parent.isRecommended ? Theme.primaryColor : Theme.textColor
                            font.family: Theme.bodyFont.family
                            font.pixelSize: Theme.scaled(12)
                            Accessible.ignored: true
                        }
                        Text {
                            Layout.preferredWidth: Theme.scaled(55)
                            text: "~" + modelData.estimatedDilution.toFixed(1) + "%"
                            color: parent.isRecommended ? Theme.primaryColor : Theme.textColor
                            font.family: Theme.bodyFont.family
                            font.pixelSize: Theme.scaled(12)
                            Accessible.ignored: true
                        }

                        // Stability bar
                        Rectangle {
                            Layout.fillWidth: true
                            height: Theme.scaled(16)
                            radius: Theme.scaled(3)
                            color: Theme.backgroundColor

                            Rectangle {
                                width: parent.width * Math.min(1, modelData.stabilityScore / 100)
                                height: parent.height
                                radius: Theme.scaled(3)
                                color: modelData.stabilityScore >= 75 ? Theme.primaryColor
                                     : modelData.stabilityScore >= 50 ? Theme.warningColor
                                     : Theme.errorColor
                            }
                        }

                        Text {
                            visible: parent.isRecommended
                            text: "\u2713"
                            color: Theme.primaryColor
                            font.pixelSize: Theme.scaled(14)
                            font.bold: true
                            Accessible.ignored: true
                        }
                    }
                }
            }

            // Buttons
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.scaled(16)
                Layout.topMargin: Theme.spacingSmall
                spacing: Theme.spacingMedium

                Item { Layout.fillWidth: true }

                AccessibleButton {
                    visible: SteamCalibrator.state > 0 && SteamCalibrator.state < 5
                    text: TranslationManager.translate("common.button.cancel", "Cancel")
                    accessibleName: TranslationManager.translate("common.button.cancel", "Cancel")
                    onClicked: {
                        SteamCalibrator.cancelCalibration()
                        root.close()
                    }
                }

                AccessibleButton {
                    visible: SteamCalibrator.state === 1
                    text: TranslationManager.translate("steamCal.begin", "Begin")
                    accessibleName: TranslationManager.translate("steamCal.begin", "Begin")
                    primary: true
                    onClicked: SteamCalibrator.advanceToNextStep()
                }

                AccessibleButton {
                    visible: SteamCalibrator.state === 5 && SteamCalibrator.hasCalibration
                    text: TranslationManager.translate("steamCal.apply", "Apply Settings")
                    accessibleName: TranslationManager.translate("steamCal.applyAccessible",
                        "Apply recommended steam settings")
                    primary: true
                    onClicked: {
                        SteamCalibrator.applyRecommendation()
                        root.close()
                    }
                }

                AccessibleButton {
                    visible: SteamCalibrator.state === 5
                    text: TranslationManager.translate("common.button.close", "Close")
                    accessibleName: TranslationManager.translate("common.button.close", "Close")
                    onClicked: root.close()
                }
            }
        }
    }
}
