// CalibrationScreen: interactive servo-trim calibration for Zowi.
// Mirrors ZowiAppReborn's CalibrationViewActivity (a single 4-step pager):
//   WARNING → LEGS → FEET → CHECK
// Trims are clamped to ±60° and sent live via the firmware G command
// (volatile) while adjusting; persisted to EEPROM with the C command on
// confirm. A VICTORY gesture (H 12) plays after saving.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "CalibrationScreen"
    title: tr("calibration")
    showBackButton: true

    // Constants matching the robot's servo model (BASE_GRADE = 90).
    readonly property int baseGrade: 90
    readonly property int minTrim: -60
    readonly property int maxTrim: 60
    // The firmware moves each servo for ~200 ms; don't flood it with faster
    // consecutive G commands while the user adjusts.
    readonly property int debounceMs: 200

    // Trim offsets, indexed like the servo order YL YR RL RR.
    property int trimYL: 0
    property int trimYR: 0
    property int trimRL: 0
    property int trimRR: 0
    property int step: 0          // 0=Warning 1=Legs 2=Feet 3=Check
    // Preview hook: lets zowi_screen_preview open the screen on a specific
    // step (0..3). Falls back to step 0 (Warning) when not previewing.
    property int startStep: (typeof PreviewStep !== "undefined") ? PreviewStep : 0

    property real lastSendTime: 0
    property bool pendingSend: false

    function tr(source) { return Translator.translate("CalibrationScreen.qml", source) }

    // "C yl yr rl rr\r" — persists the trims to EEPROM.
    function setTrimsCommand() {
        return "C " + trimYL + " " + trimYR + " " + trimRL + " " + trimRR + "\r"
    }
    // "G <yl+90> <yr+90> <rl+90> <rr+90>\r" — moves the servos live (volatile).
    function servosCommand() {
        return "G " + (baseGrade + trimYL) + " " + (baseGrade + trimYR) + " "
                 + (baseGrade + trimRL) + " " + (baseGrade + trimRR) + "\r"
    }

    function send(cmd) { if (Robot.connected) Robot.sendData(cmd) }

    function cancelPendingServos() {
        pendingSend = false
        sendTimer.stop()
    }

    function resetToNeutral() {
        cancelPendingServos()
        send("C 0 0 0 0\r")
        trimYL = 0; trimYR = 0; trimRL = 0; trimRR = 0
        send("G " + baseGrade + " " + baseGrade + " " + baseGrade + " " + baseGrade + "\r")
    }

    function playVictory() { send("H 12\r") }

    // Live-update the move via G after a trim change. Only one G is kept in
    // flight at a time: rapid trims coalesce into a single send of the most
    // recent value (intermediate ones are dropped) once the firmware's ~200 ms
    // _moveServos completes, instead of queuing several commands out of phase.
    function sendServos() {
        var now = Date.now()
        if (now - lastSendTime >= debounceMs) {
            flushServos()
        } else if (!pendingSend) {
            // A G is still in flight. Coalesce rapid trims into the most recent
            // value and flush it no later than debounceMs after the first
            // change (like the CLI's needSend loop) — do NOT restart the timer
            // on every press, or a held button would keep postponing the send.
            pendingSend = true
            sendTimer.start()
        }
    }

    // Emit the current (latest) G immediately and restart the debounce window.
    function flushServos() {
        pendingSend = false
        sendTimer.stop()
        send(servosCommand())
        lastSendTime = Date.now()
    }

    Timer {
        id: sendTimer
        interval: root.debounceMs
        repeat: false
        onTriggered: {
            if (root.pendingSend) {
                root.flushServos()
            }
        }
    }

    function clamp(v) { return Math.max(minTrim, Math.min(maxTrim, v)) }

    function adjust(which, delta) {
        var val = 0
        if (which === "yl") { val = trimYL + delta; trimYL = clamp(val) }
        else if (which === "yr") { val = trimYR + delta; trimYR = clamp(val) }
        else if (which === "rl") { val = trimRL + delta; trimRL = clamp(val) }
        else if (which === "rr") { val = trimRR + delta; trimRR = clamp(val) }
        sendServos()
    }

    function showStep(s) { step = s }

    function testMovement() {
        cancelPendingServos()
        send(setTrimsCommand())
        playVictory()
    }

    function confirmSave() {
        cancelPendingServos()
        send(setTrimsCommand())
        playVictory()
        root.backClicked()
    }

    Component.onCompleted: {
        step = startStep
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
        currentIndex: root.step

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
                        value: root.trimYL
                        onPlus: root.adjust("yl", 1)
                        onMinus: root.adjust("yl", -1)
                        onPlusCoarse: root.adjust("yl", 10)
                        onMinusCoarse: root.adjust("yl", -10)
                    }

                    CalibrationTrimColumn {
                        label: root.tr("right_leg")
                        shortLabel: "YR"
                        value: root.trimYR
                        onPlus: root.adjust("yr", 1)
                        onMinus: root.adjust("yr", -1)
                        onPlusCoarse: root.adjust("yr", 10)
                        onMinusCoarse: root.adjust("yr", -10)
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
                        value: root.trimRL
                        onPlus: root.adjust("rl", 1)
                        onMinus: root.adjust("rl", -1)
                        onPlusCoarse: root.adjust("rl", 10)
                        onMinusCoarse: root.adjust("rl", -10)
                    }

                    CalibrationTrimColumn {
                        label: root.tr("right_foot")
                        shortLabel: "RR"
                        value: root.trimRR
                        onPlus: root.adjust("rr", 1)
                        onMinus: root.adjust("rr", -1)
                        onPlusCoarse: root.adjust("rr", 10)
                        onMinusCoarse: root.adjust("rr", -10)
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