import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DecenzaDE1

Rectangle {
    id: card

    property var entryData: ({})
    property int displayMode: 0  // 0=full preview, 1=compact list
    property bool isSelected: false

    width: parent ? parent.width : 100
    height: displayMode === 0 ? Theme.scaled(72) : Theme.scaled(40)
    radius: Theme.cardRadius
    color: isSelected ? Qt.rgba(Theme.primaryColor.r, Theme.primaryColor.g, Theme.primaryColor.b, 0.15)
                      : Theme.backgroundColor
    border.color: isSelected ? Theme.primaryColor : Theme.borderColor
    border.width: isSelected ? 2 : 1

    // Type badge colors
    function typeBadgeColor(type) {
        switch (type) {
            case "item": return Theme.primaryColor
            case "zone": return Theme.accentColor
            case "layout": return Theme.successColor
            default: return Theme.textSecondaryColor
        }
    }

    function typeBadgeLabel(type) {
        switch (type) {
            case "item": return "ITEM"
            case "zone": return "ZONE"
            case "layout": return "LAYOUT"
            default: return type.toUpperCase()
        }
    }

    // Full preview mode
    ColumnLayout {
        visible: displayMode === 0
        anchors.fill: parent
        anchors.margins: Theme.scaled(8)
        spacing: Theme.scaled(4)

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.scaled(6)

            // Type badge
            Rectangle {
                width: badgeText.implicitWidth + Theme.scaled(8)
                height: Theme.scaled(18)
                radius: Theme.scaled(4)
                color: typeBadgeColor(entryData.type || "")

                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: typeBadgeLabel(entryData.type || "")
                    color: "white"
                    font.family: Theme.captionFont.family
                    font.pixelSize: Theme.scaled(10)
                    font.bold: true
                }
            }

            // Entry name
            Text {
                Layout.fillWidth: true
                text: entryData.name || ""
                color: Theme.textColor
                font: Theme.bodyFont
                elide: Text.ElideRight
            }
        }

        // Description
        Text {
            Layout.fillWidth: true
            visible: (entryData.description || "") !== ""
            text: entryData.description || ""
            color: Theme.textSecondaryColor
            font: Theme.captionFont
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        // Tags preview
        Flow {
            Layout.fillWidth: true
            spacing: Theme.scaled(4)
            clip: true

            Repeater {
                model: {
                    var tags = entryData.tags || []
                    // Show up to 4 tags, filter to readable ones
                    var display = []
                    for (var i = 0; i < tags.length && display.length < 4; i++) {
                        var tag = tags[i] || ""
                        if (tag.indexOf("var:") === 0) {
                            display.push(tag.substring(4))  // Show %TEMP% etc.
                        } else if (tag.indexOf("action:") === 0) {
                            var parts = tag.substring(7).split(":")
                            display.push(parts[parts.length - 1])  // Show "settings", "history" etc.
                        }
                    }
                    return display
                }

                Rectangle {
                    width: tagText.implicitWidth + Theme.scaled(6)
                    height: Theme.scaled(16)
                    radius: Theme.scaled(3)
                    color: Qt.rgba(Theme.primaryColor.r, Theme.primaryColor.g, Theme.primaryColor.b, 0.1)

                    Text {
                        id: tagText
                        anchors.centerIn: parent
                        text: modelData
                        color: Theme.primaryColor
                        font.family: Theme.captionFont.family
                        font.pixelSize: Theme.scaled(9)
                    }
                }
            }
        }
    }

    // Compact list mode
    RowLayout {
        visible: displayMode === 1
        anchors.fill: parent
        anchors.leftMargin: Theme.scaled(8)
        anchors.rightMargin: Theme.scaled(8)
        spacing: Theme.scaled(6)

        // Type badge
        Rectangle {
            width: compactBadgeText.implicitWidth + Theme.scaled(6)
            height: Theme.scaled(16)
            radius: Theme.scaled(3)
            color: typeBadgeColor(entryData.type || "")

            Text {
                id: compactBadgeText
                anchors.centerIn: parent
                text: typeBadgeLabel(entryData.type || "")
                color: "white"
                font.family: Theme.captionFont.family
                font.pixelSize: Theme.scaled(9)
                font.bold: true
            }
        }

        // Name
        Text {
            Layout.fillWidth: true
            text: entryData.name || ""
            color: Theme.textColor
            font: Theme.bodyFont
            elide: Text.ElideRight
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: card.clicked()
        onDoubleClicked: card.doubleClicked()
    }

    signal clicked()
    signal doubleClicked()
}
