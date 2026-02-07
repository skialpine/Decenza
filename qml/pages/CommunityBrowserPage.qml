import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DecenzaDE1
import "../components"
import "../components/library"

Item {
    id: communityBrowser
    objectName: "communityBrowserPage"

    // Filter state
    property string filterType: ""
    property string filterVariable: ""
    property string filterAction: ""
    property string searchText: ""
    property string sortBy: "newest"
    property int currentPage: 1

    Component.onCompleted: refreshResults()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.scaled(12)
        spacing: Theme.spacingMedium

        // Header with back button
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMedium

            AccessibleButton {
                text: "\u2190 Back"
                accessibleName: "Back to settings"
                onClicked: pageStack.pop()
            }

            Text {
                text: "Community Library"
                color: Theme.textColor
                font: Theme.titleFont
                Layout.fillWidth: true
            }

            Text {
                visible: LibrarySharing.totalCommunityResults > 0
                text: LibrarySharing.totalCommunityResults + " entries"
                color: Theme.textSecondaryColor
                font: Theme.captionFont
            }
        }

        // Filter bar
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.scaled(8)

            // Type filter
            ComboBox {
                id: typeFilter
                Layout.preferredWidth: Theme.scaled(120)
                model: ["All Types", "Items", "Zones", "Layouts"]
                onCurrentIndexChanged: {
                    var types = ["", "item", "zone", "layout"]
                    filterType = types[currentIndex]
                    refreshResults()
                }
            }

            // Variable filter
            ComboBox {
                id: variableFilter
                Layout.preferredWidth: Theme.scaled(140)
                model: ["Any Variable", "%TEMP%", "%STEAM_TEMP%", "%PRESSURE%",
                        "%FLOW%", "%WEIGHT%", "%WATER%", "%SHOT_TIME%",
                        "%PROFILE%", "%STATE%", "%TIME%", "%DATE%",
                        "%RATIO%", "%DOSE%", "%TARGET_WEIGHT%"]
                onCurrentIndexChanged: {
                    filterVariable = currentIndex > 0 ? currentText : ""
                    refreshResults()
                }
            }

            // Action filter
            ComboBox {
                id: actionFilter
                Layout.preferredWidth: Theme.scaled(140)
                model: ["Any Action", "navigate:settings", "navigate:history",
                        "navigate:autofavorites", "navigate:steam",
                        "navigate:hotwater", "navigate:flush",
                        "command:sleep", "command:quit", "command:tare",
                        "command:startEspresso", "command:stop"]
                onCurrentIndexChanged: {
                    filterAction = currentIndex > 0 ? currentText : ""
                    refreshResults()
                }
            }

            // Search field
            StyledTextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search..."
                onTextChanged: {
                    searchTimer.restart()
                }
            }

            // Sort
            ComboBox {
                id: sortFilter
                Layout.preferredWidth: Theme.scaled(120)
                model: ["Newest", "Most Popular", "Name A-Z"]
                onCurrentIndexChanged: {
                    var sorts = ["newest", "popular", "name"]
                    sortBy = sorts[currentIndex]
                    refreshResults()
                }
            }
        }

        // Loading indicator
        Text {
            visible: LibrarySharing.browsing
            text: "Loading..."
            color: Theme.primaryColor
            font: Theme.bodyFont
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        // Results grid
        GridView {
            id: resultsGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            cellWidth: Theme.scaled(280)
            cellHeight: Theme.scaled(80)
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            model: LibrarySharing.communityEntries

            delegate: LibraryItemCard {
                width: resultsGrid.cellWidth - Theme.scaled(8)
                height: resultsGrid.cellHeight - Theme.scaled(8)
                entryData: modelData
                displayMode: 0

                onClicked: {
                    LibrarySharing.downloadEntry(modelData.id)
                }
            }

            // Load more when reaching bottom
            onAtYEndChanged: {
                if (atYEnd && count > 0 && !LibrarySharing.browsing) {
                    var totalPages = Math.ceil(LibrarySharing.totalCommunityResults / 20)
                    if (currentPage < totalPages) {
                        currentPage++
                        LibrarySharing.browseCommunity(filterType, filterVariable,
                            filterAction, searchText, sortBy, currentPage)
                    }
                }
            }

            // Empty state
            Text {
                visible: resultsGrid.count === 0 && !LibrarySharing.browsing
                anchors.centerIn: parent
                text: searchText || filterType || filterVariable || filterAction
                    ? "No entries match your filters."
                    : "Community library is empty.\nBe the first to share a widget!"
                color: Theme.textSecondaryColor
                font: Theme.bodyFont
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // Debounce search input
    Timer {
        id: searchTimer
        interval: 400
        onTriggered: {
            searchText = searchField.text
            refreshResults()
        }
    }

    // Download feedback
    Connections {
        target: LibrarySharing
        function onDownloadComplete(localEntryId) {
            downloadToast.text = "Downloaded to library!"
            downloadToast.visible = true
            downloadToastTimer.restart()
        }
        function onDownloadFailed(error) {
            downloadToast.text = "Download failed: " + error
            downloadToast.visible = true
            downloadToastTimer.restart()
        }
    }

    // Simple toast notification
    Rectangle {
        id: downloadToast
        visible: false
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.scaled(20)
        anchors.horizontalCenter: parent.horizontalCenter
        width: toastText.implicitWidth + Theme.scaled(32)
        height: Theme.scaled(40)
        radius: Theme.scaled(20)
        color: Theme.surfaceColor
        border.color: Theme.borderColor
        border.width: 1

        property alias text: toastText.text

        Text {
            id: toastText
            anchors.centerIn: parent
            color: Theme.textColor
            font: Theme.bodyFont
        }
    }

    Timer {
        id: downloadToastTimer
        interval: 3000
        onTriggered: downloadToast.visible = false
    }

    function refreshResults() {
        currentPage = 1
        LibrarySharing.browseCommunity(filterType, filterVariable,
            filterAction, searchText, sortBy, currentPage)
    }
}
