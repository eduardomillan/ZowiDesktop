// GameMouthsScreen: "Pintabocas" (Draw the mouths).
// Zowi shows a random mouth and the player must draw it on the 6×5 LED grid
// before the countdown expires. Live compare: as soon as the drawn pattern
// matches the target the round is solved (mirrors Android's
// MouthGridLayoutTouchListener → checkLedMouth). Fully playable offline — the
// mouth and gesture commands are cosmetics that are simply not sent when the
// robot is not connected. Intentional desktop deviation from Android: the
// target mouth is also shown as an on-screen miniature for the whole round.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "GameMouthsScreen"
    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: true
    footerHeight: 110

    function tr(source) { return Translator.translate("GameMouthsScreen.qml", source) }

    // Pause the identity poll while the game is open (the mouth/gesture
    // commands go out during play; the E/I/B burst would drain the robot's
    // queue). Help dialog auto-opens on entry per config "mouths_help":
    // "always" → every time, "once" → only the first (mouths_help_seen flag).
    // It never starts the game — the Play button does.
    Component.onCompleted: {
        Robot.setDataPollingEnabled(false)
        var helpMode = Config.get("mouths_help") || "always"
        var helpSeen = Session.getString("mouths_help_seen", "false") === "true"
        if (helpMode === "always") {
            helpDialog.open()
        } else if (!helpSeen) {
            Session.saveString("mouths_help_seen", "true")
            helpDialog.open()
        }
    }
    Component.onDestruction: Robot.setDataPollingEnabled(true)

    // ─── Corner buttons: Ranking + Help (top-right, like ZowiAppReborn) ─────
    // Same slot/pattern as the achievements button on other screens and as the
    // Zowi Dice game. Ranking is a disabled placeholder until the shared
    // top-10 layer lands; Help opens the how-to-play dialog.
    corner: Row {
        spacing: -10

        Button {
            id: rankingBtn
            width: 88
            height: 88
            enabled: false

            contentItem: Image {
                source: "qrc:/images/android/ranking_button.png"
                sourceSize.width: 56
                sourceSize.height: 56
                fillMode: Image.PreserveAspectFit
            }

            background: Rectangle {
                radius: 44
                color: rankingBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
            }
        }

        Button {
            id: helpBtn
            width: 88
            height: 88

            contentItem: Image {
                source: "qrc:/images/android/how_to_play_button.png"
                sourceSize.width: 56
                sourceSize.height: 56
                fillMode: Image.PreserveAspectFit
            }

            background: Rectangle {
                radius: 44
                color: helpBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
            }

            onClicked: helpDialog.open()
        }
    }

    // ─── Level + countdown bar (above the card, while a round runs) ─────────
    Column {
        id: topColumn
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: 4
        }
        width: Math.min(parent.width * 0.7, 460)
        spacing: 6
        visible: Mouths.state === Mouths.stateRoundActive
                 || Mouths.state === Mouths.stateRoundSolved

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.tr("level_prefix").arg(Mouths.level)
            color: Config.get("color_primary") || "#2d5a2d"
            font.pixelSize: 15
            font.bold: true
        }

        // Countdown timer bar (styled like the Zowi Dice progress bar).
        Rectangle {
            id: countdownContainer
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            height: 14
            radius: 7
            color: Config.get("color_bg_disabled") || "#e6e6e6"
            border.color: Config.get("color_accent") || "#21a69b"
            border.width: 1

            Rectangle {
                id: countdownBar
                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                }
                height: parent.height
                radius: 6
                color: Config.get("color_accent") || "#21a69b"
                width: parent.width * Mouths.countdownMs / Math.max(Mouths.roundTimeMs, 1)
                Behavior on width { NumberAnimation { duration: 100 } }
            }
        }
    }

    // ─── Card: target miniature + drawing grid ──────────────────────────────
    // Mirrors the Android layout (grid in a rounded "maker box"); the desktop
    // adds the always-visible target miniature next to the drawing grid.
    Rectangle {
        id: card
        anchors.centerIn: parent
        // Reserve ~34 px at the bottom of the content area for the score.
        anchors.verticalCenterOffset: -14
        radius: 24
        color: Config.get("color_bg_connected") || "#e8f5e8"
        border.color: Config.get("color_accent") || "#21a69b"
        border.width: 2
        width: cardRow.width + 28
        height: cardRow.height + 28

        Row {
            id: cardRow
            anchors.centerIn: parent
            spacing: 22

            // Target mouth, always visible while drawing (desktop deviation).
            Column {
                id: targetColumn
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.tr("target_label")
                    color: Config.get("color_primary") || "#2d5a2d"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Grid {
                    id: targetMini
                    anchors.horizontalCenter: parent.horizontalCenter
                    rows: 5
                    columns: 6
                    rowSpacing: 3
                    columnSpacing: 3

                    Repeater {
                        model: 30

                        delegate: Rectangle {
                            width: 11
                            height: 11
                            radius: width / 2
                            color: (Mouths.targetPattern & (1 << (29 - index))) !== 0
                                   ? (Config.get("color_accent") || "#21a69b")
                                   : "#ffffff"
                            border.color: Config.get("color_primary") || "#2d5a2d"
                            border.width: 1
                        }
                    }
                }
            }

            // The drawing grid: only touchable while a round is active. Every
            // change is compared live against the target; a match solves it.
            MouthGrid {
                id: drawGrid
                onPatternChanged: {
                    if (Mouths.state === Mouths.stateRoundActive)
                        Mouths.submitDraw(drawGrid.matrix)
                }
            }
        }
    }

    // ─── Score: strip between the card and the footer ───────────────────────
    Text {
        id: scoreText
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 6
        }
        text: root.tr("score_prefix").arg(Mouths.score)
        color: Config.get("color_primary") || "#2d5a2d"
        font.pixelSize: 16
        font.bold: true
    }

    // ─── Footer: Play button (Idle / GameOver) ──────────────────────────────
    footer: Item {
        anchors.fill: parent

        Button {
            id: playBtn
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: 12
            }
            visible: Mouths.state === Mouths.stateIdle
                     || Mouths.state === Mouths.stateGameOver
            implicitWidth: 200
            height: 50
            text: root.tr("play_button")

            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.bold: true
                font.pixelSize: 18
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 25
                color: playBtn.pressed ? Config.get("color_accent_pressed") || "#17736c" : (Config.get("color_accent") || "#21a69b")
            }

            onClicked: Mouths.startGame()
        }
    }

    // ─── Help Dialog ────────────────────────────────────────────────────────
    // Fully custom content (no default header/footer — their square white
    // rectangles would cover the rounded corners). Height is content-driven so
    // the Close button never sits on the border whatever the locale text
    // length; mirrors the Zowi Dice help dialog.
    Dialog {
        id: helpDialog
        modal: true
        width: 460
        anchors.centerIn: parent

        property real helpContentH: helpTitle.height + helpImg.height
                                    + helpText.implicitHeight + helpCloseBtn.height
                                    + 3 * helpCol.spacing
        height: Math.ceil(helpContentH) + 48 + Math.round(helpContentH * 0.05)

        background: Rectangle {
            radius: 20
            color: "#ffffff"
            border.color: Config.get("color_accent") || "#21a69b"
            border.width: 2
        }

        contentItem: Column {
            id: helpCol
            spacing: 18
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 24
            }

            Text {
                id: helpTitle
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.tr("help_button")
                font.pixelSize: 20
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Image {
                id: helpImg
                anchors.horizontalCenter: parent.horizontalCenter
                width: 130
                height: 130
                source: "qrc:/images/android/mouths_game_button.png"
                sourceSize.width: 130
                sourceSize.height: 130
                fillMode: Image.PreserveAspectFit
            }

            Text {
                id: helpText
                // Width from anchors (not a `width:` binding) so the wrapped
                // text cannot feed a width binding loop through the Column's
                // implicit sizing.
                anchors {
                    left: parent.left
                    right: parent.right
                }
                text: root.tr("how_to_play_text")
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 14
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Button {
                id: helpCloseBtn
                anchors.horizontalCenter: parent.horizontalCenter
                implicitWidth: 160
                implicitHeight: 44
                text: root.tr("close")

                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 22
                    color: helpCloseBtn.pressed ? Config.get("color_accent_pressed") || "#17736c" : (Config.get("color_accent") || "#21a69b")
                }

                onClicked: helpDialog.close()
            }
        }
    }

    // ─── Game Over Dialog ───────────────────────────────────────────────────
    Dialog {
        id: gameOverDialog
        modal: true
        width: 360
        anchors.centerIn: parent

        property real gameOverContentH: gameOverTitle.height + gameOverScore.height
                                        + gameOverBtns.height
                                        + 2 * gameOverCol.spacing
        height: Math.ceil(gameOverContentH) + 48 + Math.round(gameOverContentH * 0.05)

        background: Rectangle {
            radius: 20
            color: "#ffffff"
            border.color: Config.get("color_accent") || "#21a69b"
            border.width: 2
        }

        contentItem: Column {
            id: gameOverCol
            spacing: 18
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 24
            }

            Text {
                id: gameOverTitle
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.tr("game_over")
                font.pixelSize: 20
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Text {
                id: gameOverScore
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.tr("final_score").arg(Mouths.score)
                font.pixelSize: 24
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Row {
                id: gameOverBtns
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 14

                // Close: back to Idle with the Play button (resetGame).
                Button {
                    id: gameOverCloseBtn
                    implicitWidth: 130
                    implicitHeight: 44
                    text: root.tr("close")

                    contentItem: Text {
                        text: parent.text
                        color: Config.get("color_primary") || "#2d5a2d"
                        font.bold: true
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 22
                        color: gameOverCloseBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
                        border.color: Config.get("color_accent") || "#21a69b"
                        border.width: 2
                    }

                    onClicked: {
                        gameOverDialog.close()
                        Mouths.resetGame()
                    }
                }

                // Retry: start a new game immediately.
                Button {
                    id: gameOverRetryBtn
                    implicitWidth: 130
                    implicitHeight: 44
                    text: root.tr("retry_button")

                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font.bold: true
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 22
                        color: gameOverRetryBtn.pressed ? Config.get("color_accent_pressed") || "#17736c" : (Config.get("color_accent") || "#21a69b")
                    }

                    onClicked: {
                        gameOverDialog.close()
                        Mouths.startGame()
                    }
                }
            }
        }
    }

    // React to the game: clear the grid when a round starts or the game ends
    // (mirrors Android's resetMouthGrid on game over), and open the game-over
    // dialog when the countdown expires.
    Connections {
        target: Mouths
        function onStateChanged() {
            if (Mouths.state === Mouths.stateRoundActive)
                drawGrid.clearAll()
            else if (Mouths.state === Mouths.stateGameOver)
                drawGrid.clearAll()
        }
        function onGameOver(score) {
            gameOverDialog.open()
        }
    }
}