import QtQuick
import QtQuick.Layouts
import DecenzaDE1
import "../.."

Item {
    id: root
    property bool isCompact: false
    property string itemId: ""

    implicitWidth: isCompact ? compactContent.implicitWidth : fullContent.implicitWidth
    implicitHeight: isCompact ? compactContent.implicitHeight : fullContent.implicitHeight

    // --- COMPACT MODE ---
    Item {
        id: compactContent
        visible: root.isCompact
        anchors.fill: parent
        implicitWidth: compactRow.implicitWidth
        implicitHeight: compactRow.implicitHeight

        Row {
            id: compactRow
            anchors.centerIn: parent
            spacing: Theme.spacingSmall

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.scaled(10)
                height: Theme.scaled(10)
                radius: Theme.scaled(5)
                color: DE1Device.connected ? Theme.successColor : Theme.errorColor
            }

            Tr {
                key: DE1Device.connected ? "statusbar.online" : "statusbar.offline"
                fallback: DE1Device.connected ? "Online" : "Offline"
                color: DE1Device.connected ? Theme.successColor : Theme.textSecondaryColor
                font: Theme.bodyFont
            }
        }
    }

    // --- FULL MODE ---
    Item {
        id: fullContent
        visible: !root.isCompact
        anchors.fill: parent
        implicitWidth: connectionIndicator.implicitWidth
        implicitHeight: connectionIndicator.implicitHeight

        ConnectionIndicator {
            id: connectionIndicator
            anchors.centerIn: parent
            machineConnected: DE1Device.connected
            scaleConnected: ScaleDevice && ScaleDevice.connected
            isFlowScale: ScaleDevice && ScaleDevice.name === "Flow Scale"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (typeof AccessibilityManager !== "undefined" && AccessibilityManager.enabled) {
                    var status = DE1Device.connected ? "Machine connected" : "Machine disconnected"
                    if (ScaleDevice && ScaleDevice.connected) {
                        if (ScaleDevice.name === "Flow Scale") {
                            status += ". Using simulated scale from flow sensor"
                        } else {
                            status += ". Scale connected: " + ScaleDevice.name
                        }
                    } else {
                        status += ". No scale connected"
                    }
                    AccessibilityManager.announceLabel(status)
                }
            }
        }
    }
}
