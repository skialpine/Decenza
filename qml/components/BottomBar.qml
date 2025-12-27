import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DecenzaDE1

Rectangle {
    id: root

    property string title: ""
    property color barColor: Theme.primaryColor
    property bool showBackButton: true
    property string rightText: ""  // Simple right-aligned text
    default property alias content: contentRow.data  // Custom content goes here

    signal backClicked()

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    height: Theme.bottomBarHeight
    color: barColor

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.chartMarginSmall
        anchors.rightMargin: Theme.spacingLarge
        spacing: Theme.spacingMedium

        // Back button (square hitbox, full bar height)
        Item {
            visible: root.showBackButton
            Layout.preferredWidth: Theme.bottomBarHeight
            Layout.preferredHeight: Theme.bottomBarHeight

            Image {
                anchors.centerIn: parent
                source: "qrc:/icons/back.svg"
                sourceSize.width: Theme.scaled(28)
                sourceSize.height: Theme.scaled(28)
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.backClicked()
            }
        }

        Text {
            visible: root.title !== ""
            text: root.title
            color: "white"
            font.pixelSize: Theme.scaled(20)
            font.bold: true
        }

        Item { Layout.fillWidth: true }

        // Custom content area
        RowLayout {
            id: contentRow
            spacing: Theme.spacingMedium
        }

        // Simple right text (alternative to custom content)
        Text {
            visible: root.rightText !== ""
            text: root.rightText
            color: "white"
            font: Theme.bodyFont
        }
    }
}
