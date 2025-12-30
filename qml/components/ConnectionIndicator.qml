import QtQuick
import QtQuick.Layouts
import DecenzaDE1

ColumnLayout {
    property bool machineConnected: false
    property bool scaleConnected: false
    property bool isFlowScale: false

    spacing: Theme.spacingSmall

    // Connection status (Online/Offline)
    Item {
        Layout.alignment: Qt.AlignHCenter
        implicitWidth: machineConnected ? onlineText.implicitWidth : offlineText.implicitWidth
        implicitHeight: machineConnected ? onlineText.implicitHeight : offlineText.implicitHeight

        Tr {
            id: onlineText
            key: "connection.online"
            fallback: "Online"
            visible: machineConnected
            color: Theme.successColor
            font: Theme.valueFont
        }

        Tr {
            id: offlineText
            key: "connection.offline"
            fallback: "Offline"
            visible: !machineConnected
            color: Theme.errorColor
            font: Theme.valueFont
        }
    }

    // Connection details (Machine, Machine + Scale, etc.)
    Item {
        Layout.alignment: Qt.AlignHCenter
        implicitWidth: Math.max(machineOnlyText.visible ? machineOnlyText.implicitWidth : 0,
                                machineScaleText.visible ? machineScaleText.implicitWidth : 0,
                                machineSimulatedText.visible ? machineSimulatedText.implicitWidth : 0)
        implicitHeight: machineOnlyText.implicitHeight

        Tr {
            id: machineOnlyText
            key: "connection.machine"
            fallback: "Machine"
            visible: !machineConnected || (machineConnected && !scaleConnected && !isFlowScale)
            color: Theme.textSecondaryColor
            font: Theme.labelFont
        }

        Tr {
            id: machineScaleText
            key: "connection.machineScale"
            fallback: "Machine + Scale"
            visible: machineConnected && scaleConnected && !isFlowScale
            color: Theme.textSecondaryColor
            font: Theme.labelFont
        }

        Tr {
            id: machineSimulatedText
            key: "connection.machineSimulatedScale"
            fallback: "Machine + Simulated Scale"
            visible: machineConnected && isFlowScale
            color: Theme.textSecondaryColor
            font: Theme.labelFont
        }
    }
}
