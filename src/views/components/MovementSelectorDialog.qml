// MovementSelectorDialog: modal dialog wrapper for MovementSelectorContent,
// shown from GameTimelineScreen when timeline_selectors_as_dialogs config flag is enabled.
import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {
    id: root

    modal: true
    anchors.centerIn: parent
    width: 700
    padding: 0

    signal movementSelected(string name, string type)

    background: Rectangle {
        radius: 20
        color: "#ffffff"
        border.color: Config.get("color_primary") || "#2d5a2d"
        border.width: 2
    }

    contentItem: Column {
        spacing: 12
        anchors.margins: 20
        width: root.width - 40

        Text {
            text: qsTr("Select Movement")
            font.bold: true
            font.pixelSize: 18
            color: Config.get("color_primary") || "#2d5a2d"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Loader {
            active: root.visible
            width: parent.width
            height: 350
            sourceComponent: MovementSelectorContent {
                padSpacing: 40
                movementPadSize: 260
                actionPadSize: 260
                onMovementSelected: (name, type) => {
                    root.movementSelected(name, type)
                    root.close()
                }
            }
        }

        Button {
            text: qsTr("Close")
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: root.close()
            background: Rectangle {
                color: parent.down ? (Config.get("color_accent_pressed") || "#17736c") : (Config.get("color_accent") || "#21a69b")
                radius: 8
            }
            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
