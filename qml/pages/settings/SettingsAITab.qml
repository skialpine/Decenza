import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DecenzaDE1
import "../../components"

KeyboardAwareContainer {
    id: aiTab
    textFields: [apiKeyField, ollamaEndpointField]

    property string testResultMessage: ""
    property bool testResultSuccess: false

    // Helper function to check if provider has a key configured
    function isProviderConfigured(providerId) {
        switch(providerId) {
            case "openai": return Settings.openaiApiKey.length > 0
            case "anthropic": return Settings.anthropicApiKey.length > 0
            case "gemini": return Settings.geminiApiKey.length > 0
            case "ollama": return Settings.ollamaEndpoint.length > 0 && Settings.ollamaModel.length > 0
            default: return false
        }
    }

    ColumnLayout {
        id: aiTabContent
        anchors.fill: parent
        spacing: Theme.scaled(10)

        // Provider selection - horizontal row
        Rectangle {
            Layout.fillWidth: true
            height: Theme.scaled(70)
            color: Theme.surfaceColor
            radius: Theme.cardRadius

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.scaled(8)
                spacing: Theme.scaled(6)

                Repeater {
                    model: [
                        { id: "openai", name: "OpenAI", desc: "GPT-4o" },
                        { id: "anthropic", name: "Anthropic", desc: "Claude" },
                        { id: "gemini", name: "Gemini", desc: "Flash" },
                        { id: "ollama", name: "Ollama", desc: "Local" }
                    ]

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: Theme.scaled(6)

                        property bool isSelected: Settings.aiProvider === modelData.id
                        property bool hasKey: aiTab.isProviderConfigured(modelData.id)

                        color: {
                            if (isSelected) return Theme.primaryColor
                            if (hasKey) return Qt.rgba(0.2, 0.7, 0.3, 0.25)
                            return Qt.darker(Theme.surfaceColor, 1.15)
                        }
                        border.color: {
                            if (isSelected) return Theme.primaryColor
                            if (hasKey) return Qt.rgba(0.2, 0.7, 0.3, 0.5)
                            return "transparent"
                        }
                        border.width: isSelected ? 2 : 1

                        Column {
                            anchors.centerIn: parent
                            spacing: Theme.scaled(2)

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData.name
                                font.pixelSize: Theme.scaled(12)
                                font.bold: isSelected
                                color: isSelected ? "white" : Theme.textColor
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData.desc
                                font.pixelSize: Theme.scaled(10)
                                color: isSelected ? Qt.rgba(1,1,1,0.8) : Theme.textSecondaryColor
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: Settings.aiProvider = modelData.id
                        }
                    }
                }
            }
        }

        // Claude recommendation note
        Rectangle {
            Layout.fillWidth: true
            color: Qt.rgba(Theme.primaryColor.r, Theme.primaryColor.g, Theme.primaryColor.b, 0.15)
            radius: Theme.cardRadius
            height: recommendationText.implicitHeight + 16

            Tr {
                id: recommendationText
                anchors.fill: parent
                anchors.margins: Theme.scaled(8)
                key: "settings.ai.recommendation"
                fallback: "For shot analysis, we recommend Claude (Anthropic). In our testing, Claude better understands espresso extraction dynamics and gives more accurate dial-in advice. Other providers work for translation and general tasks."
                wrapMode: Text.WordWrap
                color: Theme.textSecondaryColor
                font.pixelSize: Theme.scaled(11)
            }
        }

        // API Key / Ollama settings
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surfaceColor
            radius: Theme.cardRadius

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.scaled(12)
                spacing: Theme.scaled(8)

                // Cloud provider API key
                ColumnLayout {
                    visible: Settings.aiProvider !== "ollama"
                    Layout.fillWidth: true
                    spacing: Theme.scaled(6)

                    Tr {
                        key: "settings.ai.apiKey"
                        fallback: "API Key"
                        color: Theme.textColor
                        font.pixelSize: Theme.scaled(13)
                        font.bold: true
                    }

                    StyledTextField {
                        id: apiKeyField
                        Layout.fillWidth: true
                        echoMode: TextInput.Password
                        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                        text: {
                            switch(Settings.aiProvider) {
                                case "openai": return Settings.openaiApiKey
                                case "anthropic": return Settings.anthropicApiKey
                                case "gemini": return Settings.geminiApiKey
                                default: return ""
                            }
                        }
                        onTextChanged: {
                            switch(Settings.aiProvider) {
                                case "openai": Settings.openaiApiKey = text; break
                                case "anthropic": Settings.anthropicApiKey = text; break
                                case "gemini": Settings.geminiApiKey = text; break
                            }
                        }
                    }

                    Text {
                        text: {
                            var getKey = TranslationManager.translate("settings.ai.getkey", "Get key:")
                            switch(Settings.aiProvider) {
                                case "openai": return getKey + " platform.openai.com -> API Keys"
                                case "anthropic": return getKey + " console.anthropic.com -> API Keys"
                                case "gemini": return getKey + " aistudio.google.com -> Get API Key"
                                default: return ""
                            }
                        }
                        color: Theme.textSecondaryColor
                        font.pixelSize: Theme.scaled(11)
                    }
                }

                // Ollama settings
                ColumnLayout {
                    visible: Settings.aiProvider === "ollama"
                    Layout.fillWidth: true
                    spacing: Theme.scaled(6)

                    Tr {
                        key: "settings.ai.ollamaSettings"
                        fallback: "Ollama Settings"
                        color: Theme.textColor
                        font.pixelSize: Theme.scaled(13)
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.scaled(8)

                        StyledTextField {
                            id: ollamaEndpointField
                            Layout.fillWidth: true
                            placeholderText: "http://localhost:11434"
                            text: Settings.ollamaEndpoint
                            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
                            onTextChanged: Settings.ollamaEndpoint = text
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.scaled(8)

                        StyledComboBox {
                            Layout.fillWidth: true
                            model: MainController.aiManager ? MainController.aiManager.ollamaModels : []
                            currentIndex: model ? model.indexOf(Settings.ollamaModel) : -1
                            onCurrentTextChanged: if (currentText) Settings.ollamaModel = currentText
                            background: Rectangle {
                                implicitHeight: Theme.scaled(36)
                                color: Theme.surfaceColor
                                border.color: Theme.borderColor
                                radius: Theme.scaled(6)
                            }
                        }

                        Rectangle {
                            width: Theme.scaled(70)
                            height: Theme.scaled(36)
                            radius: Theme.scaled(6)
                            color: Theme.surfaceColor
                            border.color: Theme.borderColor

                            Tr {
                                anchors.centerIn: parent
                                key: "settings.ai.refresh"
                                fallback: "Refresh"
                                color: Theme.textColor
                                font.pixelSize: Theme.scaled(12)
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: MainController.aiManager?.refreshOllamaModels()
                            }
                        }
                    }

                    Tr {
                        key: "settings.ai.ollamainstall"
                        fallback: "Install: ollama.ai -> run: ollama pull llama3.2"
                        color: Theme.textSecondaryColor
                        font.pixelSize: Theme.scaled(11)
                    }
                }

                Item { Layout.fillHeight: true }

                // Test connection + cost in a row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.scaled(10)

                    Rectangle {
                        width: Theme.scaled(110)
                        height: Theme.scaled(36)
                        radius: Theme.scaled(6)
                        color: MainController.aiManager?.isConfigured ? Theme.primaryColor : Theme.surfaceColor
                        border.color: MainController.aiManager?.isConfigured ? Theme.primaryColor : Theme.borderColor

                        Tr {
                            anchors.centerIn: parent
                            key: "settings.ai.testconnection"
                            fallback: "Test Connection"
                            color: MainController.aiManager?.isConfigured ? "white" : Theme.textSecondaryColor
                            font.pixelSize: Theme.scaled(12)
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: MainController.aiManager?.isConfigured
                            onClicked: {
                                aiTab.testResultMessage = TranslationManager.translate("settings.ai.testing", "Testing...")
                                MainController.aiManager.testConnection()
                            }
                        }
                    }

                    Text {
                        visible: aiTab.testResultMessage.length > 0
                        text: aiTab.testResultMessage
                        color: aiTab.testResultSuccess ? Theme.successColor : Theme.errorColor
                        font.pixelSize: Theme.scaled(11)
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Text {
                        visible: aiTab.testResultMessage.length === 0
                        text: {
                            switch(Settings.aiProvider) {
                                case "openai": return "~$0.01/" + TranslationManager.translate("settings.ai.pershot", "shot")
                                case "anthropic": return "~$0.003/" + TranslationManager.translate("settings.ai.pershot", "shot")
                                case "gemini": return "~$0.002/" + TranslationManager.translate("settings.ai.pershot", "shot")
                                case "ollama": return TranslationManager.translate("settings.ai.free", "Free")
                                default: return ""
                            }
                        }
                        color: Theme.textSecondaryColor
                        font.pixelSize: Theme.scaled(11)
                    }

                    Item { Layout.fillWidth: true }

                    // Continue Conversation button
                    Rectangle {
                        id: continueConversationBtn
                        property bool hasConversation: MainController.aiManager && MainController.aiManager.conversation &&
                                                       MainController.aiManager.conversation.hasSavedConversation
                        visible: MainController.aiManager && MainController.aiManager.isConfigured
                        width: Math.max(Theme.scaled(100), continueText.implicitWidth + Theme.scaled(24))
                        height: Theme.scaled(36)
                        radius: Theme.scaled(6)
                        color: Theme.surfaceColor
                        border.color: continueConversationBtn.hasConversation ? Theme.primaryColor : Theme.borderColor
                        opacity: continueConversationBtn.hasConversation ? 1.0 : 0.5

                        Text {
                            id: continueText
                            anchors.centerIn: parent
                            text: continueConversationBtn.hasConversation
                                  ? TranslationManager.translate("settings.ai.continueconversation", "Continue Chat")
                                  : TranslationManager.translate("settings.ai.noconversation", "No Chat")
                            font.pixelSize: Theme.scaled(12)
                            color: continueConversationBtn.hasConversation ? Theme.primaryColor : Theme.textSecondaryColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: continueConversationBtn.hasConversation
                            onClicked: {
                                MainController.aiManager.conversation.loadFromStorage()
                                conversationOverlay.visible = true
                            }
                        }
                    }
                }
            }
        }
    }

    // Conversation overlay panel
    Rectangle {
        id: conversationOverlay
        visible: false
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.7)
        z: 200

        MouseArea {
            anchors.fill: parent
            onClicked: {
                MainController.aiManager?.conversation?.saveToStorage()
                conversationOverlay.visible = false
            }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: Theme.scaled(16)
            color: Theme.surfaceColor
            radius: Theme.cardRadius

            MouseArea { anchors.fill: parent; onClicked: {} }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingMedium
                spacing: Theme.spacingSmall

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: TranslationManager.translate("settings.ai.conversation.title", "AI Conversation")
                        font: Theme.subtitleFont
                        color: Theme.textColor
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        width: clearBtnText.width + Theme.scaled(16)
                        height: Theme.scaled(28)
                        radius: Theme.scaled(4)
                        color: Theme.errorColor

                        Text {
                            id: clearBtnText
                            anchors.centerIn: parent
                            text: TranslationManager.translate("settings.ai.conversation.clear", "Clear")
                            font.pixelSize: Theme.scaled(11)
                            color: "white"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                MainController.aiManager?.conversation?.clearHistory()
                                MainController.aiManager?.conversation?.saveToStorage()
                                conversationOverlay.visible = false
                            }
                        }
                    }

                    Item { width: Theme.scaled(12) }

                    Rectangle {
                        width: Theme.scaled(28)
                        height: Theme.scaled(28)
                        radius: Theme.scaled(14)
                        color: Theme.backgroundColor

                        Text {
                            anchors.centerIn: parent
                            text: "\u00D7"
                            font.pixelSize: Theme.scaled(18)
                            color: Theme.textColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                MainController.aiManager?.conversation?.saveToStorage()
                                conversationOverlay.visible = false
                            }
                        }
                    }
                }

                Flickable {
                    id: conversationFlickable
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentHeight: conversationText.height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    TextArea {
                        id: conversationText
                        width: parent.width
                        text: MainController.aiManager?.conversation?.getConversationText() ?? ""
                        textFormat: Text.MarkdownText
                        wrapMode: TextEdit.WordWrap
                        readOnly: true
                        font: Theme.bodyFont
                        color: Theme.textColor
                        background: null
                        padding: 0
                    }
                }

                RowLayout {
                    visible: MainController.aiManager?.conversation?.busy ?? false
                    Layout.fillWidth: true

                    BusyIndicator {
                        running: true
                        Layout.preferredWidth: Theme.scaled(20)
                        Layout.preferredHeight: Theme.scaled(20)
                        palette.dark: Theme.primaryColor
                    }

                    Text {
                        text: TranslationManager.translate("settings.ai.conversation.thinking", "Thinking...")
                        font: Theme.bodyFont
                        color: Theme.textSecondaryColor
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall

                    StyledTextField {
                        id: conversationInput
                        Layout.fillWidth: true
                        placeholder: TranslationManager.translate("settings.ai.conversation.placeholder", "Ask a question...")
                        enabled: !(MainController.aiManager?.conversation?.busy ?? true)

                        Keys.onReturnPressed: sendMsg()
                        Keys.onEnterPressed: sendMsg()

                        function sendMsg() {
                            if (text.length === 0) return
                            MainController.aiManager?.conversation?.followUp(text)
                            text = ""
                        }
                    }

                    Rectangle {
                        width: Theme.scaled(50)
                        height: Theme.scaled(36)
                        radius: Theme.scaled(6)
                        color: conversationInput.text.length > 0 ? Theme.primaryColor : Theme.surfaceColor
                        border.color: conversationInput.text.length > 0 ? Theme.primaryColor : Theme.borderColor

                        Text {
                            anchors.centerIn: parent
                            text: TranslationManager.translate("settings.ai.conversation.send", "Send")
                            font.pixelSize: Theme.scaled(12)
                            color: conversationInput.text.length > 0 ? "white" : Theme.textSecondaryColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: conversationInput.text.length > 0 && !(MainController.aiManager?.conversation?.busy ?? true)
                            onClicked: conversationInput.sendMsg()
                        }
                    }
                }
            }
        }

        Connections {
            target: MainController.aiManager?.conversation ?? null
            function onResponseReceived() {
                Qt.callLater(function() {
                    conversationFlickable.contentY = Math.max(0, conversationFlickable.contentHeight - conversationFlickable.height)
                })
            }
            function onHistoryChanged() {
                conversationText.text = MainController.aiManager?.conversation?.getConversationText() ?? ""
            }
        }
    }

    Connections {
        target: MainController.aiManager
        function onTestResultChanged() {
            aiTab.testResultSuccess = MainController.aiManager.lastTestSuccess
            aiTab.testResultMessage = MainController.aiManager.lastTestResult
        }
    }
}
