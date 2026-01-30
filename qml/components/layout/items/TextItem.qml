import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import DecenzaDE1
import "../.."

Item {
    id: root
    property bool isCompact: false
    property string itemId: ""
    property var modelData: ({})

    readonly property string content: modelData.content || "Text"
    readonly property string textAlign: modelData.align || "center"
    readonly property string action: modelData.action || ""
    readonly property bool hasAction: action !== ""

    readonly property int qtAlignment: {
        switch (textAlign) {
            case "left": return Text.AlignLeft
            case "right": return Text.AlignRight
            default: return Text.AlignHCenter
        }
    }

    implicitWidth: isCompact ? compactContent.implicitWidth : fullContent.implicitWidth
    implicitHeight: isCompact ? compactContent.implicitHeight : fullContent.implicitHeight

    // Trigger counter to force re-evaluation of resolvedText
    property int _refreshTick: 0

    // Variable substitution - explicit dependency references so QML tracks changes
    readonly property string resolvedText: {
        // Reference dependencies to establish QML bindings
        var _tick = _refreshTick
        var _c = content
        if (typeof DE1Device !== "undefined") {
            void(DE1Device.temperature); void(DE1Device.steamTemperature)
            void(DE1Device.pressure); void(DE1Device.flow)
            void(DE1Device.waterLevel); void(DE1Device.waterLevelMl)
            void(DE1Device.stateString)
        }
        if (typeof MachineState !== "undefined") {
            void(MachineState.scaleWeight); void(MachineState.shotTime)
            void(MachineState.cumulativeVolume)
        }
        if (typeof MainController !== "undefined") {
            void(MainController.targetWeight); void(MainController.currentProfileName)
            void(MainController.profileTargetTemperature)
            void(MainController.brewByRatio); void(MainController.brewByRatioDose)
        }
        if (typeof DE1Device !== "undefined") void(DE1Device.connected)
        if (typeof ScaleDevice !== "undefined") { void(ScaleDevice.name); void(ScaleDevice.connected) }
        return substituteVariables(_c)
    }

    // Timer to update time/date variables (and any other periodic refresh)
    Timer {
        id: refreshTimer
        interval: 1000
        running: content.indexOf("%TIME%") >= 0 || content.indexOf("%DATE%") >= 0
        repeat: true
        onTriggered: root._refreshTick++
    }

    function substituteVariables(text) {
        if (!text) return ""
        var result = text
        // Machine
        result = result.replace(/%TEMP%/g, typeof DE1Device !== "undefined" ? DE1Device.temperature.toFixed(1) : "—")
        result = result.replace(/%STEAM_TEMP%/g, typeof DE1Device !== "undefined" ? DE1Device.steamTemperature.toFixed(1) : "—")
        result = result.replace(/%PRESSURE%/g, typeof DE1Device !== "undefined" ? DE1Device.pressure.toFixed(1) : "—")
        result = result.replace(/%FLOW%/g, typeof DE1Device !== "undefined" ? DE1Device.flow.toFixed(1) : "—")
        result = result.replace(/%WATER%/g, typeof DE1Device !== "undefined" ? DE1Device.waterLevel.toFixed(0) : "—")
        result = result.replace(/%WATER_ML%/g, typeof DE1Device !== "undefined" ? DE1Device.waterLevelMl.toFixed(0) : "—")
        result = result.replace(/%STATE%/g, typeof DE1Device !== "undefined" ? DE1Device.stateString : "—")
        // Scale / Shot
        result = result.replace(/%WEIGHT%/g, typeof MachineState !== "undefined" ? MachineState.scaleWeight.toFixed(1) : "—")
        result = result.replace(/%SHOT_TIME%/g, typeof MachineState !== "undefined" ? MachineState.shotTime.toFixed(1) : "—")
        result = result.replace(/%VOLUME%/g, typeof MachineState !== "undefined" ? MachineState.cumulativeVolume.toFixed(0) : "—")
        // Controller
        result = result.replace(/%TARGET_WEIGHT%/g, typeof MainController !== "undefined" ? MainController.targetWeight.toFixed(1) : "—")
        result = result.replace(/%PROFILE%/g, typeof MainController !== "undefined" ? MainController.currentProfileName : "—")
        result = result.replace(/%TARGET_TEMP%/g, typeof MainController !== "undefined" ? MainController.profileTargetTemperature.toFixed(1) : "—")
        result = result.replace(/%RATIO%/g, typeof MainController !== "undefined" ? MainController.brewByRatio.toFixed(1) : "—")
        result = result.replace(/%DOSE%/g, typeof MainController !== "undefined" ? MainController.brewByRatioDose.toFixed(1) : "—")
        // Scale device
        result = result.replace(/%SCALE%/g, typeof ScaleDevice !== "undefined" ? ScaleDevice.name : "—")
        // Connection status
        var machineOn = typeof DE1Device !== "undefined" && DE1Device.connected
        var scaleOn = typeof ScaleDevice !== "undefined" && ScaleDevice.connected
        var flowScale = typeof ScaleDevice !== "undefined" && ScaleDevice.name === "Flow Scale"
        result = result.replace(/%CONNECTED%/g, machineOn ? "Online" : "Offline")
        result = result.replace(/%CONNECTED_COLOR%/g, machineOn ? Theme.successColor : Theme.errorColor)
        if (machineOn && scaleOn && !flowScale)
            result = result.replace(/%DEVICES%/g, "Machine + Scale")
        else if (machineOn && flowScale)
            result = result.replace(/%DEVICES%/g, "Machine + Simulated Scale")
        else
            result = result.replace(/%DEVICES%/g, "Machine")
        // Time
        var now = new Date()
        result = result.replace(/%TIME%/g, Qt.formatTime(now, "hh:mm"))
        result = result.replace(/%DATE%/g, Qt.formatDate(now, "yyyy-MM-dd"))
        return result
    }

    function executeAction() {
        if (!hasAction) return
        var parts = action.split(":")
        if (parts.length < 2) return
        var category = parts[0]
        var target = parts.slice(1).join(":")

        if (category === "navigate") {
            var pageMap = {
                "settings": "SettingsPage.qml",
                "history": "ShotHistoryPage.qml",
                "profiles": "ProfileSelectorPage.qml",
                "profileEditor": "ProfileEditorPage.qml",
                "recipes": "RecipeEditorPage.qml",
                "descaling": "DescalingPage.qml",
                "ai": "AISettingsPage.qml",
                "visualizer": "VisualizerBrowserPage.qml"
            }
            var page = pageMap[target]
            if (page && typeof pageStack !== "undefined") {
                pageStack.push(Qt.resolvedUrl("../../../pages/" + page))
            }
        } else if (category === "command") {
            switch (target) {
                case "sleep":
                    if (typeof DE1Device !== "undefined" && DE1Device.guiEnabled) {
                        DE1Device.goToSleep()
                        var win = Window.window
                        if (win && typeof win.goToScreensaver === "function")
                            win.goToScreensaver()
                    }
                    break
                case "startEspresso":
                    if (typeof DE1Device !== "undefined" && DE1Device.guiEnabled)
                        DE1Device.startEspresso()
                    break
                case "startSteam":
                    if (typeof DE1Device !== "undefined" && DE1Device.guiEnabled)
                        DE1Device.startSteam()
                    break
                case "startHotWater":
                    if (typeof DE1Device !== "undefined" && DE1Device.guiEnabled)
                        DE1Device.startHotWater()
                    break
                case "startFlush":
                    if (typeof DE1Device !== "undefined" && DE1Device.guiEnabled)
                        DE1Device.startFlush()
                    break
                case "idle":
                    if (typeof DE1Device !== "undefined")
                        DE1Device.requestIdle()
                    break
                case "tare":
                    if (typeof MachineState !== "undefined")
                        MachineState.tareScale()
                    break
            }
        }
    }

    // --- COMPACT MODE (bar rendering) ---
    Item {
        id: compactContent
        visible: root.isCompact
        anchors.fill: parent
        implicitWidth: compactText.implicitWidth + (root.hasAction ? Theme.scaled(16) : 0)
        implicitHeight: Theme.bottomBarHeight

        Rectangle {
            visible: root.hasAction
            anchors.fill: parent
            anchors.topMargin: Theme.spacingSmall
            anchors.bottomMargin: Theme.spacingSmall
            color: compactTapArea.pressed
                ? Qt.darker(Theme.surfaceColor, 1.3)
                : Theme.surfaceColor
            radius: Theme.cardRadius
            border.color: Theme.borderColor
            border.width: 1
        }

        Text {
            id: compactText
            anchors.centerIn: parent
            text: root.resolvedText
            textFormat: Text.RichText
            color: Theme.textColor
            font: Theme.bodyFont
            horizontalAlignment: root.qtAlignment
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        MouseArea {
            id: compactTapArea
            anchors.fill: parent
            visible: root.hasAction
            onClicked: root.executeAction()
        }
    }

    // --- FULL MODE (center rendering) ---
    Item {
        id: fullContent
        visible: !root.isCompact
        anchors.fill: parent
        implicitWidth: fullText.implicitWidth + (root.hasAction ? Theme.scaled(24) : 0)
        implicitHeight: fullText.implicitHeight + (root.hasAction ? Theme.scaled(16) : 0)

        Rectangle {
            visible: root.hasAction
            anchors.fill: parent
            color: fullTapArea.pressed
                ? Qt.darker(Theme.surfaceColor, 1.3)
                : Theme.surfaceColor
            radius: Theme.cardRadius
            border.color: Theme.borderColor
            border.width: 1
        }

        Text {
            id: fullText
            anchors.centerIn: parent
            width: parent.width > 0 ? parent.width - (root.hasAction ? Theme.scaled(24) : 0) : implicitWidth
            text: root.resolvedText
            textFormat: Text.RichText
            color: Theme.textColor
            font: Theme.bodyFont
            horizontalAlignment: root.qtAlignment
            wrapMode: Text.Wrap
        }

        MouseArea {
            id: fullTapArea
            anchors.fill: parent
            visible: root.hasAction
            onClicked: root.executeAction()
        }
    }
}
