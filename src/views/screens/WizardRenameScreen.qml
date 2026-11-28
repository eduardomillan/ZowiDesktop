// WizardRenameScreen: shown right after a successful pairing.
// Lets the user give the Zowi a friendly name. The default name comes from
// config.json ("zowi_default_name"). Sends the rename command over Bluetooth
// ("R <name>\r") and waits for the firmware's final ack (&&F) before leaving.
import QtQuick 2.15
import QtQuick.Controls 2.15

ScreenTemplate {
    id: wizardRename
    screenName: "WizardRenameScreen"

    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: !renaming

    property string defaultName: Config.get("zowi_default_name") || "OpenZowi"
    property string robotName: Robot.deviceName || defaultName
    property bool renaming: false
    property bool renameDone: false
    // The command is re-sent once if nothing confirms it in time (see retryTimer).
    property bool _retried: false
    // Set when reached over a USB-only connection; the rename is best-effort
    // there because the robot may not ACK it (no &&F).
    property bool usbMode: false
    // The rename controls stay locked until the robot has answered the identity
    // requests (name, appId, battery): it only answers once its welcome gesture
    // is over, so this is the readiness gate (same as the CLI's
    // waitForRobotData). No extra fixed delay: this screen is only opened after
    // the robot has already reported its name.
    property bool locked: !dataReady
    property bool dataReady: Robot.connected
                             && Robot.deviceName !== ""
                             && Robot.appId !== ""
                             && Robot.battery >= 0

    signal renamed(string name)

    function tr(source) { return Translator.translate("WizardRenameScreen.qml", source) }

    // The firmware rename command is space-delimited and the name ends up in
    // EEPROM: letters only (ASCII plus Latin-1 accented letters, excluding the
    // × and ÷ signs). Mirrors zowi::isValidRobotName() in the core, which is
    // the source of truth; this mirror only keeps the field clean while
    // typing (a RegExpValidator would need different type names on Qt 5/6).
    // The firmware keeps the name in 11 bytes (10 usable); accented letters take
    // two bytes in UTF-8 (zowi::kMaxRobotNameBytes in the core).
    readonly property int maxNameBytes: 10
    function nameBytes(name) {
        var n = 0
        for (var i = 0; i < name.length; ++i)
            n += name.charCodeAt(i) > 0x7F ? 2 : 1
        return n
    }
    function isValidName(name) {
        return /^[A-Za-z\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u00FF]+$/.test(name)
               && nameBytes(name) <= maxNameBytes
    }

    Column {
        anchors.centerIn: parent
        spacing: 25

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: Config.get("zowi_found_image")
            sourceSize.height: 160
            fillMode: Image.PreserveAspectFit
        }

        TextField {
            id: nameField
            anchors.horizontalCenter: parent.horizontalCenter
            width: 340
            height: 48
            text: wizardRename.robotName
            placeholderText: tr("name_placeholder")
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
            selectByMouse: true
            enabled: Robot.connected && !wizardRename.renaming && !wizardRename.locked
            // Strip anything that is not a letter while typing/pasting.
            onTextChanged: {
                var cleaned = text.replace(/[^A-Za-z\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u00FF]/g, "")
                // ...and stop at the firmware's 10-byte limit.
                while (cleaned.length > 0 && wizardRename.nameBytes(cleaned) > wizardRename.maxNameBytes)
                    cleaned = cleaned.slice(0, -1)
                if (cleaned !== text) {
                    var pos = cursorPosition
                    text = cleaned
                    cursorPosition = Math.min(pos, cleaned.length)
                }
            }
            onAccepted: renameButton.clicked()
        }

        Button {
            id: renameButton
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: 260
            height: 56
            text: wizardRename.renaming ? tr("renaming")
                  : (Robot.connected ? tr("rename") : tr("wait_pairing"))
            enabled: Robot.connected && !wizardRename.renaming && !wizardRename.locked
                     && wizardRename.isValidName(nameField.text.trim())

            contentItem: Text {
                text: parent.text
                font.pixelSize: 16
                font.bold: true
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 28
                color: renameButton.pressed ? Config.get("color_accent_pressed") || "#17736c" : Config.get("color_accent") || "#21a69b"
            }

            onClicked: {
                var name = nameField.text.trim()
                // Belt-and-braces: the field validator already blocks invalid
                // characters, but programmatic text (the current robot name)
                // bypasses it, so re-check here.
                if (name === "" || !wizardRename.isValidName(name) || !Robot.connected)
                    return
                // Skip rename if the robot already has the requested name.
                if (Robot.deviceName && Robot.deviceName.toLowerCase() === name.toLowerCase()) {
                    wizardRename.renamed(name)
                    return
                }
                wizardRename.renaming = true
                wizardRename._retried = false
                statusText.text = tr("sending")
                statusText.color = Config.get("color_primary") || "#2d5a2d"
                statusText.visible = true
                renameTimer.restart()
                retryTimer.restart()
                // Firmware command: "R <name>\r"
                Robot.sendData("R " + name + "\r")
            }
        }

        Text {
            id: statusText
            anchors.horizontalCenter: parent.horizontalCenter
            text: ""
            font.pixelSize: 14
            visible: false
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: 340
        }

        Text {
            id: waitText
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("waiting_data")
            font.pixelSize: 14
            color: Config.get("color_primary") || "#2d5a2d"
            visible: Robot.connected && !wizardRename.dataReady && !wizardRename.renaming
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // A command sent while the robot is busy (e.g. a welcome gesture or a stalled
    // link) can be swallowed. If neither the ack nor the new name has shown up
    // after 2.5 s, send it once more: renaming twice to the same name is
    // harmless, and the user no longer has to press the button a second time.
    Timer {
        id: retryTimer
        interval: 2500
        onTriggered: {
            if (!wizardRename.renaming || wizardRename.renameDone || wizardRename._retried)
                return
            wizardRename._retried = true
            var name = nameField.text.trim()
            console.log("WizardRenameScreen: no confirmation yet, re-sending the rename")
            Robot.sendData("R " + name + "\r")
        }
    }

    Timer {
        id: renameTimer
        // Bluetooth: the ack arrives within ~100 ms, so 5 s leaves room for a few
        // identity polls (the lost-ack fallback). USB often sends no ack: 8 s.
        interval: wizardRename.usbMode ? 8000 : 5000
        onTriggered: {
            if (wizardRename.renameDone) return
            retryTimer.stop()
            wizardRename.renaming = false
            // Over USB the robot often does not ACK the rename (no &&F), so the
            // registration must not get stuck. Treat the timeout as best-effort:
            // keep the typed name and continue to Home instead of blocking.
            if (wizardRename.usbMode) {
                statusText.text = tr("rename_skipped_usb").arg(nameField.text.trim())
                statusText.color = Config.get("color_primary") || "#2d5a2d"
                statusText.visible = true
                wizardRename.renamed(nameField.text.trim())
                return
            }
            statusText.text = tr("rename_failed")
            statusText.color = Config.get("color_danger") || "#e74c3c"
            statusText.visible = true
        }
    }

    // Completes the rename once the robot confirmed it, whichever way we found
    // out (see the Connections below). Idempotent.
    function completeRename() {
        if (!wizardRename.renaming || wizardRename.renameDone) return
        var name = nameField.text.trim()
        wizardRename.renameDone = true
        renameTimer.stop()
        retryTimer.stop()
        wizardRename.renaming = false
        statusText.text = tr("done").arg(name)
        statusText.color = Config.get("color_primary") || "#2d5a2d"
        statusText.visible = true
        Robot.setDeviceName(name)
        wizardRename.renamed(name)
    }

    onVisibleChanged: {
        if (wizardRename.visible && !wizardRename.locked && !wizardRename.renaming)
            nameField.forceActiveFocus()
    }

    // When robot data arrives after the welcome-gesture lock has expired,
    // the controls finally unlock — grab focus so the user can type.
    onLockedChanged: {
        if (!wizardRename.locked && !wizardRename.renaming) {
            nameField.selectAll()
            nameField.forceActiveFocus()
        }
    }

    // Focus the field as soon as the screen is ready (also when it already is
    // at creation: onLockedChanged would not fire then).
    Component.onCompleted: {
        if (!wizardRename.locked) {
            nameField.selectAll()
            nameField.forceActiveFocus()
        }
    }

    Connections {
        target: Robot

        function onDataReceived(data) {
            if (wizardRename.renaming)
                console.log("WizardRenameScreen: recv", JSON.stringify(data))
        }

        // The final ack (&&F). Use the controller's parsed signal, NOT a search
        // for "&&F" in the raw chunks: Bluetooth may split the frame ("&" then
        // "&F%%"), which made a successful rename look like a failure.
        function onFinalAckReceived() {
            wizardRename.completeRename()
        }

        // Safety net for a lost ack (also over USB, where the robot often does
        // not send it): the periodic identity poll reports the robot's name, and
        // when it is the requested one the rename has taken effect. (The click
        // handler skips the rename when the robot already has that name, so an
        // equal name while renaming can only come from our command.)
        function onDeviceChanged() {
            if (wizardRename.renaming && !wizardRename.renameDone
                    && Robot.deviceName
                    && Robot.deviceName.toLowerCase() === nameField.text.trim().toLowerCase())
                wizardRename.completeRename()
        }
    }
}
