// MovementSelectorContent: reusable movement-selection D-pad + action grid,
// extracted from MovementSelector.qml for use in both full-screen ScreenTemplate and Dialog modes.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    signal movementSelected(string name, string type)

    function tr(source) { return Translator.translate("MovementSelector.qml", source) }

    property real padSpacing: 60

    // movementPad D-pad layout (exact replica of PadScreen.qml)
    property real movementPadSize: 290
    property real turnButtonSize: 80
    property real dpadButtonSize: 110
    property real turnRowSpacing: 100
    property real dpadRowSpacing: 50
    property real dpadVerticalOffset: 40
    property real dpadHorizontalOffset: 10

    // actionPad layout (exact replica of PadScreen.qml)
    property real actionPadSize: 290
    property real actionButtonSize: 80
    property real actionRowSpacing: 5

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: padsRow.implicitHeight + 40
        clip: true

        RowLayout {
            id: padsRow
            anchors.centerIn: parent
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
                width: root.actionPadSize
                height: root.actionPadSize
                color: Config.get("color_bg_connected") || "#e8f5e8"
                radius: 15
                border.color: Config.get("color_primary") || "#2d5a2d"
                border.width: 2

                Column {
                    anchors.centerIn: parent
                    spacing: root.actionRowSpacing

                    Row {
                        spacing: root.actionRowSpacing
                        anchors.horizontalCenter: parent.horizontalCenter

                        Image {
                            id: bendBtn
                            width: root.actionButtonSize
                            height: root.actionButtonSize
                            source: "qrc:/images/android/pad_bend_button.png"
                            sourceSize.width: root.actionButtonSize
                            sourceSize.height: root.actionButtonSize
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                id: bendArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onPressed: bendBtn.source = "qrc:/images/android/pressed_pad_bend_button.png"
                                onReleased: {
                                    bendBtn.source = "qrc:/images/android/pad_bend_button.png"
                                    root.movementSelected("Bend Forward", "movement")
                                }
                            }
                            ToolTip.visible: bendArea.containsMouse
                            ToolTip.text: "Bend Forward"
                        }

                        Image {
                            id: shakeLegBtn
                            width: root.actionButtonSize
                            height: root.actionButtonSize
                            source: "qrc:/images/android/pad_shake_leg_button.png"
                            sourceSize.width: root.actionButtonSize
                            sourceSize.height: root.actionButtonSize
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                id: shakeLegArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onPressed: shakeLegBtn.source = "qrc:/images/android/pressed_pad_shake_leg_button.png"
                                onReleased: {
                                    shakeLegBtn.source = "qrc:/images/android/pad_shake_leg_button.png"
                                    root.movementSelected("Shake Leg", "movement")
                                }
                            }
                            ToolTip.visible: shakeLegArea.containsMouse
                            ToolTip.text: "Shake Leg"
                        }
                    }

                    Row {
                        spacing: root.actionRowSpacing
                        anchors.horizontalCenter: parent.horizontalCenter

                        Image {
                            id: updownBtn
                            width: root.actionButtonSize
                            height: root.actionButtonSize
                            source: "qrc:/images/android/pad_updown_button.png"
                            sourceSize.width: root.actionButtonSize
                            sourceSize.height: root.actionButtonSize
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                id: updownArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onPressed: updownBtn.source = "qrc:/images/android/pressed_pad_updown_button.png"
                                onReleased: {
                                    updownBtn.source = "qrc:/images/android/pad_updown_button.png"
                                    root.movementSelected("Up/Down", "movement")
                                }
                            }
                            ToolTip.visible: updownArea.containsMouse
                            ToolTip.text: "Up/Down"
                        }

                        Image {
                            id: jitterBtn
                            width: root.actionButtonSize
                            height: root.actionButtonSize
                            source: "qrc:/images/android/pad_jitter_button.png"
                            sourceSize.width: root.actionButtonSize
                            sourceSize.height: root.actionButtonSize
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                id: jitterArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onPressed: jitterBtn.source = "qrc:/images/android/pressed_pad_jitter_button.png"
                                onReleased: {
                                    jitterBtn.source = "qrc:/images/android/pad_jitter_button.png"
                                    root.movementSelected("Jitter", "movement")
                                }
                            }
                            ToolTip.visible: jitterArea.containsMouse
                            ToolTip.text: "Jitter"
                        }

                        Image {
                            id: swingBtn
                            width: root.actionButtonSize
                            height: root.actionButtonSize
                            source: "qrc:/images/android/pad_swing_button.png"
                            sourceSize.width: root.actionButtonSize
                            sourceSize.height: root.actionButtonSize
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                id: swingArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onPressed: swingBtn.source = "qrc:/images/android/pressed_pad_swing_button.png"
                                onReleased: {
                                    swingBtn.source = "qrc:/images/android/pad_swing_button.png"
                                    root.movementSelected("Swing", "movement")
                                }
                            }
                            ToolTip.visible: swingArea.containsMouse
                            ToolTip.text: "Swing"
                        }
                    }

                    Row {
                        spacing: root.actionRowSpacing
                        anchors.horizontalCenter: parent.horizontalCenter

                        Image {
                            id: flappingBtn
                            width: root.actionButtonSize
                            height: root.actionButtonSize
                            source: "qrc:/images/android/pad_flapping_button.png"
                            sourceSize.width: root.actionButtonSize
                            sourceSize.height: root.actionButtonSize
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                id: flappingArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onPressed: flappingBtn.source = "qrc:/images/android/pressed_pad_flapping_button.png"
                                onReleased: {
                                    flappingBtn.source = "qrc:/images/android/pad_flapping_button.png"
                                    root.movementSelected("Flapping", "movement")
                                }
                            }
                            ToolTip.visible: flappingArea.containsMouse
                            ToolTip.text: "Flapping"
                        }

                        Image {
                            id: crusaitoBtn
                            width: root.actionButtonSize
                            height: root.actionButtonSize
                            source: "qrc:/images/android/pad_crusaito_button.png"
                            sourceSize.width: root.actionButtonSize
                            sourceSize.height: root.actionButtonSize
                            fillMode: Image.PreserveAspectFit

                            MouseArea {
                                id: crusaitoArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onPressed: crusaitoBtn.source = "qrc:/images/android/pressed_pad_crusaito_button.png"
                                onReleased: {
                                    crusaitoBtn.source = "qrc:/images/android/pad_crusaito_button.png"
                                    root.movementSelected("Crusaito", "movement")
                                }
                            }
                            ToolTip.visible: crusaitoArea.containsMouse
                            ToolTip.text: "Crusaito"
                        }
                    }
                }
            }
        }
    }
}
