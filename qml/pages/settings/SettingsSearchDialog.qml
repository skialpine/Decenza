import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Decenza
import "../../components"
import "../../components/SettingsSearchIndex.js" as SearchIndex

Dialog {
    id: searchDialog
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width * 0.85, Theme.scaled(450))
    height: Math.min(parent.height * 0.7, Theme.scaled(500))
    modal: true
    dim: true
    padding: Theme.scaled(16)
    closePolicy: Dialog.CloseOnEscape | Dialog.CloseOnPressOutside

    signal resultSelected(int tabIndex)

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.cardRadius
        border.color: Theme.borderColor
        border.width: 1
    }

    onOpened: {
        searchField.text = ""
        searchField.forceActiveFocus()
    }

    // Filter results based on search text
    property var allEntries: SearchIndex.getSearchEntries()
    property var filteredEntries: {
        var query = searchField.text.trim().toLowerCase()
        if (query.length === 0) return allEntries

        var results = []
        var words = query.split(/\s+/)

        for (var i = 0; i < allEntries.length; i++) {
            var entry = allEntries[i]
            var searchText = (entry.title + " " + entry.description + " " + entry.keywords.join(" ")).toLowerCase()

            var allMatch = true
            for (var w = 0; w < words.length; w++) {
                if (searchText.indexOf(words[w]) === -1) {
                    allMatch = false
                    break
                }
            }
            if (allMatch) results.push(entry)
        }
        return results
    }

    contentItem: ColumnLayout {
        spacing: Theme.scaled(12)

        // Search field
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.scaled(8)

            Image {
                source: "qrc:/icons/search.svg"
                sourceSize.width: Theme.scaled(20)
                sourceSize.height: Theme.scaled(20)
                Layout.alignment: Qt.AlignVCenter
                visible: source != ""
            }

            StyledTextField {
                id: searchField
                Layout.fillWidth: true
                placeholder: TranslationManager.translate("settings.search.placeholder", "Search settings...")
                accessibleName: TranslationManager.translate("settings.search.placeholder", "Search settings")
            }
        }

        // Results count
        Text {
            text: filteredEntries.length === allEntries.length
                ? TranslationManager.translate("settings.search.browseAll", "Browse all settings")
                : TranslationManager.translate("settings.search.resultsCount", "%1 results").arg(filteredEntries.length)
            color: Theme.textSecondaryColor
            font.pixelSize: Theme.scaled(11)
        }

        // Results list
        ListView {
            id: resultsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: filteredEntries

            delegate: Rectangle {
                id: resultDelegate
                width: resultsList.width
                height: resultContent.implicitHeight + Theme.scaled(16)
                color: resultMouseArea.containsMouse ? Theme.backgroundColor : "transparent"
                radius: Theme.scaled(6)

                Accessible.role: Accessible.Button
                Accessible.name: modelData.title + ", " + SearchIndex.getTabName(modelData.tabIndex) + " tab"
                Accessible.focusable: true
                Accessible.onPressAction: resultMouseArea.clicked(null)

                RowLayout {
                    id: resultContent
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: Theme.scaled(8)
                    spacing: Theme.scaled(8)

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.scaled(2)

                        Text {
                            text: modelData.title
                            color: Theme.textColor
                            font.pixelSize: Theme.scaled(14)
                            font.bold: true
                            Accessible.ignored: true
                        }

                        Text {
                            Layout.fillWidth: true
                            text: modelData.description
                            color: Theme.textSecondaryColor
                            font.pixelSize: Theme.scaled(11)
                            wrapMode: Text.WordWrap
                            Accessible.ignored: true
                        }
                    }

                    // Tab badge
                    Rectangle {
                        Layout.alignment: Qt.AlignVCenter
                        implicitWidth: tabBadgeText.implicitWidth + Theme.scaled(12)
                        implicitHeight: Theme.scaled(20)
                        radius: Theme.scaled(10)
                        color: Qt.rgba(Theme.primaryColor.r, Theme.primaryColor.g, Theme.primaryColor.b, 0.15)

                        Text {
                            id: tabBadgeText
                            anchors.centerIn: parent
                            text: SearchIndex.getTabName(modelData.tabIndex)
                            color: Theme.primaryColor
                            font.pixelSize: Theme.scaled(10)
                            font.bold: true
                            Accessible.ignored: true
                        }
                    }
                }

                MouseArea {
                    id: resultMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        searchDialog.resultSelected(modelData.tabIndex)
                        searchDialog.close()
                    }
                }
            }

            // Empty state
            Text {
                anchors.centerIn: parent
                visible: filteredEntries.length === 0
                text: TranslationManager.translate("settings.search.noResults", "No settings found")
                color: Theme.textSecondaryColor
                font.pixelSize: Theme.scaled(14)
            }
        }

        // Close button
        AccessibleButton {
            Layout.alignment: Qt.AlignRight
            text: TranslationManager.translate("common.button.close", "Close")
            accessibleName: TranslationManager.translate("settings.search.close", "Close search")
            onClicked: searchDialog.close()
        }
    }
}
