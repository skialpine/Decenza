import QtQuick
import QtCharts
import DecenzaDE1

ChartView {
    id: chart
    antialiasing: true
    backgroundColor: "transparent"
    plotAreaColor: Qt.darker(Theme.surfaceColor, 1.3)
    legend.visible: false

    margins.top: 0
    margins.bottom: 0
    margins.left: 0
    margins.right: 0

    // Shot comparison model
    property var comparisonModel: null

    // Visibility toggles for curve types
    property bool showPressure: true
    property bool showFlow: true
    property bool showWeight: true
    property bool showWeightFlow: true

    // Colors for each shot (primary, flow/light, weight/lighter)
    readonly property var shotColorSets: [
        { primary: "#4CAF50", flow: "#81C784", weight: "#A5D6A7" },  // Green
        { primary: "#2196F3", flow: "#64B5F6", weight: "#90CAF9" },  // Blue
        { primary: "#FF9800", flow: "#FFB74D", weight: "#FFCC80" }   // Orange
    ]

    // Load data from comparison model
    function loadData() {
        if (!comparisonModel) return

        // Clear all series
        pressure1.clear(); flow1.clear(); weight1.clear(); weightFlow1.clear()
        pressure2.clear(); flow2.clear(); weight2.clear(); weightFlow2.clear()
        pressure3.clear(); flow3.clear(); weight3.clear(); weightFlow3.clear()

        var windowStart = comparisonModel.windowStart

        // Load each shot's data with colors based on global position
        for (var i = 0; i < comparisonModel.shotCount; i++) {
            var pressureData = comparisonModel.getPressureData(i)
            var flowData = comparisonModel.getFlowData(i)
            var weightData = comparisonModel.getWeightData(i)
            var weightFlowData = comparisonModel.getWeightFlowRateData(i)

            var pSeries = i === 0 ? pressure1 : (i === 1 ? pressure2 : pressure3)
            var fSeries = i === 0 ? flow1 : (i === 1 ? flow2 : flow3)
            var wSeries = i === 0 ? weight1 : (i === 1 ? weight2 : weight3)
            var wfSeries = i === 0 ? weightFlow1 : (i === 1 ? weightFlow2 : weightFlow3)

            // Set colors based on global position (windowStart + i) % 3
            var colorIndex = (windowStart + i) % 3
            var colors = shotColorSets[colorIndex]
            pSeries.color = colors.primary
            fSeries.color = colors.flow
            wSeries.color = colors.weight
            wfSeries.color = Qt.lighter(colors.weight, 1.3)

            for (var j = 0; j < pressureData.length; j++) {
                pSeries.append(pressureData[j].x, pressureData[j].y)
            }
            for (j = 0; j < flowData.length; j++) {
                fSeries.append(flowData[j].x, flowData[j].y)
            }
            for (j = 0; j < weightData.length; j++) {
                wSeries.append(weightData[j].x, weightData[j].y / 5)  // Scale for display
            }
            for (j = 0; j < weightFlowData.length; j++) {
                wfSeries.append(weightFlowData[j].x, weightFlowData[j].y)
            }
        }

        // Update axes - fit to data with small padding (minimum 15s for very short shots)
        timeAxis.max = Math.max(15, comparisonModel.maxTime + 0.5)
    }

    // Find the Y value in a LineSeries closest to the given time
    function findValueAtTime(series, time) {
        if (series.count === 0) return null
        var closest = series.at(0)
        var minDist = Math.abs(closest.x - time)
        for (var i = 1; i < series.count; i++) {
            var p = series.at(i)
            var dist = Math.abs(p.x - time)
            if (dist < minDist) {
                closest = p
                minDist = dist
            } else if (dist > minDist) {
                break  // sorted by x, so we passed the closest
            }
        }
        // Only return if within 1 second of the requested time
        return minDist < 1.0 ? closest.y : null
    }

    // Announce curve values at a pixel position (called on tap)
    function announceAtPosition(pixelX, pixelY) {
        var dataPoint = chart.mapToValue(Qt.point(pixelX, pixelY), pressure1)
        var time = dataPoint.x
        if (time < 0 || time > timeAxis.max) return

        var curveTypes = []
        if (showPressure) curveTypes.push({ name: "Pressure", series: [pressure1, pressure2, pressure3] })
        if (showFlow) curveTypes.push({ name: "Flow", series: [flow1, flow2, flow3] })
        if (showWeightFlow) curveTypes.push({ name: "Weight flow", series: [weightFlow1, weightFlow2, weightFlow3] })
        if (showWeight) curveTypes.push({ name: "Weight", series: [weight1, weight2, weight3], scale: 5 })

        var groups = []
        for (var c = 0; c < curveTypes.length; c++) {
            var ct = curveTypes[c]
            var values = []
            for (var s = 0; s < ct.series.length; s++) {
                var v = findValueAtTime(ct.series[s], time)
                if (v !== null) values.push((ct.scale ? v * ct.scale : v).toFixed(1))
            }
            if (values.length > 0) groups.push({ name: ct.name, text: values.join(", ") })
        }

        if (groups.length === 0) return
        // One curve type: just time + values. Multiple: prefix each with name.
        var body = groups.length === 1
            ? groups[0].text
            : groups.map(function(g) { return g.name + " " + g.text }).join(". ")

        if (typeof AccessibilityManager !== "undefined") {
            AccessibilityManager.announce(time.toFixed(1) + ". " + body, true)
        }
    }

    onComparisonModelChanged: loadData()
    Component.onCompleted: loadData()

    // Connect to model changes
    Connections {
        target: comparisonModel
        function onShotsChanged() { loadData() }
    }

    // Time axis
    ValueAxis {
        id: timeAxis
        min: 0
        max: 60
        tickCount: 7
        labelFormat: "%.0f"
        labelsColor: Theme.textSecondaryColor
        gridLineColor: Qt.rgba(255, 255, 255, 0.1)
        titleText: "Time (s)"
        titleBrush: Theme.textSecondaryColor
    }

    // Pressure/Flow axis (left Y)
    ValueAxis {
        id: pressureAxis
        min: 0
        max: 12
        tickCount: 5
        labelFormat: "%.0f"
        labelsColor: Theme.textSecondaryColor
        gridLineColor: Qt.rgba(255, 255, 255, 0.1)
        titleText: "bar / mL/s"
        titleBrush: Theme.textSecondaryColor
    }

    // Weight axis (hidden)
    ValueAxis {
        id: weightAxis
        min: 0
        max: 12
        visible: false
    }

    // Shot 1 series (Green)
    LineSeries {
        id: pressure1
        name: "Pressure 1"
        color: "#4CAF50"
        width: Theme.graphLineWidth
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showPressure
    }
    LineSeries {
        id: flow1
        name: "Flow 1"
        color: "#81C784"
        width: Theme.graphLineWidth
        style: Qt.DashLine
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showFlow
    }
    LineSeries {
        id: weight1
        name: "Weight 1"
        color: "#A5D6A7"
        width: Math.max(1, Theme.graphLineWidth - 1)
        style: Qt.DotLine
        axisX: timeAxis
        axisY: weightAxis
        visible: chart.showWeight
    }
    LineSeries {
        id: weightFlow1
        name: "Weight Flow 1"
        color: Qt.lighter("#A5D6A7", 1.3)
        width: Math.max(1, Theme.graphLineWidth - 1)
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showWeightFlow
    }

    // Shot 2 series (Blue)
    LineSeries {
        id: pressure2
        name: "Pressure 2"
        color: "#2196F3"
        width: Theme.graphLineWidth
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showPressure
    }
    LineSeries {
        id: flow2
        name: "Flow 2"
        color: "#64B5F6"
        width: Theme.graphLineWidth
        style: Qt.DashLine
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showFlow
    }
    LineSeries {
        id: weight2
        name: "Weight 2"
        color: "#90CAF9"
        width: Math.max(1, Theme.graphLineWidth - 1)
        style: Qt.DotLine
        axisX: timeAxis
        axisY: weightAxis
        visible: chart.showWeight
    }
    LineSeries {
        id: weightFlow2
        name: "Weight Flow 2"
        color: Qt.lighter("#90CAF9", 1.3)
        width: Math.max(1, Theme.graphLineWidth - 1)
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showWeightFlow
    }

    // Shot 3 series (Orange)
    LineSeries {
        id: pressure3
        name: "Pressure 3"
        color: "#FF9800"
        width: Theme.graphLineWidth
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showPressure
    }
    LineSeries {
        id: flow3
        name: "Flow 3"
        color: "#FFB74D"
        width: Theme.graphLineWidth
        style: Qt.DashLine
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showFlow
    }
    LineSeries {
        id: weight3
        name: "Weight 3"
        color: "#FFCC80"
        width: Math.max(1, Theme.graphLineWidth - 1)
        style: Qt.DotLine
        axisX: timeAxis
        axisY: weightAxis
        visible: chart.showWeight
    }
    LineSeries {
        id: weightFlow3
        name: "Weight Flow 3"
        color: Qt.lighter("#FFCC80", 1.3)
        width: Math.max(1, Theme.graphLineWidth - 1)
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showWeightFlow
    }
}
