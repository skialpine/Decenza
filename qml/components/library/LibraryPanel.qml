import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DecenzaDE1
import ".."

Rectangle {
    id: libraryPanel

    // Interface with layout editor
    property string selectedItemId: ""
    property string selectedFromZone: ""
    property string selectedZoneName: ""

    // Display state
    property int displayMode: 0  // 0=full, 1=compact
    property string activeTab: "local"  // "local", "community", "featured", "mine"

    color: Theme.surfaceColor
    radius: Theme.cardRadius
    border.color: Theme.borderColor
    border.width: 1

    onActiveTabChanged: {
        if (activeTab === "community")
            LibrarySharing.browseCommunity("", "", "", "", "newest", 1)
        else if (activeTab === "featured")
            LibrarySharing.loadFeatured()
        else if (activeTab === "mine")
            LibrarySharing.browseMyUploads(1)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.scaled(8)
        spacing: Theme.scaled(6)

        // Header with display mode toggles
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.scaled(6)

            Text {
                text: "Library"
                color: Theme.textColor
                font: Theme.subtitleFont
                Layout.fillWidth: true
            }

            // Full preview mode button
            Rectangle {
                width: Theme.scaled(28)
                height: Theme.scaled(28)
                radius: Theme.scaled(4)
                color: displayMode === 0 ? Theme.primaryColor : "transparent"
                border.color: displayMode === 0 ? Theme.primaryColor : Theme.borderColor
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "\u25A3"  // Grid icon
                    color: displayMode === 0 ? "white" : Theme.textSecondaryColor
                    font.pixelSize: Theme.scaled(14)
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: displayMode = 0
                }
            }

            // Compact list mode button
            Rectangle {
                width: Theme.scaled(28)
                height: Theme.scaled(28)
                radius: Theme.scaled(4)
                color: displayMode === 1 ? Theme.primaryColor : "transparent"
                border.color: displayMode === 1 ? Theme.primaryColor : Theme.borderColor
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "\u2630"  // List icon
                    color: displayMode === 1 ? "white" : Theme.textSecondaryColor
                    font.pixelSize: Theme.scaled(14)
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: displayMode = 1
                }
            }
        }

        // Tab row
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.scaled(2)

            Repeater {
                model: [
                    { key: "local", label: "My Library" },
                    { key: "community", label: "Community" },
                    { key: "featured", label: "Featured" },
                    { key: "mine", label: "Shared" }
                ]

                Rectangle {
                    Layout.fillWidth: true
                    height: Theme.scaled(28)
                    radius: Theme.scaled(4)
                    color: activeTab === modelData.key
                        ? Theme.primaryColor
                        : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: modelData.label
                        color: activeTab === modelData.key ? "white" : Theme.textSecondaryColor
                        font.family: Theme.captionFont.family
                        font.pixelSize: Theme.scaled(10)
                        font.bold: activeTab === modelData.key
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: activeTab = modelData.key
                    }
                }
            }
        }

        // Action buttons row
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.scaled(4)

            // Add button with dropdown
            Rectangle {
                Layout.fillWidth: true
                height: Theme.scaled(30)
                radius: Theme.scaled(4)
                color: addMa.pressed ? Qt.darker(Theme.primaryColor, 1.2) : Theme.primaryColor

                RowLayout {
                    anchors.centerIn: parent
                    spacing: Theme.scaled(4)

                    Text {
                        text: "+"
                        color: "white"
                        font.family: Theme.bodyFont.family
                        font.pixelSize: Theme.scaled(14)
                        font.bold: true
                    }
                    Text {
                        text: "Add"
                        color: "white"
                        font.family: Theme.bodyFont.family
                        font.pixelSize: Theme.scaled(12)
                    }
                    Text {
                        text: "\u25BE"
                        color: "white"
                        font.pixelSize: Theme.scaled(10)
                    }
                }

                MouseArea {
                    id: addMa
                    anchors.fill: parent
                    onClicked: addMenu.open()
                }

                Popup {
                    id: addMenu
                    y: parent.height + Theme.scaled(4)
                    padding: Theme.scaled(4)
                    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

                    background: Rectangle {
                        color: Theme.surfaceColor
                        radius: Theme.cardRadius
                        border.color: Theme.borderColor
                        border.width: 1
                    }

                    contentItem: ColumnLayout {
                        spacing: Theme.scaled(2)

                        Repeater {
                            model: [
                                { label: "Save Item", type: "item", enabled: selectedItemId !== "" },
                                { label: "Save Zone", type: "zone", enabled: selectedZoneName !== "" },
                                { label: "Save Layout", type: "layout", enabled: true }
                            ]

                            Rectangle {
                                Layout.fillWidth: true
                                implicitWidth: Theme.scaled(140)
                                height: Theme.scaled(32)
                                radius: Theme.scaled(4)
                                color: menuItemMa.containsMouse && modelData.enabled
                                    ? Qt.rgba(Theme.primaryColor.r, Theme.primaryColor.g, Theme.primaryColor.b, 0.12)
                                    : "transparent"
                                opacity: modelData.enabled ? 1.0 : 0.4

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: Theme.scaled(10)
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.label
                                    color: Theme.textColor
                                    font: Theme.bodyFont
                                }

                                MouseArea {
                                    id: menuItemMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    enabled: modelData.enabled
                                    onClicked: {
                                        addMenu.close()
                                        saveDialog.saveType = modelData.type
                                        saveDialog.showThemeCheckbox = (modelData.type === "layout")
                                        saveDialog.open()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Delete button
            Rectangle {
                width: Theme.scaled(30)
                height: Theme.scaled(30)
                radius: Theme.scaled(4)
                color: delMa.pressed ? Qt.rgba(Theme.errorColor.r, Theme.errorColor.g, Theme.errorColor.b, 0.3) : "transparent"
                border.color: WidgetLibrary.selectedEntryId !== "" ? Theme.errorColor : Theme.borderColor
                border.width: 1
                opacity: WidgetLibrary.selectedEntryId !== "" ? 1.0 : 0.4

                Text {
                    anchors.centerIn: parent
                    text: "\uD83D\uDDD1"  // Wastebasket
                    font.pixelSize: Theme.scaled(14)
                }

                MouseArea {
                    id: delMa
                    anchors.fill: parent
                    enabled: WidgetLibrary.selectedEntryId !== ""
                    onClicked: deleteConfirm.open()
                }
            }

            // Share button
            Rectangle {
                width: Theme.scaled(30)
                height: Theme.scaled(30)
                radius: Theme.scaled(4)
                color: shareMa.pressed ? Qt.rgba(Theme.primaryColor.r, Theme.primaryColor.g, Theme.primaryColor.b, 0.3) : "transparent"
                border.color: WidgetLibrary.selectedEntryId !== "" ? Theme.primaryColor : Theme.borderColor
                border.width: 1
                opacity: WidgetLibrary.selectedEntryId !== "" ? 1.0 : 0.4

                Text {
                    anchors.centerIn: parent
                    text: "\u2191"  // Up arrow (share)
                    color: Theme.primaryColor
                    font.pixelSize: Theme.scaled(14)
                    font.bold: true
                }

                MouseArea {
                    id: shareMa
                    anchors.fill: parent
                    enabled: WidgetLibrary.selectedEntryId !== ""
                    onClicked: {
                        LibrarySharing.uploadEntry(WidgetLibrary.selectedEntryId)
                    }
                }
            }
        }

        // Library entries list
        ListView {
            id: libraryList
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.scaled(4)
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            model: {
                if (activeTab === "local") return WidgetLibrary.entries
                if (activeTab === "community") return LibrarySharing.communityEntries
                if (activeTab === "featured") return LibrarySharing.featuredEntries
                if (activeTab === "mine") return LibrarySharing.communityEntries
                return []
            }

            delegate: LibraryItemCard {
                entryData: modelData
                displayMode: libraryPanel.displayMode
                isSelected: WidgetLibrary.selectedEntryId === (modelData.id || "")

                onClicked: {
                    WidgetLibrary.selectedEntryId = modelData.id || ""
                }
                onDoubleClicked: {
                    // TODO: Open apply dialog (zone picker for items/zones)
                    console.log("Apply entry:", modelData.id)
                }
            }

            // Empty state
            Text {
                visible: libraryList.count === 0 && !LibrarySharing.browsing
                anchors.centerIn: parent
                text: {
                    if (activeTab === "local")
                        return "No items in library.\nSelect a widget and click Add."
                    if (activeTab === "mine")
                        return "No shared items yet.\nSelect an entry and click Share."
                    return "No entries found."
                }
                color: Theme.textSecondaryColor
                font: Theme.captionFont
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // Status indicators
        Text {
            visible: LibrarySharing.uploading
            text: "Uploading..."
            color: Theme.primaryColor
            font: Theme.captionFont
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            visible: LibrarySharing.browsing
            text: "Loading..."
            color: Theme.textSecondaryColor
            font: Theme.captionFont
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        // Browse All button (for community/featured tabs)
        AccessibleButton {
            visible: activeTab !== "local"
            Layout.fillWidth: true
            text: "Browse All"
            accessibleName: "Browse all community items"
            onClicked: {
                pageStack.push("qrc:/qml/pages/CommunityBrowserPage.qml")
            }
        }
    }

    // Save dialog
    SaveToLibraryDialog {
        id: saveDialog

        onAccepted: function(name, description, includeTheme) {
            var entryId = ""
            switch (saveDialog.saveType) {
                case "item":
                    entryId = WidgetLibrary.addItemFromLayout(selectedItemId, name, description)
                    break
                case "zone":
                    entryId = WidgetLibrary.addZoneFromLayout(selectedZoneName, name, description)
                    break
                case "layout":
                    entryId = WidgetLibrary.addCurrentLayout(name, description, includeTheme)
                    break
            }
            if (entryId) {
                console.log("Saved to library:", saveDialog.saveType, entryId)
            }
        }
    }

    // Delete confirmation dialog
    Popup {
        id: deleteConfirm
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
        padding: Theme.scaled(16)

        background: Rectangle {
            color: Theme.surfaceColor
            radius: Theme.cardRadius
            border.color: Theme.borderColor
            border.width: 1
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingMedium

            Text {
                text: "Delete this library entry?"
                color: Theme.textColor
                font: Theme.subtitleFont
            }

            Text {
                text: {
                    var entry = WidgetLibrary.getEntry(WidgetLibrary.selectedEntryId)
                    return entry.name || ""
                }
                color: Theme.textSecondaryColor
                font: Theme.bodyFont
            }

            RowLayout {
                spacing: Theme.spacingSmall
                Item { Layout.fillWidth: true }

                AccessibleButton {
                    text: "Cancel"
                    accessibleName: "Cancel deletion"
                    onClicked: deleteConfirm.close()
                }

                AccessibleButton {
                    text: "Delete"
                    accessibleName: "Confirm delete"
                    onClicked: {
                        WidgetLibrary.removeEntry(WidgetLibrary.selectedEntryId)
                        deleteConfirm.close()
                    }
                }
            }
        }
    }
}
