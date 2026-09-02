// CalibrationScreen: interactive servo-trim calibration for Zowi.
// Mirrors ZowiAppReborn's CalibrationViewActivity (a single 4-step pager):
//   WARNING → LEGS → FEET → CHECK
// All calibration state lives in the shared zowi::CalibrationSession (exposed
// to QML as `Calibration`): the trims, clamps (±60°), the generated G/C
// commands and the "one G in flight" debounce policy. This screen only renders
// the pager and forwards the user's adjustments; A VICTORY gesture (H 12) plays
// after saving.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "CalibrationScreen"
    title: tr("calibration")
    showBackButton: true

    // Servo indices for CalibrationSession (YL YR RL RR).
    readonly property int yl: 0
    readonly property int yr: 1
    readonly property int rl: 2
    readonly property int rr: 3

    function tr(source) { return Translator.translate("CalibrationScreen.qml", source) }
    function send(cmd) { if (Robot.connected) Robot.sendData(cmd) }
    function playVictory() { send("H 12\r") }

    // Live-update the move via G after a trim change. Core's shouldSend keeps at
    // most one G in flight; if a change falls inside the debounce window it is
    // coalesced (the most recent value stays in the session), and this timer
    // re-emits it as soon as the window elapses — mirroring the CLI's needSend
    // loop so a quick press is never lost.
    Timer {
        id: sendTimer
        interval: 200
        repeat: false
        onTriggered: root.flushServos()
    }

    function adjust(index, delta) {
        if (Calibration.adjust(index, delta)) {
            flushServos()
        }
    }

    // Attempt a live G now; if core coalesces it, schedule a retry so the latest
    // trim is always flushed within the debounce window.
    function flushServos() {
        sendTimer.stop()
        if (!Calibration.sendServos()) {
            sendTimer.start()
        }
    }

    function cancelPendingServos() { sendTimer.stop() }

    function resetToNeutral() {
        cancelPendingServos()
        // "C 0 0 0 0\r" persists a zeroed trim; then return all servos to 90°.
        send("C 0 0 0 0\r")
        send(Calibration.resetToNeutral())
    }

    function testMovement() {
        cancelPendingServos()
        send(Calibration.trimsCommand())
        playVictory()
    }

    function confirmSave() {
        cancelPendingServos()
        send(Calibration.trimsCommand())
        playVictory()
        root.backClicked()
    }

    function showStep(s) { Calibration.jumpToStep(s) }

    Component.onCompleted: {
        // Keep the BLE channel clean during calibration: pause the periodic
        // name/appId/battery poll so only live G commands are in flight,
        // exactly like zowi_cli calibrate. Resume it when the screen goes away.
        Robot.setDataPollingEnabled(false)
        root.forceActiveFocus()
    }

    Component.onDestruction: Robot.setDataPollingEnabled(true)

    // ── Content: a 4-step StackLayout mirroring the Android pager ──
    StackLayout {
        id: steps
        anchors.fill: parent
        currentIndex: Calibration.step

        // ── Step 0: WARNING ──────────────────────────────────────
        Item {
            Column {
                anchors.centerIn: parent
                width: Math.min(parent.width - 60, 560)
                spacing: 24

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: root.tr("warning")
                    color: Config.get("color_primary") || "#2d5a2d"
                    font.pixelSize: 15
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 18

                    Button {
                        text: root.tr("cancel")
                        background: Rectangle {
                            color: Config.get("color_danger") || "#e74c3c"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#ffffff"
                            font.pixelSize: 13
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: root.backClicked()
                    }

                    Button {
                        text: root.tr("continue")
                        background: Rectangle {
                            color: Config.get("color_accent") || "#21a69b"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#ffffff"
                            font.pixelSize: 13
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            root.resetToNeutral()
                            root.showStep(1)
                        }
                    }
                }
            }
        }

        // ── Step 1: LEGS (YL / YR) ──────────────────────────────
        Item {
            Column {
                anchors.centerIn: parent
                width: Math.min(parent.width - 60, 620)
                spacing: 18

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: "qrc:/images/android/calibrate_legs_image.png"
                    sourceSize.height: 120
                    sourceSize.width: 200
                    fillMode: Image.PreserveAspectFit
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 60

                    CalibrationTrimColumn {
                        label: root.tr("left_leg")
                        shortLabel: "YL"
                        value: Calibration.trimYL
                        onPlus: root.adjust(root.yl, 1)
                        onMinus: root.adjust(root.yl, -1)
                        onPlusCoarse: root.adjust(root.yl, 10)
                        onMinusCoarse: root.adjust(root.yl, -10)
                    }

                    CalibrationTrimColumn {
                        label: root.tr("right_leg")
                        shortLabel: "YR"
                        value: Calibration.trimYR
                        onPlus: root.adjust(root.yr, 1)
                        onMinus: root.adjust(root.yr, -1)
                        onPlusCoarse: root.adjust(root.yr, 10)
                        onMinusCoarse: root.adjust(root.yr, -10)
                    }
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.tr("next_step")
                    background: Rectangle {
                        color: Config.get("color_accent") || "#21a69b"
                        radius: 6
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.showStep(2)
                }
            }
        }

        // ── Step 2: FEET (RL / RR) ──────────────────────────────
        Item {
            Column {
                anchors.centerIn: parent
                width: Math.min(parent.width - 60, 620)
                spacing: 18

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: "qrc:/images/android/calibrate_feet_image.png"
                    sourceSize.height: 120
                    sourceSize.width: 200
                    fillMode: Image.PreserveAspectFit
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 60

                    CalibrationTrimColumn {
                        label: root.tr("left_foot")
                        shortLabel: "RL"
                        value: Calibration.trimRL
                        onPlus: root.adjust(root.rl, 1)
                        onMinus: root.adjust(root.rl, -1)
                        onPlusCoarse: root.adjust(root.rl, 10)
                        onMinusCoarse: root.adjust(root.rl, -10)
                    }

                    CalibrationTrimColumn {
                        label: root.tr("right_foot")
                        shortLabel: "RR"
                        value: Calibration.trimRR
                        onPlus: root.adjust(root.rr, 1)
                        onMinus: root.adjust(root.rr, -1)
                        onPlusCoarse: root.adjust(root.rr, 10)
                        onMinusCoarse: root.adjust(root.rr, -10)
                    }
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.tr("finish")
                    background: Rectangle {
                        color: Config.get("color_accent") || "#21a69b"
                        radius: 6
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.showStep(3)
                }
            }
        }

        // ── Step 3: CHECK ───────────────────────────────────────
        Item {
            Column {
                anchors.centerIn: parent
                width: Math.min(parent.width - 60, 560)
                spacing: 20

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: "qrc:/images/android/calibrate_standing_image.png"
                    sourceSize.height: 160
                    sourceSize.width: 180
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.tr("check_desc")
                    color: Config.get("color_primary") || "#2d5a2d"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 14

                    Button {
                        text: root.tr("test_movement")
                        background: Rectangle {
                            color: Config.get("color_accent") || "#21a69b"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#ffffff"
                            font.pixelSize: 13
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: root.testMovement()
                    }

                    Button {
                        text: root.tr("restart")
                        background: Rectangle {
                            color: Config.get("color_warning") || "#e67e22"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#ffffff"
                            font.pixelSize: 13
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            root.resetToNeutral()
                            root.showStep(1)
                        }
                    }

                    Button {
                        text: root.tr("confirm")
                        background: Rectangle {
                            color: Config.get("color_primary") || "#2d5a2d"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#ffffff"
                            font.pixelSize: 13
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: root.confirmSave()
                    }
                }
            }
        }
    }
}