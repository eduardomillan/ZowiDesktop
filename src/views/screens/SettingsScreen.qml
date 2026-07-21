// SettingsScreen: app configuration hub (mirrors ZowiAppReborn's settings).
// Options: restore firmware (stub), rename (reuses WizardRenameScreen),
// search updates (stub), delete achievements (stub), forget Zowi (like
// SplashScreen reset), calibrate (stub), and Hospital (external link).
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: settingsScreen
    screenName: "SettingsScreen"
    title: tr("settings")
    showBackButton: true

    signal renameRequested()
    signal forgetCompleted()

    property bool forgetting: false
    property bool _forgettingNoZowi: false
    property bool restoring: false
    property bool switching: false
    property bool batteryLow: false
    property int restoreProgress: 0

    function tr(source) { return Translator.translate("SettingsScreen.qml", source) }

    function transportLabel(t) {
        if (t === Robot.TransportUsb) return tr("via_usb")
        if (t === Robot.TransportBluetooth) return tr("via_bluetooth")
        return tr("via_bluetooth")
    }

    function registeredTransportLabel() {
        var r = Session.loadActiveZowiTransport()
        if (r === "usb") return tr("via_usb")
        if (r === "bluetooth") return tr("via_bluetooth")
        return transportLabel(Robot.activeTransport)
    }

    // --- Connection situation helpers -------------------------------------
    function connectionStatusText() {
        var s = Robot.situation
        if (s === Robot.SituationConnected)
            return tr("status_connected").arg(transportLabel(Robot.activeTransport))
        if (s === Robot.SituationConnecting)
            return tr("status_connecting")
        if (s === Robot.SituationDisconnected)
            return tr("status_disconnected")
        if (s === Robot.SituationTransportLost)
            return tr("status_transport_lost").arg(registeredTransportLabel())
        if (s === Robot.SituationUnregistered)
            return tr("status_unregistered")
        return tr("status_demo")
    }

    function connectionStatusColor() {
        var s = Robot.situation
        if (s === Robot.SituationConnected)
            return Config.get("statusbar_fg_connected") || "#2d5a2d"
        if (s === Robot.SituationConnecting)
            return Config.get("statusbar_fg_connected") || "#2d5a2d"
        if (s === Robot.SituationDemo || s === Robot.SituationTransportLost)
            return Config.get("statusbar_fg_low_battery") || "#c0392b"
        return Config.get("statusbar_fg_firmware") || "#8a6d1f"
    }

    // Returns the list of contextual actions for the current situation. Each
    // entry is { label: <i18n key>, action: <fn> }.
    function connectionActions() {
        var s = Robot.situation
        var acts = []
        if (s === Robot.SituationDisconnected) {
            acts.push({ label: "action_retry", action: settingsScreen.retryConnection })
            acts.push({ label: "action_forget_reconfigure", action: settingsScreen.forgetZowi })
        } else if (s === Robot.SituationTransportLost) {
            acts.push({ label: "action_retry", action: settingsScreen.retryConnection })
            acts.push({ label: "action_forget_reconfigure", action: settingsScreen.forgetZowi })
        } else if (s === Robot.SituationUnregistered) {
            acts.push({ label: "action_register", action: settingsScreen.startRegistration })
        } else if (s === Robot.SituationDemo) {
            acts.push({ label: "action_refresh", action: settingsScreen.refreshTransports })
        }
        // USB quick-connect whenever USB is present but we are not connected.
        if (Robot.usbAvailable && !Robot.connected &&
            s !== Robot.SituationUnregistered) {
            acts.push({ label: "action_connect_usb", action: settingsScreen.connectUsb })
        }
        return acts
    }

    function retryConnection() {
        if (msgBar.visible || settingsScreen.restoring || settingsScreen.switching) return
        settingsScreen.switching = true
        var ok = Robot.switchTransport(Robot.TransportAuto)
        settingsScreen.switching = false
        if (ok) msgBar.show(tr("transport_switched"))
        else msgBar.show(tr("transport_failed"), "#c0392b")
    }

    function connectUsb() {
        if (msgBar.visible || settingsScreen.restoring) return
        Robot.connectUsb()
    }

    function refreshTransports() {
        Robot.refreshTransports()
    }

    function startRegistration() {
        if (msgBar.visible || settingsScreen.restoring) return
        Session.saveWizardDismissed(false)
        Robot.setTransportPreference(Robot.TransportAuto)
        settingsScreen.forgetCompleted()
    }

    // `connGated: true` options are only usable while a Zowi is connected
    // (mirrors ZowiAppReborn's zowiDependantViews: rename, restore firmware,
    // calibrate). The rest are always available.
    property var options: [
        { key: "restore",        desc: "restore_desc",        connGated: true,  action: function() { settingsScreen.restoreFirmware() } },
        { key: "rename",         desc: "rename_desc",         connGated: true,  action: function() { settingsScreen.renameRequested() } },
        { key: "update",         desc: "update_desc",         connGated: false, action: function() { msgBar.show(tr("update_stub")) } },
        { key: "achievements",   desc: "achievements_desc",   connGated: false, action: function() { msgBar.show(tr("achievements_stub")) } },
        { key: "forget",         desc: "forget_desc",         connGated: false, action: function() { settingsScreen.forgetZowi() } },
        { key: "calibrate",      desc: "calibrate_desc",      connGated: true,  action: function() { msgBar.show(tr("calibrate_stub")) } },
        { key: "hospital",       desc: "hospital_desc",       connGated: false, action: function() { Qt.openUrlExternally(Config.get("hospital_url")) } }
    ]

    function restoreFirmware() {
        if (msgBar.visible) return
        if (settingsScreen.restoring) return
        if (!Robot.connected) {
            msgBar.show(tr("restore_failed"), "#c0392b")
            return
        }
        settingsScreen.restoring = true
        restoreProgress = 0
        msgBar.show(tr("restore_started"))
        // Phase 2: restoreFirmware() runs synchronously on the GUI thread and
        // reports progress and outcome via dedicated signals
        // (onFirmwareRestoreStarted / Progress / Finished).
        Robot.restoreFirmware(Config.get("factory_firmware_path"))
    }

    function forgetZowi() {
        if (msgBar.visible) return
        var addr = Session.loadActiveZowiDeviceAddress()
        if (!addr) addr = Robot.deviceAddress
        settingsScreen._forgettingNoZowi = (addr === "")
        settingsScreen.forgetting = true
        // Forget the registered Zowi. The controller also tries a factory
        // rename (zowi_default_name) if it can reach the robot first.
        forgetter.forget(addr)
    }

    ForgetController {
        id: forgetter
        onForgetFinished: function(unpaired, message) {
            settingsScreen.forgetting = false
            if (settingsScreen._forgettingNoZowi)
                msgBar.show(tr("reset_no_zowi"), "#c0392b")
            else if (unpaired)
                msgBar.show(tr("unpair_success"))
            else
                msgBar.show(tr("unpair_app_only"))
            settingsScreen.forgetCompleted()
        }
        onStatusMessage: function(text) { msgBar.show(text) }
    }

    Connections {
        target: Robot
        // Phase 2: restore progress/outcome arrive through dedicated signals
        // instead of being multiplexed onto errorOccurred.
        function onFirmwareRestoreStarted() {
            if (!settingsScreen.restoring) return
            restoreProgress = 0
        }
        function onFirmwareRestoreProgress(percent, written, total) {
            if (!settingsScreen.restoring) return
            restoreProgress = percent
        }
        function onFirmwareRestoreFinished(success, message) {
            if (!settingsScreen.restoring) return
            settingsScreen.restoring = false
            settingsScreen.batteryLow = false
            restoreProgress = success ? 100 : 0
            if (success)
                msgBar.show(tr("restore_success"))
            else
                msgBar.show(tr("restore_failed"), "#c0392b")
        }
        // Phase 3: the restore completed the upload but the reported battery is
        // below the safe threshold; ask the user to confirm before finishing.
        function onFirmwareRestoreBatteryLow(level) {
            if (!settingsScreen.restoring) return
            settingsScreen.batteryLow = true
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: optionCol.height
        clip: true

        Column {
            id: optionCol
            width: parent.width

            // --- Connection status + contextual actions ---
            // The transport is no longer chosen manually: the controller's
            // situation state machine (see .local/transport_thoughts.md) decides
            // it and we surface the state plus the actions that make sense now.
            Rectangle {
                width: optionCol.width
                height: connCol.implicitHeight + 28
                color: "#f4f9f4"

                Column {
                    id: connCol
                    anchors {
                        left: parent.left; right: parent.right
                        top: parent.top; topMargin: 14
                        leftMargin: 18; rightMargin: 18
                    }
                    spacing: 10

                    Text {
                        text: settingsScreen.tr("connection")
                        font.bold: true
                        font.pixelSize: 16
                        color: "#2d5a2d"
                        opacity: (settingsScreen.restoring || settingsScreen.switching) ? 0.45 : 1.0
                    }

                    Text {
                        width: parent.width
                        text: settingsScreen.connectionStatusText()
                        font.pixelSize: 13
                        color: settingsScreen.connectionStatusColor()
                        opacity: (settingsScreen.restoring || settingsScreen.switching) ? 0.45 : 1.0
                        wrapMode: Text.WordWrap
                    }

                    // Contextual actions for the current situation.
                    Flow {
                        width: parent.width
                        spacing: 8
                        enabled: !settingsScreen.restoring && !settingsScreen.switching
                        opacity: (settingsScreen.restoring || settingsScreen.switching) ? 0.45 : 1.0
                        Repeater {
                            model: settingsScreen.connectionActions()
                            delegate: Rectangle {
                                height: 40
                                radius: 8
                                width: actionLabel.implicitWidth + 28
                                color: "#ffffff"
                                border.color: "#21a69b"
                                border.width: 1
                                Text {
                                    id: actionLabel
                                    anchors.centerIn: parent
                                    text: settingsScreen.tr(modelData.label)
                                    color: "#2d5a2d"
                                    font.pixelSize: 13
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    enabled: !settingsScreen.restoring && !settingsScreen.switching
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: modelData.action()
                                }
                            }
                        }
                    }
                }
            }

            Repeater {
                model: settingsScreen.options
                delegate: Rectangle {
                    id: optionRow
                    // Connection-dependent options are enabled only while a
                    // Zowi is connected; the rest are always enabled. Any
                    // option that pops a message bar is locked while a message
                    // is visible, and re-evaluates its state once it clears.
                    property bool effectiveEnabled: (!msgBar.visible) &&
                        (!settingsScreen.restoring) &&
                        (!settingsScreen.switching) &&
                        (modelData.connGated ? Robot.connected : true)

                    width: optionCol.width
                    height: 76
                    enabled: optionRow.effectiveEnabled
                    color: rowMouse.pressed ? "#e0f0e0" : (index % 2 ? "#ffffff" : "#f4f9f4")
                    opacity: optionRow.effectiveEnabled ? 1.0 : 0.45

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        onClicked: {
                            if (!optionRow.effectiveEnabled) return
                            if (msgBar.visible) return
                            modelData.action()
                        }
                    }

                    Column {
                        anchors {
                            left: parent.left
                            leftMargin: 18
                            verticalCenter: parent.verticalCenter
                        }
                        spacing: 4
                        width: parent.width - 70

                        Text {
                            text: settingsScreen.tr(modelData.key)
                            font.bold: true
                            font.pixelSize: 16
                            color: optionRow.effectiveEnabled ? "#2d5a2d" : "#777777"
                        }
                        Text {
                            text: settingsScreen.tr(modelData.desc)
                            font.pixelSize: 12
                            color: optionRow.effectiveEnabled ? "#2d5a2d" : "#777777"
                            opacity: 0.7
                            wrapMode: Text.WordWrap
                        }
                    }

                    Text {
                        anchors {
                            right: parent.right
                            rightMargin: 18
                            verticalCenter: parent.verticalCenter
                        }
                        text: "›"
                        font.pixelSize: 26
                        color: "#21a69b"
                        visible: optionRow.effectiveEnabled
                    }
                }
            }
        }
    }

    MessageBar {
        id: msgBar
        duration: parseInt(Config.get("message_duration")) || 2000
    }

    // Restore progress (Phase 2): occupies the same bottom position as the
    // MessageBar while a firmware restore runs, covering it (z above). The
    // status text (yellow) sits above the progress bar.
    Rectangle {
        id: restoreProgressBox
        visible: settingsScreen.restoring
        z: 1
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 56
        color: "#17736c"

        Text {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                topMargin: 6
            }
            horizontalAlignment: Text.AlignHCenter
            text: tr("restore_progress").arg(settingsScreen.restoreProgress)
            color: "#f1c40f"
            font.pixelSize: 13
            font.bold: true
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 40
                rightMargin: 40
                bottom: parent.bottom
                bottomMargin: 10
            }
            height: 10
            radius: 5
            color: "#0f4f4a"

            Rectangle {
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                }
                width: Math.max(2, parent.width * (settingsScreen.restoreProgress / 100.0))
                radius: 5
                color: "#21a69b"
            }
        }
    }

    // Phase 3: low-battery confirmation dialog shown over the progress bar while
    // the restore waits for the user's decision. Styled to match the app theme:
    // light app background, a warning-yellow panel with a dark-green border.
    Rectangle {
        id: batteryLowDialog
        visible: settingsScreen.batteryLow
        anchors.fill: parent
        color: "transparent"

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 360)
            height: confirmColumn.height + 36
            radius: 12
            color: "#fdfbe7"
            border.color: "#2d5a2d"
            border.width: 2

            Column {
                id: confirmColumn
                anchors.centerIn: parent
                width: parent.width - 36
                spacing: 14

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: tr("restore_battery_low")
                    color: "#2d5a2d"
                    font.pixelSize: 14
                    font.bold: true
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 16

                    Button {
                        text: tr("confirm")
                        background: Rectangle {
                            color: "#21a69b"
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
                            settingsScreen.batteryLow = false
                            Robot.confirmRestoreBattery(true)
                        }
                    }

                    Button {
                        text: tr("cancel")
                        background: Rectangle {
                            color: "#e74c3c"
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
                            settingsScreen.batteryLow = false
                            Robot.confirmRestoreBattery(false)
                        }
                    }
                }
            }
        }
    }
}
