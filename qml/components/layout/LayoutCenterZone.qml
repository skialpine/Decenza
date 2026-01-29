import QtQuick
import QtQuick.Layouts

Item {
    id: root

    required property string zoneName
    required property var items

    // Calculate button sizing like current IdlePage
    readonly property int buttonCount: items ? items.length : 0
    readonly property real availableWidth: width - Theme.scaled(20) -
        (buttonCount > 1 ? (buttonCount - 1) * Theme.scaled(10) : 0)
    readonly property real buttonWidth: buttonCount > 0
        ? Math.min(Theme.scaled(150), availableWidth / buttonCount) : Theme.scaled(150)
    readonly property real buttonHeight: Theme.scaled(120)

    implicitHeight: contentRow.implicitHeight

    Component.onCompleted: {
        console.log("[IdlePage] LayoutCenterZone", zoneName, "items count:", items ? items.length : "null", "size:", width, "x", height, "buttonWidth:", buttonWidth, "buttonHeight:", buttonHeight)
    }

    RowLayout {
        id: contentRow
        anchors.centerIn: parent
        spacing: Theme.scaled(10)

        Repeater {
            model: root.items
            onCountChanged: console.log("[IdlePage] LayoutCenterZone", root.zoneName, "Repeater count:", count)
            delegate: LayoutItemDelegate {
                zoneName: root.zoneName
                Layout.preferredWidth: root.buttonWidth
                Layout.preferredHeight: root.buttonHeight
            }
        }
    }
}
