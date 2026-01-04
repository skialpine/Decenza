import QtQuick
import QtQuick.Controls
import DecenzaDE1

// Styled button with proper scaling (use AccessibleButton when accessibility name is needed)
Button {
    id: root

    // Optional primary style (filled background)
    property bool primary: false

    implicitHeight: Theme.scaled(36)
    leftPadding: Theme.scaled(16)
    rightPadding: Theme.scaled(16)

    contentItem: Text {
        text: root.text
        font.pixelSize: Theme.scaled(14)
        font.family: Theme.bodyFont.family
        color: {
            if (!root.enabled) return Theme.textSecondaryColor
            if (root.primary) return "white"
            return Theme.textColor
        }
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitHeight: Theme.scaled(36)
        color: {
            if (root.primary) {
                return root.down ? Qt.darker(Theme.primaryColor, 1.1) : Theme.primaryColor
            }
            return root.down ? Qt.darker(Theme.surfaceColor, 1.2) : Theme.surfaceColor
        }
        border.color: root.primary ? "transparent" : (root.enabled ? Theme.borderColor : Qt.darker(Theme.borderColor, 1.2))
        border.width: root.primary ? 0 : 1
        radius: Theme.scaled(6)
    }

    // Focus indicator
    FocusIndicator {
        targetItem: root
        visible: root.activeFocus
    }
}
