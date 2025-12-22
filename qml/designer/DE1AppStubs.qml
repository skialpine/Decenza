// Design-time stubs for C++ backend types
// These provide placeholder values for Qt Design Studio

import QtQuick

QtObject {
    id: backend

    // DE1Device stub
    property QtObject de1Device: QtObject {
        property bool connected: true
        property double pressure: 9.0
        property double flow: 2.5
        property double temperature: 93.0
        property string firmwareVersion: "1.0.0"
        property int state: 0
        function stopOperation() {}
        function connectToDevice(addr) {}
    }

    // ScaleDevice stub
    property QtObject scaleDevice: QtObject {
        property bool connected: true
        property double weight: 36.0
        property double flowRate: 2.0
        function tare() {}
    }

    // MachineState stub
    property QtObject machineState: QtObject {
        property int phase: 0
        property double shotTime: 25.0
        property bool isFlowing: false
    }

    // MainController stub
    property QtObject mainController: QtObject {
        property string currentProfileName: "Default Profile"
        property double targetWeight: 36.0
        property var availableProfiles: ["Default", "Blooming", "Turbo"]
        function loadProfile(name) {}
        function applySteamSettings() {}
        function setSteamFlowImmediate(flow) {}
        function setSteamTemperatureImmediate(temp) {}
    }

    // BLEManager stub
    property QtObject bleManager: QtObject {
        property bool scanning: false
        property var discoveredDevices: []
        property var discoveredScales: []
        function startScan() {}
        function stopScan() {}
    }

    // Settings stub
    property QtObject settings: QtObject {
        property int steamTimeout: 30
        property int steamFlow: 700
        property int steamTemperature: 160
        property int selectedSteamCup: 0
        property var steamCupPresets: [
            { name: "Small", duration: 20, flow: 600 },
            { name: "Medium", duration: 30, flow: 700 },
            { name: "Large", duration: 45, flow: 800 }
        ]
        function getSteamCupPreset(idx) { return steamCupPresets[idx] }
        function updateSteamCupPreset(idx, name, dur, flow) {}
        function addSteamCupPreset(name, dur, flow) {}
        function removeSteamCupPreset(idx) {}
        function moveSteamCupPreset(from, to) {}
    }

    // ShotDataModel stub
    property QtObject shotDataModel: QtObject {
        property var pressureData: []
        property var flowData: []
        property var temperatureData: []
        property var weightData: []
        property var pressureGoalData: []
        property var flowGoalData: []
        property var temperatureGoalData: []
        property var phaseMarkers: []
        property double extractionStartTime: -1
    }
}
