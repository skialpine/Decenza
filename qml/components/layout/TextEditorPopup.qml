import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DecenzaDE1
import ".."

Popup {
    id: popup

    property string itemId: ""
    property string zoneName: ""
    property string pageContext: "idle"

    // Current values (set before opening)
    property string textContent: "Text"
    property string textAlign: "center"
    property string textAction: ""
    property bool aiPending: false

    signal saved()

    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: Theme.spacingSmall

    // Wide popup at top of window to avoid keyboard
    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Theme.spacingSmall
    width: parent.width - Theme.spacingSmall * 2
    height: Math.min(mainColumn.implicitHeight + padding * 2, parent.height * 0.6)

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.cardRadius
        border.color: Theme.borderColor
        border.width: 1
    }

    onClosed: aiPending = false

    function openForItem(id, zone, props) {
        itemId = id
        zoneName = zone
        textContent = props.content || "Text"
        textAlign = props.align || "center"
        textAction = props.action || ""
        aiPending = false
        contentInput.text = textContent
        open()
    }

    function doSave() {
        textContent = contentInput.text || "Text"
        Settings.setItemProperty(itemId, "content", textContent)
        Settings.setItemProperty(itemId, "align", textAlign)
        Settings.setItemProperty(itemId, "action", textAction)
        saved()
        close()
    }

    // --- Tag insertion helpers ---
    function insertTag(openTag, closeTag) {
        var start = contentInput.selectionStart
        var end = contentInput.selectionEnd
        var txt = contentInput.text
        if (start !== end) {
            var selected = txt.substring(start, end)
            contentInput.text = txt.substring(0, start) + openTag + selected + closeTag + txt.substring(end)
            contentInput.cursorPosition = end + openTag.length + closeTag.length
        } else {
            contentInput.text = txt.substring(0, start) + openTag + closeTag + txt.substring(start)
            contentInput.cursorPosition = start + openTag.length
        }
        contentInput.forceActiveFocus()
    }

    function insertBold()         { insertTag("<b>", "</b>") }
    function insertItalic()       { insertTag("<i>", "</i>") }
    function insertColor(color)   { insertTag('<span style="color:' + color + '">', '</span>') }
    function insertFontSize(size) { insertTag('<span style="font-size:' + size + 'px">', '</span>') }

    function insertVariable(token) {
        var pos = contentInput.cursorPosition
        var txt = contentInput.text
        contentInput.text = txt.substring(0, pos) + token + txt.substring(pos)
        contentInput.cursorPosition = pos + token.length
        contentInput.forceActiveFocus()
    }

    // AI Advice: listen for response while this popup is open
    Connections {
        target: MainController.aiManager || null
        enabled: popup.aiPending
        function onRecommendationReceived(recommendation) {
            if (!popup.aiPending) return
            popup.aiPending = false
            // Strip markdown code fences if the AI wrapped the output
            var html = recommendation.trim()
            if (html.startsWith("```")) {
                html = html.replace(/^```[a-z]*\n?/, "").replace(/\n?```$/, "").trim()
            }
            contentInput.text = html
        }
        function onErrorOccurred(error) {
            if (!popup.aiPending) return
            popup.aiPending = false
        }
    }

    function requestAiAdvice() {
        if (!MainController.aiManager || !MainController.aiManager.isConfigured) return
        if (MainController.aiManager.isAnalyzing) return

        var userText = contentInput.text.trim()
        if (!userText) return

        var systemPrompt =
            "You are an HTML generator for a text widget in a Decent Espresso machine controller app. " +
            "The widget renders a subset of HTML 4 (no CSS classes, no <div>, no JavaScript). " +
            "Supported tags: <b>, <i>, <u>, <span style=\"...\">, <br>. " +
            "Supported inline styles: color (named or hex), font-size (in px), font-weight. " +
            "Example: <span style=\"color:#e94560; font-size:48px; font-weight:bold\">Text</span>\n\n" +

            "The widget supports live variables that are replaced with real-time machine data. " +
            "Available variables (wrap in percent signs):\n" +
            "  %TEMP% - Group head temperature in °C (e.g. 92.3)\n" +
            "  %STEAM_TEMP% - Steam heater temperature in °C (e.g. 155.0)\n" +
            "  %PRESSURE% - Group pressure in bar (e.g. 9.0)\n" +
            "  %FLOW% - Water flow rate in ml/s (e.g. 2.1)\n" +
            "  %WATER% - Water tank level in % (e.g. 78)\n" +
            "  %WATER_ML% - Water tank level in ml (e.g. 850)\n" +
            "  %WEIGHT% - Current scale weight in g (e.g. 36.2)\n" +
            "  %SHOT_TIME% - Elapsed shot time in seconds (e.g. 28.5)\n" +
            "  %TARGET_WEIGHT% - Stop-at-weight target in g (e.g. 36.0)\n" +
            "  %VOLUME% - Cumulative volume poured in ml (e.g. 42)\n" +
            "  %PROFILE% - Active profile name (e.g. Adaptive v2)\n" +
            "  %STATE% - Current machine state (e.g. Idle, Pouring, Steaming)\n" +
            "  %TARGET_TEMP% - Profile target temperature in °C (e.g. 93.0)\n" +
            "  %SCALE% - Connected scale device name (e.g. Lunar)\n" +
            "  %TIME% - Current time HH:MM (e.g. 14:30)\n" +
            "  %DATE% - Current date YYYY-MM-DD (e.g. 2025-01-15)\n" +
            "  %RATIO% - Brew ratio (e.g. 2.0)\n" +
            "  %DOSE% - Dose weight in g (e.g. 18.0)\n" +
            "  %CONNECTED% - Machine connection status: 'Online' or 'Offline'\n" +
            "  %CONNECTED_COLOR% - Color for connection status (green when online, red when offline). Use inside style attributes: style=\"color:%CONNECTED_COLOR%\"\n" +
            "  %DEVICES% - Connected devices description: 'Machine', 'Machine + Scale', or 'Machine + Simulated Scale'\n\n" +

            "Use <br> for line breaks. Variables can be placed inside tags: <b>%TEMP%</b> renders temperature in bold. " +
            "Multiple styles can be combined in one span: <span style=\"color:red; font-size:28px\">text</span>. " +
            "Tags can be nested: <b><span style=\"color:#e94560\">bold red</span></b>.\n\n" +

            "RULES:\n" +
            "- Output ONLY the raw HTML content. No markdown, no code fences, no explanation.\n" +
            "- Do not use <html>, <head>, <body>, <div>, <p>, or <table> tags.\n" +
            "- Do not use CSS classes or external stylesheets.\n" +
            "- Keep it concise — this renders in a small widget on a touchscreen.\n" +
            "- Use appropriate font sizes: 11-12px for labels, 18px for normal, 28-48px for large readouts.\n" +
            "- Use colors that look good on a dark background (the app has a dark theme)."

        var userPrompt
        // Detect if the content already contains HTML
        var hasHtml = /<[a-z][\s\S]*>/i.test(userText) || /%[A-Z_]+%/.test(userText)
        if (hasHtml) {
            userPrompt = "Here is the current HTML content of my text widget:\n\n" +
                         userText + "\n\n" +
                         "Please modify it according to this request. If the request is unclear, " +
                         "interpret it as a modification to the existing content. " +
                         "Output only the updated HTML."
        } else {
            userPrompt = userText
        }

        popup.aiPending = true
        MainController.aiManager.analyze(systemPrompt, userPrompt)
    }

    // Variable substitution for preview
    function substitutePreview(text) {
        if (!text) return ""
        var result = text
        result = result.replace(/%TEMP%/g, "92.3")
        result = result.replace(/%STEAM_TEMP%/g, "155.0")
        result = result.replace(/%PRESSURE%/g, "9.0")
        result = result.replace(/%FLOW%/g, "2.1")
        result = result.replace(/%WATER%/g, "78")
        result = result.replace(/%WATER_ML%/g, "850")
        result = result.replace(/%STATE%/g, "Idle")
        result = result.replace(/%WEIGHT%/g, "36.2")
        result = result.replace(/%SHOT_TIME%/g, "28.5")
        result = result.replace(/%VOLUME%/g, "42")
        result = result.replace(/%TARGET_WEIGHT%/g, "36.0")
        result = result.replace(/%PROFILE%/g, "Adaptive v2")
        result = result.replace(/%TARGET_TEMP%/g, "93.0")
        result = result.replace(/%RATIO%/g, "2.0")
        result = result.replace(/%DOSE%/g, "18.0")
        result = result.replace(/%SCALE%/g, "Lunar")
        result = result.replace(/%CONNECTED%/g, "Online")
        result = result.replace(/%CONNECTED_COLOR%/g, Theme.successColor)
        result = result.replace(/%DEVICES%/g, "Machine + Scale")
        var now = new Date()
        result = result.replace(/%TIME%/g, Qt.formatTime(now, "hh:mm"))
        result = result.replace(/%DATE%/g, Qt.formatDate(now, "yyyy-MM-dd"))
        return result
    }

    contentItem: ColumnLayout {
        id: mainColumn
        spacing: Theme.scaled(4)

        // === ROW 1: Content input (full width, scrollable) ===
        Rectangle {
            Layout.fillWidth: true
            height: Theme.scaled(56)
            color: Theme.backgroundColor
            radius: Theme.scaled(6)
            border.color: contentInput.activeFocus ? Theme.primaryColor : Theme.borderColor
            border.width: 1

            Flickable {
                id: inputFlickable
                anchors.fill: parent
                anchors.margins: Theme.scaled(4)
                clip: true
                flickableDirection: Flickable.VerticalFlick
                boundsBehavior: Flickable.StopAtBounds

                TextArea.flickable: TextArea {
                    id: contentInput
                    text: popup.textContent
                    color: Theme.textColor
                    font.family: "monospace"
                    font.pixelSize: Theme.captionFont.pixelSize
                    placeholderText: "Enter text or HTML..."
                    placeholderTextColor: Theme.textSecondaryColor
                    wrapMode: Text.Wrap
                    background: null
                    topPadding: 0
                    bottomPadding: 0
                    leftPadding: 0
                    rightPadding: 0
                }

                ScrollBar.vertical: ScrollBar {
                    width: Theme.scaled(4)
                    policy: inputFlickable.contentHeight > inputFlickable.height
                            ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                }
            }
        }

        // === ROW 2: Three columns — Format/Color | Variables | Actions ===
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.scaled(6)

            // --- LEFT COLUMN: Format + Color ---
            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: Theme.scaled(180)
                spacing: Theme.scaled(4)

                // Format toolbar
                Text {
                    text: "Format"
                    color: Theme.textSecondaryColor
                    font: Theme.captionFont
                }

                RowLayout {
                    spacing: Theme.scaled(4)

                    // Bold
                    Rectangle {
                        width: Theme.scaled(30); height: Theme.scaled(30)
                        radius: Theme.scaled(4)
                        color: boldMa.pressed ? Theme.primaryColor : Theme.backgroundColor
                        border.color: Theme.borderColor; border.width: 1
                        Text {
                            anchors.centerIn: parent; text: "B"
                            color: Theme.textColor; font.bold: true
                            font.pixelSize: Theme.captionFont.pixelSize
                        }
                        MouseArea { id: boldMa; anchors.fill: parent; onClicked: popup.insertBold() }
                    }

                    // Italic
                    Rectangle {
                        width: Theme.scaled(30); height: Theme.scaled(30)
                        radius: Theme.scaled(4)
                        color: italicMa.pressed ? Theme.primaryColor : Theme.backgroundColor
                        border.color: Theme.borderColor; border.width: 1
                        Text {
                            anchors.centerIn: parent; text: "I"
                            color: Theme.textColor; font.italic: true
                            font.pixelSize: Theme.captionFont.pixelSize
                        }
                        MouseArea { id: italicMa; anchors.fill: parent; onClicked: popup.insertItalic() }
                    }

                    Rectangle { width: 1; height: Theme.scaled(20); color: Theme.borderColor }

                    // Font sizes
                    Repeater {
                        model: [
                            { label: "S", size: "12" },
                            { label: "M", size: "18" },
                            { label: "L", size: "28" },
                            { label: "XL", size: "48" }
                        ]
                        Rectangle {
                            width: Theme.scaled(30); height: Theme.scaled(30)
                            radius: Theme.scaled(4)
                            color: sizeMa.pressed ? Theme.primaryColor : Theme.backgroundColor
                            border.color: Theme.borderColor; border.width: 1
                            Text {
                                anchors.centerIn: parent; text: modelData.label
                                color: Theme.textColor; font: Theme.captionFont
                            }
                            MouseArea { id: sizeMa; anchors.fill: parent; onClicked: popup.insertFontSize(modelData.size) }
                        }
                    }
                }

                // Alignment
                RowLayout {
                    spacing: Theme.scaled(4)

                    Repeater {
                        model: [
                            { label: "\u25C0", align: "left" },
                            { label: "\u25CF", align: "center" },
                            { label: "\u25B6", align: "right" }
                        ]
                        Rectangle {
                            width: Theme.scaled(30); height: Theme.scaled(30)
                            radius: Theme.scaled(4)
                            color: popup.textAlign === modelData.align ? Theme.primaryColor : Theme.backgroundColor
                            border.color: Theme.borderColor; border.width: 1
                            Text {
                                anchors.centerIn: parent; text: modelData.label
                                color: popup.textAlign === modelData.align ? "white" : Theme.textColor
                                font.pixelSize: Theme.scaled(10)
                            }
                            MouseArea { anchors.fill: parent; onClicked: popup.textAlign = modelData.align }
                        }
                    }
                }

                // Color palette
                Text {
                    text: "Color"
                    color: Theme.textSecondaryColor
                    font: Theme.captionFont
                }

                Grid {
                    columns: 6
                    spacing: Theme.scaled(4)

                    Repeater {
                        model: [
                            "#ffffff", "#a0a8b8", "#4e85f4", "#e94560",
                            "#00cc6d", "#ffaa00", "#a2693d", "#c0c5e3",
                            "#e73249", "#18c37e", "#ff4444", "#9C27B0"
                        ]

                        Rectangle {
                            width: Theme.scaled(24)
                            height: Theme.scaled(24)
                            radius: Theme.scaled(12)
                            color: modelData
                            border.color: colorMa.pressed ? "white" : Theme.borderColor
                            border.width: 1

                            MouseArea {
                                id: colorMa
                                anchors.fill: parent
                                onClicked: popup.insertColor(modelData)
                            }
                        }
                    }
                }
            }

            // Vertical separator
            Rectangle {
                Layout.fillHeight: true
                width: 1
                color: Theme.borderColor
            }

            // --- CENTER COLUMN: Variables (scrollable) ---
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.scaled(2)

                Text {
                    text: "Variables"
                    color: Theme.textSecondaryColor
                    font: Theme.captionFont
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    model: [
                        { token: "%TEMP%", label: "Temp (°C)" },
                        { token: "%STEAM_TEMP%", label: "Steam (°C)" },
                        { token: "%PRESSURE%", label: "Pressure (bar)" },
                        { token: "%FLOW%", label: "Flow (ml/s)" },
                        { token: "%WATER%", label: "Water (%)" },
                        { token: "%WATER_ML%", label: "Water (ml)" },
                        { token: "%WEIGHT%", label: "Weight (g)" },
                        { token: "%SHOT_TIME%", label: "Shot Time (s)" },
                        { token: "%TARGET_WEIGHT%", label: "Target Wt (g)" },
                        { token: "%VOLUME%", label: "Volume (ml)" },
                        { token: "%PROFILE%", label: "Profile Name" },
                        { token: "%STATE%", label: "Machine State" },
                        { token: "%TARGET_TEMP%", label: "Target Temp" },
                        { token: "%SCALE%", label: "Scale Name" },
                        { token: "%TIME%", label: "Time (HH:MM)" },
                        { token: "%DATE%", label: "Date" },
                        { token: "%RATIO%", label: "Brew Ratio" },
                        { token: "%DOSE%", label: "Dose (g)" },
                        { token: "%CONNECTED%", label: "Online/Offline" },
                        { token: "%CONNECTED_COLOR%", label: "Status Color" },
                        { token: "%DEVICES%", label: "Devices" }
                    ]

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: Theme.scaled(26)
                        radius: Theme.scaled(3)
                        color: varDelegateMa.pressed ? Qt.darker(Theme.backgroundColor, 1.3) : "transparent"

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: Theme.scaled(4)
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.label
                            color: Theme.primaryColor
                            font: Theme.captionFont
                        }

                        MouseArea {
                            id: varDelegateMa
                            anchors.fill: parent
                            onClicked: popup.insertVariable(modelData.token)
                        }
                    }
                }
            }

            // Vertical separator
            Rectangle {
                Layout.fillHeight: true
                width: 1
                color: Theme.borderColor
            }

            // --- RIGHT COLUMN: Actions (scrollable) ---
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.scaled(2)

                Text {
                    text: "Action"
                    color: Theme.textSecondaryColor
                    font: Theme.captionFont
                }

                ListView {
                    id: actionList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    model: {
                        var items = [{ id: "", label: "None" }]
                        var filtered = popup.getFilteredActions()
                        for (var i = 0; i < filtered.length; i++)
                            items.push(filtered[i])
                        return items
                    }

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: Theme.scaled(26)
                        radius: Theme.scaled(3)
                        color: popup.textAction === modelData.id
                            ? Theme.primaryColor
                            : (actionDelegateMa.pressed ? Qt.darker(Theme.backgroundColor, 1.3) : "transparent")

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: Theme.scaled(4)
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.label
                            color: popup.textAction === modelData.id ? "white" : Theme.textColor
                            font: Theme.captionFont
                        }

                        MouseArea {
                            id: actionDelegateMa
                            anchors.fill: parent
                            onClicked: popup.textAction = modelData.id
                        }
                    }
                }
            }
        }

        // === ROW 3: Preview + Buttons ===
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.scaled(6)

            // Preview
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: Math.max(previewText.implicitHeight + Theme.scaled(8), Theme.scaled(32))
                color: Theme.backgroundColor
                radius: Theme.scaled(6)
                border.color: popup.textAction !== "" ? Theme.primaryColor : Theme.borderColor
                border.width: popup.textAction !== "" ? 2 : 1

                Text {
                    id: previewText
                    anchors.centerIn: parent
                    width: parent.width - Theme.scaled(12)
                    text: popup.substitutePreview(contentInput.text)
                    textFormat: Text.RichText
                    color: Theme.textColor
                    font: Theme.bodyFont
                    horizontalAlignment: {
                        switch (popup.textAlign) {
                            case "left": return Text.AlignLeft
                            case "right": return Text.AlignRight
                            default: return Text.AlignHCenter
                        }
                    }
                    wrapMode: Text.Wrap
                }
            }

            // Buttons stacked vertically
            ColumnLayout {
                Layout.preferredWidth: popup.width * 0.15
                Layout.maximumWidth: popup.width * 0.15
                spacing: Theme.scaled(4)

                // AI Advice
                Rectangle {
                    visible: MainController.aiManager
                    Layout.fillWidth: true
                    Layout.minimumWidth: aiRow.implicitWidth + Theme.scaled(16)
                    height: Theme.scaled(28)
                    radius: Theme.scaled(6)
                    opacity: MainController.aiManager && MainController.aiManager.isConfigured
                             && contentInput.text.trim().length > 0 && !popup.aiPending ? 1.0 : 0.5
                    color: {
                        if (popup.aiPending)
                            return Theme.primaryColor
                        if (aiMa.pressed)
                            return Qt.darker(Theme.surfaceColor, 1.2)
                        return Theme.surfaceColor
                    }
                    border.color: Theme.primaryColor; border.width: 1

                    Row {
                        id: aiRow
                        anchors.centerIn: parent
                        spacing: Theme.scaled(4)

                        Image {
                            source: "qrc:/icons/sparkle.svg"
                            width: Theme.scaled(14)
                            height: Theme.scaled(14)
                            anchors.verticalCenter: parent.verticalCenter
                            visible: status === Image.Ready
                        }

                        Text {
                            text: popup.aiPending ? "Working..." : "Ask AI"
                            color: popup.aiPending ? "white" : Theme.primaryColor
                            font: Theme.captionFont
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        id: aiMa
                        anchors.fill: parent
                        enabled: MainController.aiManager && MainController.aiManager.isConfigured
                                 && contentInput.text.trim().length > 0 && !popup.aiPending
                        onClicked: popup.requestAiAdvice()
                    }
                }

                // Cancel
                Rectangle {
                    Layout.fillWidth: true
                    height: Theme.scaled(28)
                    radius: Theme.scaled(6)
                    color: cancelMa.pressed ? Qt.darker(Theme.backgroundColor, 1.2) : Theme.backgroundColor
                    border.color: Theme.borderColor; border.width: 1

                    Text {
                        id: cancelText
                        anchors.centerIn: parent
                        text: "Cancel"
                        color: Theme.textColor
                        font: Theme.captionFont
                    }
                    MouseArea { id: cancelMa; anchors.fill: parent; onClicked: popup.close() }
                }

                // Save
                Rectangle {
                    Layout.fillWidth: true
                    height: Theme.scaled(28)
                    radius: Theme.scaled(6)
                    color: saveMa.pressed ? Qt.darker(Theme.primaryColor, 1.2) : Theme.primaryColor

                    Text {
                        id: saveText
                        anchors.centerIn: parent
                        text: "Save"
                        color: "white"
                        font: Theme.captionFont
                    }
                    MouseArea { id: saveMa; anchors.fill: parent; onClicked: popup.doSave() }
                }
            }
        }
    }

    // Action registry with page context filtering
    function getFilteredActions() {
        var allActions = [
            { id: "navigate:settings",      label: "Go to Settings",       contexts: ["idle", "all"] },
            { id: "navigate:history",        label: "Go to History",        contexts: ["idle", "all"] },
            { id: "navigate:profiles",       label: "Go to Profiles",       contexts: ["idle", "all"] },
            { id: "navigate:profileEditor",  label: "Go to Profile Editor", contexts: ["idle", "all"] },
            { id: "navigate:recipes",        label: "Go to Recipes",        contexts: ["idle", "all"] },
            { id: "navigate:descaling",      label: "Go to Descaling",      contexts: ["idle", "all"] },
            { id: "navigate:ai",             label: "Go to AI Settings",    contexts: ["idle", "all"] },
            { id: "navigate:visualizer",     label: "Go to Visualizer",     contexts: ["idle", "all"] },
            { id: "command:sleep",           label: "Sleep",                contexts: ["idle"] },
            { id: "command:startEspresso",   label: "Start Espresso",       contexts: ["idle"] },
            { id: "command:startSteam",      label: "Start Steam",          contexts: ["idle"] },
            { id: "command:startHotWater",   label: "Start Hot Water",      contexts: ["idle"] },
            { id: "command:startFlush",      label: "Start Flush",          contexts: ["idle"] },
            { id: "command:idle",            label: "Stop (Idle)",          contexts: ["idle", "espresso", "steam", "hotwater", "flush"] },
            { id: "command:tare",            label: "Tare Scale",           contexts: ["idle", "espresso", "all"] }
        ]
        var ctx = popup.pageContext
        return allActions.filter(function(a) {
            return a.contexts.indexOf(ctx) >= 0 || a.contexts.indexOf("all") >= 0
        })
    }
}
