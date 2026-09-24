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

    // movementPad D-pad layout (exact replica of PadScreen.qml)
    property real movementPadSize: 290      // Fixed panel size (matches PadScreen's rectangleWidth)
    property real turnButtonSize: 80        // Turn Left/Right button size
    property real dpadButtonSize: 110       // Walk Forward/Backward, Moonwalker L/R size (matches buttonPadSize)
    property real turnRowSpacing: 100       // Spacing between Turn Left and Turn Right
    property real dpadRowSpacing: 50        // Spacing between Moonwalker Left and Right
    property real dpadVerticalOffset: 40    // Walk F/B offset toward center
    property real dpadHorizontalOffset: 10  // Moonwalker L/R offset toward center

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
                width: root.movementPadSize
                height: root.movementPadSize
                color: Config.get("color_bg_connected") || "#e8f5e8"
                radius: 15
                border.color: Config.get("color_primary") || "#2d5a2d"
                border.width: 2

                Row {
                    id: turnRow
                    anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 20 }
                    spacing: root.turnRowSpacing

                    Image {
                        id: turnLeftBtn
                        width: root.turnButtonSize
                        height: root.turnButtonSize
                        source: "qrc:/images/android/pad_turn_left_button.png"
                        sourceSize.width: root.turnButtonSize
                        sourceSize.height: root.turnButtonSize
                        fillMode: Image.PreserveAspectFit

                        MouseArea {
                            id: turnLeftArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onPressed: turnLeftBtn.source = "qrc:/images/android/pressed_pad_turn_left_button.png"
                            onReleased: {
                                turnLeftBtn.source = "qrc:/images/android/pad_turn_left_button.png"
                                root.movementSelected("Turn Left", "movement")
                            }
                        }
                        ToolTip.visible: turnLeftArea.containsMouse
                        ToolTip.text: "Turn Left"
                    }

                    Image {
                        id: turnRightBtn
                        width: root.turnButtonSize
                        height: root.turnButtonSize
                        source: "qrc:/images/android/pad_turn_right_button.png"
                        sourceSize.width: root.turnButtonSize
                        sourceSize.height: root.turnButtonSize
                        fillMode: Image.PreserveAspectFit

                        MouseArea {
                            id: turnRightArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onPressed: turnRightBtn.source = "qrc:/images/android/pressed_pad_turn_right_button.png"
                            onReleased: {
                                turnRightBtn.source = "qrc:/images/android/pad_turn_right_button.png"
                                root.movementSelected("Turn Right", "movement")
                            }
                        }
                        ToolTip.visible: turnRightArea.containsMouse
                        ToolTip.text: "Turn Right"
                    }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 0

                    Image {
                        id: upBtn
                        width: root.dpadButtonSize
                        height: root.dpadButtonSize
                        transform: Translate { y: root.dpadVerticalOffset }
                        source: "qrc:/images/android/pad_walk_forward.png"
                        sourceSize.width: root.dpadButtonSize
                        sourceSize.height: root.dpadButtonSize
                        fillMode: Image.PreserveAspectFit
                        anchors.horizontalCenter: parent.horizontalCenter

                        MouseArea {
                            id: upArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onPressed: upBtn.source = "qrc:/images/android/pressed_pad_walk_forward.png"
                            onReleased: {
                                upBtn.source = "qrc:/images/android/pad_walk_forward.png"
                                root.movementSelected("Walk Forward", "movement")
                            }
                        }
                        ToolTip.visible: upArea.containsMouse
                        ToolTip.text: "Walk Forward"
                    }

                    Row {
                        spacing: root.dpadRowSpacing
                        anchors.horizontalCenter: parent.horizontalCenter

                        Image {
                            id: leftBtn
                            width: root.dpadButtonSize
                            height: root.dpadButtonSize
                            transform: Translate { x: root.dpadHorizontalOffset }
                            source: "qrc:/images/android/pad_moonwalker_left.png"
                            sourceSize.width: root.dpadButtonSize
                            sourceSize.height: root.dpadButtonSize
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                id: leftArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onPressed: leftBtn.source = "qrc:/images/android/pressed_pad_moonwalker_left.png"
                                onReleased: {
                                    leftBtn.source = "qrc:/images/android/pad_moonwalker_left.png"
                                    root.movementSelected("Moonwalker Left", "movement")
                                }
                            }
                            ToolTip.visible: leftArea.containsMouse
                            ToolTip.text: "Moonwalker Left"
                        }

                        Image {
                            id: rightBtn
                            width: root.dpadButtonSize
                            height: root.dpadButtonSize
                            transform: Translate { x: -root.dpadHorizontalOffset }
                            source: "qrc:/images/android/pad_moonwalker_right.png"
                            sourceSize.width: root.dpadButtonSize
                            sourceSize.height: root.dpadButtonSize
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                id: rightArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onPressed: rightBtn.source = "qrc:/images/android/pressed_pad_moonwalker_right.png"
                                onReleased: {
                                    rightBtn.source = "qrc:/images/android/pad_moonwalker_right.png"
                                    root.movementSelected("Moonwalker Right", "movement")
                                }
                            }
                            ToolTip.visible: rightArea.containsMouse
                            ToolTip.text: "Moonwalker Right"
                        }
                    }

                    Image {
                        id: downBtn
                        width: root.dpadButtonSize
                        height: root.dpadButtonSize
                        transform: Translate { y: -root.dpadVerticalOffset }
                        source: "qrc:/images/android/pad_walk_backward.png"
                        sourceSize.width: root.dpadButtonSize
                        sourceSize.height: root.dpadButtonSize
                        fillMode: Image.PreserveAspectFit
                        anchors.horizontalCenter: parent.horizontalCenter

                        MouseArea {
                            id: downArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onPressed: downBtn.source = "qrc:/images/android/pressed_pad_walk_backward.png"
                            onReleased: {
                                downBtn.source = "qrc:/images/android/pad_walk_backward.png"
                                root.movementSelected("Walk Backward", "movement")
                            }
                        }
                        ToolTip.visible: downArea.containsMouse
                        ToolTip.text: "Walk Backward"
                    }
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