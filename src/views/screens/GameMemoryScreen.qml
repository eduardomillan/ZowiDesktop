// GameMemoryScreen: "Memory" (Simon-like memory game, ex "Zowi Dice")
// Zowi plays a growing random sequence of 4 moves; player must repeat from memory.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "GameMemoryScreen"
    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: true
    footerHeight: 140
    property int cornerButtonSize: 88  // Configurable corner button size
    property real helpDialogWidthRatio: 0.5  // Help dialog width as % of window width

    function tr(source) { return Translator.translate("GameMemoryScreen.qml", source) }

    // Pause identity poll while game is running (like PadScreen does).
    // The game does NOT auto-start: the Play button in the footer starts it,
    // so the user can begin when they want or go back. The help dialog opens
    // on entry according to config "zowi_dice_help": "always" → every time,
    // "once" → only the first time (zowi_says_help_seen session flag).
    // No deferral is needed: the dialog is centered with anchors.centerIn,
    // so it stays centered regardless of when layout settles.
    Component.onCompleted: {
        Robot.setDataPollingEnabled(false)
        var helpMode = Config.get("zowi_dice_help") || "once"
        var helpSeen = Session.getString("zowi_says_help_seen", "false") === "true"
        if (helpMode === "always") {
            helpDialog.open()
        } else if (!helpSeen) {
            Session.saveString("zowi_says_help_seen", "true")
            helpDialog.open()
        }
    }
    Component.onDestruction: Robot.setDataPollingEnabled(true)

    // ─── Corner buttons: Help + Ranking (top-right, like ZowiAppReborn) ─────
    // Assigned to ScreenTemplate's corner slot (below the StatusBar, outside
    // the clipped contentArea) — the same spot the achievements button uses on
    // other screens. Mirrors activity_zowi_says_minigame_view.xml, where the
    // how-to-play and ranking icon buttons sit top-right.
    corner: Row {
        spacing: -10

        // Ranking (placeholder for the future top-10 list)
        Button {
            id: rankingBtn
            width: root.cornerButtonSize
            height: root.cornerButtonSize
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

        // Help ("Cómo jugar")
        Button {
            id: helpBtn
            width: root.cornerButtonSize
            height: root.cornerButtonSize

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

    // ─── Board: 2×2 action buttons inside a rounded "maker box" card ────────
    // Reserves ~56 px at the bottom of the content area for the score strip
    // (centered between the board and the footer).
    Rectangle {
        id: board
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height - 56) * 0.9
        height: width
        radius: 24
        color: Config.get("color_bg_connected") || "#e8f5e8"
        border.color: Config.get("color_accent") || "#21a69b"
        border.width: 2

        Item {
            id: actionGrid
            anchors.fill: parent
            anchors.margins: 18

            property real btnSize: (width - 24) / 2    // 24 px gap between buttons

            // Top-Left: Tiptoe Swing
            Image {
                id: btnTiptoeSwing
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
                x: parent.btnSize + 24
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
                y: parent.btnSize + 24
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
                x: parent.btnSize + 24
                y: parent.btnSize + 24
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
    }

    // ─── Score: centered in the strip between the board and the footer ──────
    // Lives in the content area (not the footer), bottom-centered above the
    // footer. The board reserves a ~56 px band at the bottom so they never
    // overlap, whatever the window size.
    Text {
        id: scoreText
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 6
        }
        text: tr("score_prefix").arg(ZowiDice.score)
        color: Config.get("color_primary") || "#2d5a2d"
        font.pixelSize: 16
        font.bold: true
    }

    // ─── Footer: Progress, Play (in ScreenTemplate's footer area) ───────────
    // Lives in the real footer (below the board area) so nothing overlaps the
    // action grid. The score sits between the board and this footer; the
    // progress bar shows while playing (hidden otherwise), and the Play
    // button sits at the bottom of the window. Hidden children take no space,
    // so during a round only the progress bar is visible.
    footer: ColumnLayout {
        id: footerColumn
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        spacing: 10

        // Progress bar: shown while Zowi replays AND while the user repeats.
        // The "X / Y" readout tracks the current step (1-based while replaying,
        // moves repeated so far during the user's turn).
        Rectangle {
            id: progressContainer
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: parent.width * 0.7
            Layout.preferredHeight: 14
            visible: ZowiDice.state === ZowiDice.stateShowingSequence
                     || ZowiDice.state === ZowiDice.stateWaitingForUser
            radius: 7
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
                radius: 6
                color: Config.get("color_accent") || "#21a69b"
                width: progressContainer.width * ZowiDice.progress / 100
                Behavior on width { NumberAnimation { duration: 100 } }
            }

            Text {
                id: progressText
                anchors.centerIn: parent
                text: {
                    var total = ZowiDice.sequenceLength
                    var step = ZowiDice.currentStep
                    if (ZowiDice.state === ZowiDice.stateShowingSequence)
                        step = step + 1
                    return "%1 / %2".arg(step).arg(total)
                }
                color: "#ffffff"
                font.pixelSize: 11
                font.bold: true
            }
        }

        // Play button (bottom of the window), styled like the splash
        // "Continuar" button: same size and font.
        Button {
            id: playBtn
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 12
            visible: ZowiDice.state === ZowiDice.stateIdle
                     || ZowiDice.state === ZowiDice.stateGameOver
            implicitWidth: 200
            Layout.preferredHeight: 50
            text: tr("play_button")

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

            onClicked: ZowiDice.startGame()
        }
    }

    // ─── "Look at Zowi" overlay while the robot replays the sequence ────────
    Item {
        id: lookAtZowiOverlay
        anchors.fill: parent
        visible: ZowiDice.blockUserInput

        Rectangle {
            anchors.fill: parent
            color: "#000000"
            opacity: 0.55
        }

        Column {
            anchors.centerIn: parent
            width: Math.min(root.width * 0.7, 420)
            spacing: 20

            AnimatedZowi {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 170
                height: 170
            }

            Text {
                width: parent.width
                text: tr("look_at_zowi_text")
                color: "#ffffff"
                font.pixelSize: 22
                font.bold: true
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // ─── Help Dialog ────────────────────────────────────────────────────────
    // Fully custom content (no default header/footer — those draw square white
    // rectangles over the corners, hiding the radius). Centered on its parent
    // (the content area) with anchors.centerIn, which QQC2 supports for the
    // immediate parent; without it the popup lands at the parent's top-left.
    // The height is driven by the actual content (wrapped text included) plus
    // margins plus a ~5% breathing room below the Close button, so the button
    // never sits on the border whatever the locale text length.
    Dialog {
        id: helpDialog
        modal: true
        width: Math.round(root.width * root.helpDialogWidthRatio)
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
                text: tr("help_button")
                font.pixelSize: 20
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Image {
                id: helpImg
                anchors.horizontalCenter: parent.horizontalCenter
                width: 130
                height: 130
                source: "qrc:/images/android/simon_game_button.png"
                sourceSize.width: 130
                sourceSize.height: 130
                fillMode: Image.PreserveAspectFit
            }

            Text {
                id: helpText
                // Width from anchors (not a `width:` binding) so the wrapped
                // text cannot feed a width binding loop through the Column's
                // implicit sizing. Text is centered via horizontalAlignment.
                anchors {
                    left: parent.left
                    right: parent.right
                }
                text: tr("how_to_play_text")
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
                text: tr("close")

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
    // Same treatment as the help dialog: fully custom content with no default
    // header/footer (whose square white rectangles covered the rounded
    // corners), so the radius shows. Height is content-driven like the help
    // dialog, keeping the rounded corners and the button clearance in sync.
    Dialog {
        id: gameOverDialog
        modal: true
        width: 360
        anchors.centerIn: parent

        property real gameOverContentH: gameOverTitle.height + gameOverScore.height
                                        + gameOverBest.height + gameOverBtns.height
                                        + 3 * gameOverCol.spacing
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
                text: tr("game_over")
                font.pixelSize: 20
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Text {
                id: gameOverScore
                anchors.horizontalCenter: parent.horizontalCenter
                text: tr("final_score").arg(ZowiDice.score)
                font.pixelSize: 24
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Text {
                id: gameOverBest
                anchors.horizontalCenter: parent.horizontalCenter
                text: (ZowiDice.score >= 12) ? tr("new_best") : ""
                font.pixelSize: 14
                color: Config.get("color_accent") || "#21a69b"
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
                    text: tr("close")

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
                        ZowiDice.resetGame()
                    }
                }

                // Retry: start a new game immediately.
                Button {
                    id: gameOverRetryBtn
                    implicitWidth: 130
                    implicitHeight: 44
                    text: tr("retry_button")

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
                        ZowiDice.startGame()
                    }
                }
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