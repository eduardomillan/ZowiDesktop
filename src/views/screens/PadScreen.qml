// PadScreen: Interactive gamepad to control Zowi robot
// Movement controls (arrows + turns) + action buttons + speed control
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: pad
    screenName: "PadScreen"
    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: true

    function tr(source) { return Translator.translate("PadScreen.qml", source) }

    property int currentSpeed: 1000
    property string speedName: tr("speed_medium")
    property string currentCommand: ""
    property string currentAction: ""
    property int buttonSize: 110

    Timer {
        id: repeatTimer
        interval: 200
        repeat: true
        onTriggered: {
            if (currentCommand !== "") {
                sendCommand(currentCommand, currentAction)
            }
        }
    }

    function startHold(cmd, actionName) {
        currentCommand = cmd
        currentAction = actionName
        sendCommand(cmd, actionName)
        repeatTimer.start()
    }

    function stopHold() {
        repeatTimer.stop()
        currentCommand = ""
        currentAction = ""
        sendStop()
    }

    function sendCommand(cmd, actionName) {
        if (Robot.connected) {
            Robot.sendData(cmd)
            console.log("[PadScreen] " + actionName + ": " + cmd)
        } else {
            console.log("[PadScreen] Not connected, cannot send: " + actionName)
        }
    }

    function sendStop() {
        if (Robot.connected) {
            Robot.sendData("S\r")
        }
    }

    function setSpeed(speed, name) {
        currentSpeed = speed
        speedName = name
    }

    Rectangle {
        id: movementPad
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
            margins: 20
        }
        width: 300
        height: 300
        color: "#e8f5e8"
        radius: 15
        border.color: "#2d5a2d"
        border.width: 2

        Row {
            id: turnRow
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
                topMargin: 20
            }
            spacing: 100

            Image {
                id: turnLeftBtn
                width: 80
                height: 80
                source: "qrc:/images/android/pad_turn_left_button.png"
                sourceSize.width: 80
                sourceSize.height: 80
                fillMode: Image.PreserveAspectFit

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        parent.source = "qrc:/images/android/pressed_pad_turn_left_button.png"
                        startHold("M 3 " + currentSpeed + "\r", tr("turn_left"))
                    }
                    onReleased: {
                        parent.source = "qrc:/images/android/pad_turn_left_button.png"
                        stopHold()
                    }
                }
            }

            Image {
                id: turnRightBtn
                width: 80
                height: 80
                source: "qrc:/images/android/pad_turn_right_button.png"
                sourceSize.width: 80
                sourceSize.height: 80
                fillMode: Image.PreserveAspectFit

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        parent.source = "qrc:/images/android/pressed_pad_turn_right_button.png"
                        startHold("M 4 " + currentSpeed + "\r", tr("turn_right"))
                    }
                    onReleased: {
                        parent.source = "qrc:/images/android/pad_turn_right_button.png"
                        stopHold()
                    }
                }
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 0

            Image {
                id: upBtn
                width: buttonSize
                height: buttonSize
                transform: Translate { y: 40 }
                source: "qrc:/images/android/pad_walk_forward.png"
                sourceSize.width: buttonSize
                sourceSize.height: buttonSize
                fillMode: Image.PreserveAspectFit
                anchors.horizontalCenter: parent.horizontalCenter

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        parent.source = "qrc:/images/android/pressed_pad_walk_forward.png"
                        startHold("M 1 " + currentSpeed + "\r", tr("walk_forward"))
                    }
                    onReleased: {
                        parent.source = "qrc:/images/android/pad_walk_forward.png"
                        stopHold()
                    }
                }
            }

            Row {
                spacing: 50
                anchors.horizontalCenter: parent.horizontalCenter

                Image {
                    id: leftBtn
                    width: buttonSize
                    height: buttonSize
                    transform: Translate { x: 10 }
                    source: "qrc:/images/android/pad_moonwalker_left.png"
                    sourceSize.width: buttonSize
                    sourceSize.height: buttonSize
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            parent.source = "qrc:/images/android/pressed_pad_moonwalker_left.png"
                            startHold("M 6 " + currentSpeed + " 30\r", tr("moonwalker_left"))
                        }
                        onReleased: {
                            parent.source = "qrc:/images/android/pad_moonwalker_left.png"
                            stopHold()
                        }
                    }
                }

                Image {
                    id: rightBtn
                    width: buttonSize
                    height: buttonSize
                    transform: Translate { x: -10 }
                    source: "qrc:/images/android/pad_moonwalker_right.png"
                    sourceSize.width: buttonSize
                    sourceSize.height: buttonSize
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            parent.source = "qrc:/images/android/pressed_pad_moonwalker_right.png"
                            startHold("M 7 " + currentSpeed + " 30\r", tr("moonwalker_right"))
                        }
                        onReleased: {
                            parent.source = "qrc:/images/android/pad_moonwalker_right.png"
                            stopHold()
                        }
                    }
                }
            }

            Image {
                id: downBtn
                width: buttonSize
                height: buttonSize
                transform: Translate { y: -40 }
                source: "qrc:/images/android/pad_walk_backward.png"
                sourceSize.width: buttonSize
                sourceSize.height: buttonSize
                fillMode: Image.PreserveAspectFit
                anchors.horizontalCenter: parent.horizontalCenter

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        parent.source = "qrc:/images/android/pressed_pad_walk_backward.png"
                        startHold("M 2 " + currentSpeed + "\r", tr("walk_backward"))
                    }
                    onReleased: {
                        parent.source = "qrc:/images/android/pad_walk_backward.png"
                        stopHold()
                    }
                }
            }
        }
    }

    Rectangle {
        id: actionPad
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: 20
        }
        width: 300
        height: 300
        color: "#e8f5e8"
        radius: 15
        border.color: "#2d5a2d"
        border.width: 2

        Column {
            anchors.centerIn: parent
            spacing: 5

            Row {
                spacing: 5
                anchors.horizontalCenter: parent.horizontalCenter

                Image {
                    id: bendBtn
                    width: 80
                    height: 80
                    source: "qrc:/images/android/pad_bend_button.png"
                    sourceSize.width: 80
                    sourceSize.height: 80
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            parent.source = "qrc:/images/android/pressed_pad_bend_button.png"
                            startHold("M 15 " + currentSpeed + "\r", tr("bend"))
                        }
                        onReleased: {
                            parent.source = "qrc:/images/android/pad_bend_button.png"
                            stopHold()
                        }
                    }
                }

                Image {
                    id: shakeLegBtn
                    width: 80
                    height: 80
                    source: "qrc:/images/android/pad_shake_leg_button.png"
                    sourceSize.width: 80
                    sourceSize.height: 80
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            parent.source = "qrc:/images/android/pressed_pad_shake_leg_button.png"
                            startHold("M 17 " + currentSpeed + "\r", tr("shake_leg"))
                        }
                        onReleased: {
                            parent.source = "qrc:/images/android/pad_shake_leg_button.png"
                            stopHold()
                        }
                    }
                }
            }

            Row {
                spacing: 5
                anchors.horizontalCenter: parent.horizontalCenter

                Image {
                    id: updownBtn
                    width: 80
                    height: 80
                    source: "qrc:/images/android/pad_updown_button.png"
                    sourceSize.width: 80
                    sourceSize.height: 80
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            parent.source = "qrc:/images/android/pressed_pad_updown_button.png"
                            startHold("M 5 " + currentSpeed + " 15\r", tr("updown"))
                        }
                        onReleased: {
                            parent.source = "qrc:/images/android/pad_updown_button.png"
                            stopHold()
                        }
                    }
                }

                Image {
                    id: jitterBtn
                    width: 80
                    height: 80
                    source: "qrc:/images/android/pad_jitter_button.png"
                    sourceSize.width: 80
                    sourceSize.height: 80
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            parent.source = "qrc:/images/android/pressed_pad_jitter_button.png"
                            startHold("M 19 " + currentSpeed + " 15\r", tr("jitter"))
                        }
                        onReleased: {
                            parent.source = "qrc:/images/android/pad_jitter_button.png"
                            stopHold()
                        }
                    }
                }

                Image {
                    id: swingBtn
                    width: 80
                    height: 80
                    source: "qrc:/images/android/pad_swing_button.png"
                    sourceSize.width: 80
                    sourceSize.height: 80
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            parent.source = "qrc:/images/android/pressed_pad_swing_button.png"
                            startHold("M 8 " + currentSpeed + " 15\r", tr("swing"))
                        }
                        onReleased: {
                            parent.source = "qrc:/images/android/pad_swing_button.png"
                            stopHold()
                        }
                    }
                }
            }

            Row {
                spacing: 5
                anchors.horizontalCenter: parent.horizontalCenter

                Image {
                    id: flappingBtn
                    width: 80
                    height: 80
                    source: "qrc:/images/android/pad_flapping_button.png"
                    sourceSize.width: 80
                    sourceSize.height: 80
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            parent.source = "qrc:/images/android/pressed_pad_flapping_button.png"
                            startHold("M 12 " + currentSpeed + " 30\r", tr("flapping"))
                        }
                        onReleased: {
                            parent.source = "qrc:/images/android/pad_flapping_button.png"
                            stopHold()
                        }
                    }
                }

                Image {
                    id: crusaitoBtn
                    width: 80
                    height: 80
                    source: "qrc:/images/android/pad_crusaito_button.png"
                    sourceSize.width: 80
                    sourceSize.height: 80
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            parent.source = "qrc:/images/android/pressed_pad_crusaito_button.png"
                            startHold("M 9 " + currentSpeed + " 30\r", tr("crusaito"))
                        }
                        onReleased: {
                            parent.source = "qrc:/images/android/pad_crusaito_button.png"
                            stopHold()
                        }
                    }
                }
            }
        }
    }

    Item {
        id: speedControl
        width: 60
        height: 60
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            bottomMargin: 20
        }

        Image {
            id: speedSlowBtn
            anchors.fill: parent
            source: "qrc:/images/android/pad_speed_slow_button.png"
            sourceSize.width: 60
            sourceSize.height: 60
            fillMode: Image.PreserveAspectFit
            visible: currentSpeed === 2000

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    setSpeed(1000, tr("speed_medium"))
                    console.log("[PadScreen] Speed: medium (1000ms)")
                }
            }
        }

        Image {
            id: speedMediumBtn
            anchors.fill: parent
            source: "qrc:/images/android/pad_speed_medium_button.png"
            sourceSize.width: 60
            sourceSize.height: 60
            fillMode: Image.PreserveAspectFit
            visible: currentSpeed === 1000

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    setSpeed(700, tr("speed_fast"))
                    console.log("[PadScreen] Speed: fast (700ms)")
                }
            }
        }

        Image {
            id: speedFastBtn
            anchors.fill: parent
            source: "qrc:/images/android/pad_speed_fast_button.png"
            sourceSize.width: 60
            sourceSize.height: 60
            fillMode: Image.PreserveAspectFit
            visible: currentSpeed === 700

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    setSpeed(2000, tr("speed_slow"))
                    console.log("[PadScreen] Speed: slow (2000ms)")
                }
            }
        }
    }

    Image {
        id: mouthsBtn
        width: 60
        height: 60
        source: "qrc:/images/android/pad_faces_button.png"
        sourceSize.width: 60
        sourceSize.height: 60
        fillMode: Image.PreserveAspectFit
        anchors {
            left: speedControl.left
            bottom: speedControl.top
            bottomMargin: 15
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                console.log("[PadScreen] Mouths button clicked (not yet implemented)")
            }
        }
    }
}
