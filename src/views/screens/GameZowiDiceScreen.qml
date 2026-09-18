// GameZowiDiceScreen: "Zowi Dice" (Simon-like memory game)
// Zowi plays a growing random sequence of 4 moves; player must repeat from memory.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "GameZowiDiceScreen"
    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: true
    showDisconnectButton: true
    footerHeight: 140

    property int currentSpeed: 1000
    property string speedName: tr("speed_medium")

    function tr(source) { return Translator.translate("GameZowiDiceScreen.qml", source) }

    // Pause identity poll while game is running (like PadScreen does)
    Component.onCompleted: {
        Robot.setDataPollingEnabled(false)
        ZowiDice.startGame()
    }
    Component.onDestruction: Robot.setDataPollingEnabled(true)

    // ─── Action Buttons (2×2 grid) ──────────────────────────────────────────
    Item {
        id: actionGrid
        anchors {
            centerIn: parent
            margins: 20
        }
        width: Math.min(parent.width, parent.height) * 0.85
        height: width

        property real btnSize: width / 2 - 10

        // Top-Left: Walk Forward
        Image {
            id: btnWalkForward
            x: 0
            y: 0
            width: parent.btnSize
            height: parent.btnSize
            source: "qrc:/images/android/move1_button.png"
            sourceSize.width: parent.btnSize
            sourceSize.height: parent.btnSize
            fillMode: Image.PreserveAspectFit
            opacity: ZowiDice.blockUserInput || !Robot.connected ? 0.4 : 1.0

            MouseArea {
                anchors.fill: parent
                enabled: !ZowiDice.blockUserInput && Robot.connected
                onPressed: parent.source = "qrc:/images/android/pressed_move1_button.png"
                onReleased: parent.source = "qrc:/images/android/move1_button.png"
                onClicked: ZowiDice.onActionTopLeft()
            }
        }

        // Top-Right: Bend Backward
        Image {
            id: btnBendBackward
            x: parent.btnSize + 20
            y: 0
            width: parent.btnSize
            height: parent.btnSize
            source: "qrc:/images/android/move2_button.png"
            sourceSize.width: parent.btnSize
            sourceSize.height: parent.btnSize
            fillMode: Image.PreserveAspectFit
            opacity: ZowiDice.blockUserInput || !Robot.connected ? 0.4 : 1.0

            MouseArea {
                anchors.fill: parent
                enabled: !ZowiDice.blockUserInput && Robot.connected
                onPressed: parent.source = "qrc:/images/android/pressed_move2_button.png"
                onReleased: parent.source = "qrc:/images/android/move2_button.png"
                onClicked: ZowiDice.onActionTopRight()
            }
        }

        // Bottom-Left: Jump
        Image {
            id: btnJump
            x: 0
            y: parent.btnSize + 20
            width: parent.btnSize
            height: parent.btnSize
            source: "qrc:/images/android/move3_button.png"
            sourceSize.width: parent.btnSize
            sourceSize.height: parent.btnSize
            fillMode: Image.PreserveAspectFit
            opacity: ZowiDice.blockUserInput || !Robot.connected ? 0.4 : 1.0

            MouseArea {
                anchors.fill: parent
                enabled: !ZowiDice.blockUserInput && Robot.connected
                onPressed: parent.source = "qrc:/images/android/pressed_move3_button.png"
                onReleased: parent.source = "qrc:/images/android/move3_button.png"
                onClicked: ZowiDice.onActionBottomLeft()
            }
        }

        // Bottom-Right: Moonwalker Right
        Image {
            id: btnMoonwalkerRight
            x: parent.btnSize + 20
            y: parent.btnSize + 20
            width: parent.btnSize
            height: parent.btnSize
            source: "qrc:/images/android/move4_button.png"
            sourceSize.width: parent.btnSize
            sourceSize.height: parent.btnSize
            fillMode: Image.PreserveAspectFit
            opacity: ZowiDice.blockUserInput || !Robot.connected ? 0.4 : 1.0

            MouseArea {
                anchors.fill: parent
                enabled: !ZowiDice.blockUserInput && Robot.connected
                onPressed: parent.source = "qrc:/images/android/pressed_move4_button.png"
                onReleased: parent.source = "qrc:/images/android/move4_button.png"
                onClicked: ZowiDice.onActionBottomRight()
            }
        }
    }

    // ─── Footer: Score, Progress, Play/Help/Ranking ─────────────────────────
    Item {
        id: footer
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: root.footerHeight

        // Score display
        Text {
            id: scoreText
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
                topMargin: 10
            }
            text: tr("score_prefix").arg(ZowiDice.score)
            color: Config.get("color_primary") || "#2d5a2d"
            font.pixelSize: 20
            font.bold: true
        }

        // Progress bar (visible only during sequence playback)
        Rectangle {
            id: progressContainer
            anchors {
                top: scoreText.bottom
                horizontalCenter: parent.horizontalCenter
                topMargin: 10
            }
            visible: ZowiDice.state === ZowiDice.State.ShowingSequence
            width: parent.width * 0.7
            height: 12
            radius: 6
            color: Config.get("color_bg_disabled") || "#e6e6e6"
            border.color: Config.get("color_accent") || "#21a69b"
            border.width: 1

            Rectangle {
                id: progressBar
                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                }
                height: parent.height
                radius: 5
                color: Config.get("color_accent") || "#21a69b"
                width: progressContainer.width * ZowiDice.progress / 100
                Behavior on width { NumberAnimation { duration: 100 } }
            }
        }

        // Control buttons row
        Row {
            id: controlRow
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
                bottomMargin: 15
            }
            spacing: 20

            // Play button
            Button {
                id: playBtn
                visible: ZowiDice.state === ZowiDice.State.Idle || ZowiDice.state === ZowiDice.State.GameOver
                implicitWidth: 120
                height: 44
                text: tr("play_button")
                font.bold: true
                font.pixelSize: 14

                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 22
                    color: playBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : (Config.get("color_accent") || "#21a69b")
                }

                onClicked: ZowiDice.startGame()
            }

            // Help button
            Button {
                id: helpBtn
                visible: ZowiDice.state === ZowiDice.State.Idle
                implicitWidth: 100
                height: 44
                text: tr("help_button")
                font.pixelSize: 14

                contentItem: Text {
                    text: parent.text
                    color: Config.get("color_primary") || "#2d5a2d"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 22
                    color: helpBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
                    border.color: Config.get("color_accent") || "#21a69b"
                    border.width: 1
                }

                onClicked: helpDialog.open()
            }

            // Ranking button (placeholder for future)
            Button {
                id: rankingBtn
                visible: ZowiDice.state === ZowiDice.State.Idle
                implicitWidth: 100
                height: 44
                text: tr("ranking_button")
                font.pixelSize: 14
                enabled: false

                contentItem: Text {
                    text: parent.text
                    color: Config.get("color_fg_disabled") || "#9e9e9e"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 22
                    color: "transparent"
                    border.color: Config.get("color_border_disabled") || "#c8c8c8"
                    border.width: 1
                }
            }
        }
    }

    // ─── Help Dialog ────────────────────────────────────────────────────────
    Dialog {
        id: helpDialog
        title: tr("help_button")
        modal: true
        standardButtons: Dialog.Close
        width: 400
        onAccepted: close()

        contentItem: Column {
            spacing: 16
            anchors.fill: parent
            anchors.margins: 20

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "qrc:/images/android/simon_game_button.png"
                sourceSize.width: 120
                sourceSize.height: 120
                fillMode: Image.PreserveAspectFit
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 40
                text: tr("how_to_play_text")
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 14
                color: Config.get("color_primary") || "#2d5a2d"
            }
        }
    }

    // ─── Game Over Dialog ───────────────────────────────────────────────────
    Dialog {
        id: gameOverDialog
        title: tr("game_over")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Retry
        width: 320
        onAccepted: {
            if (gameOverDialog.clickedButton === Dialog.Retry) {
                ZowiDice.startGame()
            } else {
                ZowiDice.resetGame()
            }
        }

        contentItem: Column {
            spacing: 16
            anchors.fill: parent
            anchors.margins: 20

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: tr("final_score").arg(ZowiDice.score)
                font.pixelSize: 24
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: (ZowiDice.score >= 12) ? tr("new_best") : ""
                font.pixelSize: 14
                color: Config.get("color_accent") || "#21a69b"
            }
        }
    }

    // React to gameOver signal
    Connections {
        target: ZowiDice
        function onGameOver(score) {
            gameOverDialog.open()
        }
    }
}