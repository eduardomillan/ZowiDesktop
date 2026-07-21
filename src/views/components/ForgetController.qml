// ForgetController: shared, non-visual helper that "forgets" the registered
// Zowi. As part of forgetting it attempts a factory reset of the robot's name
// (back to zowi_default_name from config.json) by connecting if needed, sending
// the "R <name>\r" command, and waiting for the firmware ACK (&&F). The rename
// is best-effort: if the robot can't be reached or does not ACK, forgetting
// still completes (the device is unpaired and the session is cleared).
import QtQuick 2.15

Item {
    id: forgetter

    signal forgetFinished(bool unpaired, string message)
    signal statusMessage(string text)

    property string defaultName: Config.get("zowi_default_name") || "Zowi"

    // Internal state.
    property string _address: ""
    property bool _active: false
    property bool _waitingAck: false
    property bool _initiatedConnect: false

    function tr(source) { return Translator.translate("ForgetController.qml", source) }

    function forget(address) {
        if (_active) return
        _active = true
        _address = address || ""
        if (_address === "") {
            // Nothing registered: just drop any stale app data.
            finalize(true, "")
            return
        }
        statusMessage(tr("renaming").arg(defaultName))
        if (Robot.connected) {
            // Skip rename if the robot already has the target name.
            if (Robot.deviceName && Robot.deviceName.toLowerCase() === defaultName.toLowerCase()) {
                doUnpair()
                return
            }
            sendRename()
        } else {
            // Try to connect (transport-aware) so we can issue the rename.
            _initiatedConnect = true
            connectTimer.restart()
            var transport = Session.loadActiveZowiTransport()
            if (transport === "usb")
                Robot.connectUsb()
            else
                Robot.connectToDevice(_address)
        }
    }

    function sendRename() {
        _waitingAck = true
        ackTimer.restart()
        Robot.sendData("R " + defaultName + "\r")
    }

    function onRenameAcked() {
        if (!_waitingAck) return
        _waitingAck = false
        ackTimer.stop()
        Robot.setDeviceName(defaultName)
        statusMessage(tr("renamed").arg(defaultName))
        doUnpair()
    }

    function doUnpair() {
        if (Robot.connected) Robot.disconnectFromDevice()
        Robot.unpairDevice(_address)
    }

    function finalize(unpaired, message) {
        Session.clearActive()
        Session.saveWizardDismissed(false)
        Robot.setTransportPreference(Robot.TransportAuto)
        _active = false
        _initiatedConnect = false
        _waitingAck = false
        connectTimer.stop()
        ackTimer.stop()
        forgetFinished(unpaired, message)
    }

    Timer {
        id: connectTimer
        interval: 3000
        onTriggered: {
            if (!_active || _waitingAck) return
            // Could not reach the robot in time: forget anyway (best effort).
            _initiatedConnect = false
            doUnpair()
        }
    }

    Timer {
        id: ackTimer
        interval: 3000
        onTriggered: {
            if (!_waitingAck) return
            // Robot did not ACK the rename: forget anyway.
            _waitingAck = false
            statusMessage(tr("rename_skipped").arg(defaultName))
            doUnpair()
        }
    }

    Connections {
        target: Robot
        function onConnectedChanged() {
            if (!_active || _waitingAck) return
            if (Robot.connected && _initiatedConnect) {
                _initiatedConnect = false
                connectTimer.stop()
                if (Robot.deviceName && Robot.deviceName.toLowerCase() === defaultName.toLowerCase()) {
                    doUnpair()
                    return
                }
                sendRename()
            }
        }
        function onDataReceived(data) {
            if (_waitingAck && data.indexOf("&&F") !== -1)
                onRenameAcked()
        }
        function onUnpairFinished(success, message) {
            if (!_active) return
            // The registered Zowi is gone: finalize and fall back to Automatic.
            finalize(success, message)
        }
    }
}
