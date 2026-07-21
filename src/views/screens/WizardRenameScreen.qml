// WizardRenameScreen: shown right after a successful pairing.
// Lets the user give the Zowi a friendly name. The default name comes from
// config.json ("zowi_default_name"). Sends the rename command over Bluetooth
// ("R <name>\r") and waits for the firmware's final ack (&&F) before leaving.
import QtQuick 2.15
import QtQuick.Controls 2.15

ScreenTemplate {
    id: renameScreen
    screenName: "WizardRenameScreen"

    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: !renaming

    property string defaultName: Config.get("zowi_default_name") || "OpenZowi"
    property string robotName: Robot.deviceName || defaultName
    property bool renaming: false
    property bool renameDone: false
    // Set when reached over a USB-only connection; the rename is best-effort
    // there because the robot may not ACK it (no &&F).
    property bool usbMode: false
    // The robot performs a welcome gesture right after connecting; lock the
    // rename controls briefly so the user doesn't type/confirm into a busy link.
    property bool _lockExpired: false
    property bool locked: !_lockExpired || !dataReady
    property int lockMs: parseInt(Config.get("rename_lock_ms")) || 1500
    // Wait for the robot to report name, appId and battery before enabling
    // the rename controls (mirrors waitForRobotData in the CLI).
    property bool dataReady: Robot.connected
                             && Robot.deviceName !== ""
                             && Robot.appId !== ""
                             && Robot.battery >= 0

    signal renamed(string name)

    function tr(source) { return Translator.translate("WizardRenameScreen.qml", source) }

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
            text: renameScreen.robotName
            placeholderText: tr("name_placeholder")
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
            selectByMouse: true
            enabled: Robot.connected && !renameScreen.renaming && !renameScreen.locked
            onAccepted: renameButton.clicked()
        }

        Button {
            id: renameButton
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: 260
            height: 56
            text: renameScreen.renaming ? tr("renaming")
                  : (Robot.connected ? tr("rename") : tr("wait_pairing"))
            enabled: Robot.connected && !renameScreen.renaming && !renameScreen.locked && nameField.text.trim() !== ""

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
                color: renameButton.pressed ? "#17736c" : "#21a69b"
            }

            onClicked: {
                var name = nameField.text.trim()
                if (name === "" || !Robot.connected)
                    return
                // Skip rename if the robot already has the requested name.
                if (Robot.deviceName && Robot.deviceName.toLowerCase() === name.toLowerCase()) {
                    renameScreen.renamed(name)
                    return
                }
                renameScreen.renaming = true
                statusText.text = tr("sending")
                statusText.color = "#2d5a2d"
                statusText.visible = true
                renameTimer.restart()
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
            color: "#2d5a2d"
            visible: Robot.connected && !renameScreen.dataReady && !renameScreen.renaming
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Timer {
        id: renameTimer
        interval: 8000
        onTriggered: {
            if (renameScreen.renameDone) return
            renameScreen.renaming = false
            // Over USB the robot often does not ACK the rename (no &&F), so the
            // registration must not get stuck. Treat the timeout as best-effort:
            // keep the typed name and continue to Home instead of blocking.
            if (renameScreen.usbMode) {
                statusText.text = tr("rename_skipped_usb").arg(nameField.text.trim())
                statusText.color = "#2d5a2d"
                statusText.visible = true
                renameScreen.renamed(nameField.text.trim())
                return
            }
            statusText.text = tr("rename_failed")
            statusText.color = "#e74c3c"
            statusText.visible = true
        }
    }

    Component.onCompleted: {
        renameScreen._lockExpired = false
        lockTimer.restart()
    }

    onVisibleChanged: {
        if (renameScreen.visible && !renameScreen.locked && !renameScreen.renaming)
            nameField.forceActiveFocus()
    }

    // When robot data arrives after the welcome-gesture lock has expired,
    // the controls finally unlock — grab focus so the user can type.
    onLockedChanged: {
        if (!renameScreen.locked && !renameScreen.renaming) {
            nameField.selectAll()
            nameField.forceActiveFocus()
        }
    }

    // The rename field is disabled while locked (welcome gesture or waiting for
    // robot data). Once both conditions are satisfied, grab focus so the user
    // can type the name immediately.
    Timer {
        id: lockTimer
        interval: renameScreen.lockMs
        onTriggered: {
            renameScreen._lockExpired = true
            if (!renameScreen.locked) {
                nameField.selectAll()
                nameField.forceActiveFocus()
            }
        }
    }

    Connections {
        target: Robot

        function onDataReceived(data) {
            if (renameScreen.renaming)
                console.log("WizardRenameScreen: recv", JSON.stringify(data))
            if (renameScreen.renaming && !renameScreen.renameDone
                    && data.indexOf("&&F") !== -1) {
                renameScreen.renameDone = true
                renameTimer.stop()
                renameScreen.renaming = false
                statusText.text = tr("done").arg(nameField.text.trim())
                statusText.color = "#2d5a2d"
                statusText.visible = true
                Robot.setDeviceName(nameField.text.trim())
                renameScreen.renamed(nameField.text.trim())
            }
        }
    }
}
