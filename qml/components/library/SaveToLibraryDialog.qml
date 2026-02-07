import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DecenzaDE1
import ".."

Popup {
    id: dialog

    property string saveType: "item"  // "item", "zone", "layout"
    property bool showThemeCheckbox: false

    signal accepted(string name, string description, bool includeTheme)

    anchors.centerIn: parent
    width: Theme.scaled(320)
    modal: true
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    onAboutToShow: {
        nameField.text = ""
        descField.text = ""
        themeCheck.checked = false
        nameField.forceActiveFocus()
    }

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.cardRadius
        border.color: Theme.borderColor
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingMedium

        // Title
        Text {
            text: {
                switch (dialog.saveType) {
                    case "item": return "Save Item to Library"
                    case "zone": return "Save Zone to Library"
                    case "layout": return "Save Layout to Library"
                    default: return "Save to Library"
                }
            }
            color: Theme.textColor
            font: Theme.subtitleFont
        }

        // Name field
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.scaled(4)

            Text {
                text: "Name"
                color: Theme.textSecondaryColor
                font: Theme.captionFont
            }

            StyledTextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "Give it a name..."
            }
        }

        // Description field
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.scaled(4)

            Text {
                text: "Description (optional)"
                color: Theme.textSecondaryColor
                font: Theme.captionFont
            }

            StyledTextField {
                id: descField
                Layout.fillWidth: true
                placeholderText: "What does it do?"
            }
        }

        // Include theme checkbox (only for layouts)
        RowLayout {
            visible: dialog.showThemeCheckbox
            spacing: Theme.spacingSmall

            CheckBox {
                id: themeCheck
                checked: false
            }

            Text {
                text: "Include current theme colors"
                color: Theme.textColor
                font: Theme.bodyFont
                MouseArea {
                    anchors.fill: parent
                    onClicked: themeCheck.checked = !themeCheck.checked
                }
            }
        }

        // Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            Item { Layout.fillWidth: true }

            AccessibleButton {
                text: "Cancel"
                accessibleName: "Cancel"
                onClicked: dialog.close()
            }

            AccessibleButton {
                text: "Save"
                accessibleName: "Save to library"
                enabled: nameField.text.trim().length > 0
                onClicked: {
                    dialog.accepted(nameField.text.trim(), descField.text.trim(), themeCheck.checked)
                    dialog.close()
                }
            }
        }
    }
}
