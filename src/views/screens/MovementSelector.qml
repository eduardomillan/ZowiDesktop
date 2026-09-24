// MovementSelector: grid of Zowi movements to add to the GameTimelineScreen
// sequence. Tap a movement to emit movementSelected(name, "movement"); the
// owning screen appends it to the timeline. Mirror of Android's commands grid.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "MovementSelector"
    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: true

    function tr(source) { return Translator.translate("MovementSelector.qml", source) }

    signal movementSelected(string name, string type)

    readonly property var movementPadOptions: [
        { name: "Walk Forward",  icon: "qrc:/images/android/pad_walk_forward.png" },
        { name: "Walk Backward", icon: "qrc:/images/android/pad_walk_backward.png" },
        { name: "Turn Left",     icon: "qrc:/images/android/pad_turn_left_button.png" },
        { name: "Turn Right",    icon: "qrc:/images/android/pad_turn_right_button.png" },
        { name: "Moonwalker Left",  icon: "qrc:/images/android/pad_moonwalker_left.png" },
        { name: "Moonwalker Right", icon: "qrc:/images/android/pad_moonwalker_right.png" }
    ]
    readonly property var actionPadOptions: [
        { name: "Bend Forward",  icon: "qrc:/images/android/pad_bend_button.png" },
        { name: "Shake Leg",     icon: "qrc:/images/android/pad_shake_leg_button.png" },
        { name: "Up/Down",       icon: "qrc:/images/android/pad_updown_button.png" },
        { name: "Jitter",        icon: "qrc:/images/android/pad_jitter_button.png" },
        { name: "Swing",         icon: "qrc:/images/android/pad_swing_button.png" },
        { name: "Flapping",      icon: "qrc:/images/android/pad_flapping_button.png" },
        { name: "Crusaito",      icon: "qrc:/images/android/pad_crusaito_button.png" }
    ]

    property real buttonSize: 80        // Configurable button size
    property real cellSpacing: 14
    property int padColumns: 2           // Columns per pad grid
    property real padSpacing: 60         // Separation between movementPad and actionPad

    Component {
        id: tileDelegate
        Column {
            width: root.buttonSize + 8
            spacing: 4

            Rectangle {
                width: root.buttonSize
                height: root.buttonSize
                radius: Math.min(root.buttonSize * 0.2, 16)
                color: mvMouse.containsMouse
                       ? (Config.get("color_bg_hover") || "#e0f0e0")
                       : "#ffffff"
                border.color: Config.get("color_accent") || "#21a69b"
                border.width: 1

                Image {
                    anchors.centerIn: parent
                    width: root.buttonSize * 0.7
                    height: root.buttonSize * 0.7
                    source: modelData.icon
                    sourceSize: Qt.size(root.buttonSize * 2, root.buttonSize * 2)
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    id: mvMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.movementSelected(modelData.name, "movement")
                }
            }

            Text {
                text: modelData.name
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: Math.max(9, root.buttonSize * 0.16)
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                width: root.buttonSize + 8
            }
        }
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: padsRow.implicitHeight + 40
        clip: true

        RowLayout {
            id: padsRow
            anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 20 }
            spacing: root.padSpacing

            Rectangle {
                id: movementPad
                Layout.alignment: Qt.AlignVCenter
                width: movementGrid.implicitWidth + 30
                height: movementGrid.implicitHeight + 30
                color: Config.get("color_bg_connected") || "#e8f5e8"
                radius: 15
                border.color: Config.get("color_primary") || "#2d5a2d"
                border.width: 2

                Grid {
                    id: movementGrid
                    anchors.centerIn: parent
                    columns: root.padColumns
                    spacing: root.cellSpacing
                    Repeater { model: root.movementPadOptions; delegate: tileDelegate }
                }
            }

            Rectangle {
                id: actionPad
                Layout.alignment: Qt.AlignVCenter
                width: actionGrid.implicitWidth + 30
                height: actionGrid.implicitHeight + 30
                color: Config.get("color_bg_connected") || "#e8f5e8"
                radius: 15
                border.color: Config.get("color_primary") || "#2d5a2d"
                border.width: 2

                Grid {
                    id: actionGrid
                    anchors.centerIn: parent
                    columns: root.padColumns
                    spacing: root.cellSpacing
                    Repeater { model: root.actionPadOptions; delegate: tileDelegate }
                }
            }
        }
    }
}