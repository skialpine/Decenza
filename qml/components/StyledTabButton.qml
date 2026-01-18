import QtQuick
import QtQuick.Controls
import DecenzaDE1

// Styled tab button with consistent appearance
// Uses built-in 'checked' property (set automatically by TabBar)
TabButton {
    id: root

    // For accessibility - builds accessible name from text and selection state
    property string tabLabel: ""  // Set for translated accessibility
    property string accessibleName: {
        var label = tabLabel !== "" ? tabLabel : root.text
        var tabStr = TranslationManager.translate("common.tab", "tab")
        var selectedStr = root.checked ? ", " + TranslationManager.translate("common.selected", "selected") : ""
        return label + " " + tabStr + selectedStr
    }

    width: implicitWidth
    leftPadding: Theme.scaled(8)
    rightPadding: Theme.scaled(8)
    topPadding: Theme.scaled(6)
    bottomPadding: Theme.scaled(6)
    font.pixelSize: Theme.scaled(13)
    font.bold: root.checked

    contentItem: Text {
        text: root.text
        font: root.font
        color: root.checked ? Theme.textColor : Theme.textSecondaryColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        color: root.checked ? Theme.surfaceColor : "transparent"
        radius: Theme.scaled(6)

        Behavior on color {
            ColorAnimation { duration: 100 }
        }
    }

    Accessible.role: Accessible.PageTab
    Accessible.name: root.accessibleName
    Accessible.focusable: true

    // Focus indicator
    FocusIndicator {
        targetItem: root
        visible: root.activeFocus
    }
}
