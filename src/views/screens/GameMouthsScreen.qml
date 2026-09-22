// GameMouthsScreen: "Pintabocas" (Draw the mouths).
// Zowi shows a random mouth and the player must draw it on the 6×5 LED grid
// before the countdown expires. Live compare: as soon as the drawn pattern
// matches the target the round is solved (mirrors Android's
// MouthGridLayoutTouchListener → checkLedMouth). Fully playable offline — the
// mouth and gesture commands are cosmetics that are simply not sent when the
// robot is not connected. Desktop deviation from Android: the target mouth is
// mirrored as an on-screen miniature — hidden while the robot is connected
// (its own matrix shows it) unless config "mouths_target_onscreen" forces it.
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

    // Target-miniature policy, config "mouths_target_onscreen":
    //   "auto"   (default) → shown only while the robot is NOT connected
    //   "always"            → always shown
    //   "never"             → never shown
    readonly property bool showTargetOnScreen: {
        var mode = Config.get("mouths_target_onscreen") || "auto"
        if (mode === "always")
            return true
        if (mode === "never")
            return false
        return !Robot.connected
    }

    // Width actually occupied by the target miniature (0 when hidden). One
    // source of truth used by the grid size, the card width and the score
    // offset, so the card exactly wraps the mini+grid row and the score stays
    // perfectly centered under the drawing grid in both states.
    readonly property real miniW: root.showTargetOnScreen ? targetColumn.width : 0

    // Resizable grid, like the Memory board: the cell size derives from the
    // CONTENT area (not from the card or the grid itself, which would make a
    // circular binding). Both dimensions are considered — the height reserves
    // the level/countdown band on top and the score strip at the bottom, the
    // width the target miniature (when shown), the mini↔grid spacing and the
    // card padding — and the grid uses the smaller of the two, so it always
    // fits the window and never covers the progress bar.
    readonly property real drawCellSize: {
        var cFromWidth = (card.contentW - 128 - root.miniW) / 6   // grid 6 cells + 5×6 spacing
        var cFromHeight = (card.contentH - 156) / 5               // −(top band 48, score 56, padding 52)
        return Math.round(Math.max(26, Math.min(50, Math.min(cFromWidth, cFromHeight))))
    }
    // Explicit card size so the box is derived from the container first
    // (Memory pattern); the grid then fills it via `drawCellSize`.
    readonly property real cardW: root.miniW + 22 + 6 * root.drawCellSize + 5 * 6 + 28
    readonly property real cardH: 5 * root.drawCellSize + 4 * 6 + 28

    function tr(source) { return Translator.translate("GameMouthsScreen.qml", source) }

    // Pause the identity poll while the game is open (the mouth/gesture
    // commands go out during play; the E/I/B burst would drain the robot's
    // queue). Help dialog auto-opens on entry per config "mouths_help":
    // "always" → every time, "once" → only the first (mouths_help_seen flag).
    // It never starts the game — the Play button does.
    Component.onCompleted: {
        Robot.setDataPollingEnabled(false)
        // TEMP-VERIFY ground truth (removed once verified)
        console.log("[Mouths] verify mode=" + (Config.get("mouths_target_onscreen") || "auto")
                    + " connected=" + Robot.connected
                    + " showTarget=" + root.showTargetOnScreen
                    + " cell=" + root.drawCellSize)
        var helpMode = Config.get("mouths_help") || "always"
        var helpSeen = Session.getString("mouths_help_seen", "false") === "true"
        if (helpMode === "always") {
            helpDialog.open()
        } else if (!helpSeen) {
            Session.saveString("mouths_help_seen", "true")
            helpDialog.open()
        }
        // DEV preview hook: --step 1 starts a round so the in-game layout
        // (level, countdown bar, target miniature) can be iterated headlessly,
        // mirroring the calibration screen's PreviewStep hook.
        if (typeof PreviewStep !== "undefined" && PreviewStep >= 0)
            Mouths.startGame()
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
        width: Math.min(parent.width * 0.7, root.cardW)
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
    // Mirrors the Android layout (grid in a rounded "maker box") and follows
    // the Memory-board pattern: the card box is derived from the content area
    // first (root.cardW/cardH via the responsive cell size), keeping clear of
    // the level/countdown band on top and the score strip at the bottom. The
    // desktop adds the configurable target miniature next to the drawing grid.
    Rectangle {
        id: card
        anchors.centerIn: parent
        radius: 24
        color: Config.get("color_bg_connected") || "#e8f5e8"
        border.color: Config.get("color_accent") || "#21a69b"
        border.width: 2
        width: root.cardW
        height: root.cardH

        // Geometry of the content area this card lives in (parent IS the
        // ScreenTemplate content area). Drive the responsive grid size.
        readonly property real contentW: parent ? parent.width : 728
        readonly property real contentH: parent ? parent.height : 399

        Row {
            id: cardRow
            anchors.centerIn: parent
            spacing: 22

            // Target mouth miniature: mirrors the mouth shown on the robot
            // (hidden while the robot is connected — its own matrix shows it —
            // unless config "mouths_target_onscreen" forces "always").
            Column {
                id: targetColumn
                visible: root.showTargetOnScreen
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
                // Resizable like the Memory board: cell size = min(width and
                // height constraints) of the content area, recomputed live on
                // window resize / connection changes (target miniature hides).
                cellSize: root.drawCellSize
                onPatternChanged: {
                    if (Mouths.state === Mouths.stateRoundActive)
                        Mouths.submitDraw(drawGrid.matrix)
                }
            }
        }
    }

    // ─── Score: strip between the card and the footer ───────────────────────
    // Anchored to the card (a sibling, so the anchor is valid) with an offset
    // equal to half the miniature+spacing when it is shown: the score stays
    // centered under the drawing grid in both states, whatever the window
    // size. Bottom strip, like the Memory screen.
    Text {
        id: scoreText
        anchors {
            horizontalCenter: card.horizontalCenter
            horizontalCenterOffset: root.showTargetOnScreen ? (root.miniW + 22) / 2 : 0
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