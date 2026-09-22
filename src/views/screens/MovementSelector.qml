// MovementSelector: grid of Zowi movements to add to the GameTimelineScreen
// sequence. Tap a movement to emit movementSelected(name, "movement"); the
// owning screen appends it to the timeline. Mirror of Android's commands grid.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "MovementSelector"
    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: true

    function tr(source) { return Translator.translate("MovementSelector.qml", source) }

    signal movementSelected(string name, string type)

    readonly property var movementOptions: [
        { name: "Walk Forward",  icon: "qrc:/images/android/pad_walk_forward.png" },
        { name: "Walk Backward", icon: "qrc:/images/android/pad_walk_backward.png" },
        { name: "Turn Left",     icon: "qrc:/images/android/pad_turn_left_button.png" },
        { name: "Turn Right",    icon: "qrc:/images/android/pad_turn_right_button.png" },
        { name: "Moonwalker Left",  icon: "qrc:/images/android/pad_moonwalker_left.png" },
        { name: "Moonwalker Right", icon: "qrc:/images/android/pad_moonwalker_right.png" },
        { name: "Bend Forward",  icon: "qrc:/images/android/pad_bend_button.png" },
        { name: "Shake Leg",     icon: "qrc:/images/android/pad_shake_leg_button.png" },
        { name: "Up/Down",       icon: "qrc:/images/android/pad_updown_button.png" },
        { name: "Jitter",        icon: "qrc:/images/android/pad_jitter_button.png" },
        { name: "Swing",         icon: "qrc:/images/android/pad_swing_button.png" },
        { name: "Flapping",      icon: "qrc:/images/android/pad_flapping_button.png" },
        { name: "Crusaito",      icon: "qrc:/images/android/pad_crusaito_button.png" }
    ]

    property real iconSize: 72
    property real cellSpacing: 14

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: grid.implicitHeight
        clip: true

        Grid {
            id: grid
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
                margins: 20
            }
            columns: 4
            spacing: root.cellSpacing

            Repeater {
                model: root.movementOptions

                Column {
                    width: root.iconSize + 8
                    spacing: 4

                    Rectangle {
                        width: root.iconSize
                        height: root.iconSize
                        radius: Math.min(root.iconSize * 0.2, 16)
                        color: mvMouse.containsMouse
                               ? (Config.get("color_bg_hover") || "#e0f0e0")
                               : "#ffffff"
                        border.color: Config.get("color_accent") || "#21a69b"
                        border.width: 1

                        Image {
                            anchors.centerIn: parent
                            width: root.iconSize * 0.7
                            height: root.iconSize * 0.7
                            source: modelData.icon
                            sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
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
                        font.pixelSize: Math.max(9, root.iconSize * 0.16)
                        font.bold: true
                        color: Config.get("color_primary") || "#2d5a2d"
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        width: root.iconSize + 8
                    }
                }
            }
        }
    }
}