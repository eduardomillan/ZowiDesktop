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
    property bool restoring: false
    property bool switching: false
    property bool batteryLow: false
    property int restoreProgress: 0

    function tr(source) { return Translator.translate("SettingsScreen.qml", source) }

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
        if (!Bluetooth.connected) {
            msgBar.show(tr("restore_failed"), "#c0392b")
            return
        }
        settingsScreen.restoring = true
        restoreProgress = 0
        msgBar.show(tr("restore_started"))
        // Phase 2: restoreFirmware() runs synchronously on the GUI thread and
        // reports progress and outcome via dedicated signals
        // (onFirmwareRestoreStarted / Progress / Finished).
        Bluetooth.restoreFirmware(Config.get("factory_firmware_path"))
    }

    function forgetZowi() {
        if (msgBar.visible) return
        var addr = Session.loadActiveZowiDeviceAddress()
        if (!addr) addr = Bluetooth.deviceAddress
        if (!addr) {
            // Nothing registered to forget: just drop any stale app data and
            // fall back to Automatic transport.
            settingsScreen.forgetting = false
            Session.saveActiveZowiDeviceAddress("")
            Session.saveActiveZowiName("")
            Session.saveWizardDismissed(false)
            Bluetooth.setTransportPreference(Bluetooth.TransportAuto)
            msgBar.show(tr("reset_no_zowi"), "#c0392b")
            return
        }
        settingsScreen.forgetting = true
        if (Bluetooth.connected) Bluetooth.disconnectFromDevice()
        Bluetooth.unpairDevice(addr)
        // Forget the registered Zowi: drop every "active*" session key and
        // mark the wizard as not dismissed so the app returns to a clean state.
        Session.clearActive()
        Session.saveWizardDismissed(false)
        settingsScreen.forgetCompleted()
    }

    Connections {
        target: Bluetooth
        function onUnpairFinished(success, message) {
            if (!settingsScreen.forgetting) return
            settingsScreen.forgetting = false
            // The registered Zowi is gone: fall back to Automatic transport.
            Bluetooth.setTransportPreference(Bluetooth.TransportAuto)
            if (success)
                msgBar.show(tr("unpair_success"))
            else
                msgBar.show(tr("unpair_app_only"))
        }
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

            // --- Connection (transport) selector ---
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
                        text: settingsScreen.tr("connection_desc")
                        font.pixelSize: 12
                        color: "#2d5a2d"
                        opacity: (settingsScreen.restoring || settingsScreen.switching) ? 0.45 : 0.7
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }

                    // Segmented control: Automatic / Bluetooth / USB.
                    Row {
                        spacing: 8
                        // Lock transport switching while a firmware restore is
                        // running or a transport switch is in progress.
                        enabled: !settingsScreen.restoring && !settingsScreen.switching
                        opacity: (settingsScreen.restoring || settingsScreen.switching) ? 0.45 : 1.0
                        Repeater {
                            model: [
                                { t: Bluetooth.TransportAuto,      label: "transport_auto",      hint: "transport_auto_hint", avail: true },
                                { t: Bluetooth.TransportBluetooth, label: "transport_bluetooth", hint: "transport_bt_hint",   avail: Bluetooth.bluetoothAvailable },
                                { t: Bluetooth.TransportUsb,       label: "transport_usb",       hint: "transport_usb_hint",  avail: Bluetooth.usbAvailable }
                            ]
                            delegate: Rectangle {
                                property bool isSel: Bluetooth.transport === modelData.t
                                width: 118
                                height: 40
                                radius: 8
                                enabled: modelData.avail
                                opacity: modelData.avail ? 1.0 : 0.4
                                color: isSel ? "#21a69b" : "#ffffff"
                                border.color: isSel ? "#21a69b" : "#cfe3cf"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: settingsScreen.tr(modelData.label)
                                    color: parent.isSel ? "#ffffff" : "#2d5a2d"
                                    font.pixelSize: 13
                                    font.bold: parent.isSel
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    enabled: modelData.avail && !settingsScreen.restoring && !settingsScreen.switching
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        settingsScreen.switching = true
                                        var ok = Bluetooth.switchTransport(modelData.t)
                                        settingsScreen.switching = false
                                        if (ok)
                                            msgBar.show(settingsScreen.tr("transport_switched"))
                                        else
                                            msgBar.show(settingsScreen.tr("transport_failed"), "#c0392b")
                                    }
                                }
                            }
                        }
                    }

                    // USB quick actions, shown when USB is the chosen/active transport.
                    Row {
                        spacing: 10
                        enabled: !settingsScreen.restoring && !settingsScreen.switching
                        opacity: (settingsScreen.restoring || settingsScreen.switching) ? 0.45 : 1.0
                        visible: Bluetooth.transport === Bluetooth.TransportUsb ||
                                 Bluetooth.activeTransport === Bluetooth.TransportUsb
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            font.pixelSize: 12
                            color: Bluetooth.usbAvailable ? "#2d5a2d" : "#c0392b"
                            text: Bluetooth.usbAvailable
                                  ? settingsScreen.tr("usb_connect")
                                  : settingsScreen.tr("usb_no_ports")
                                MouseArea {
                                    anchors.fill: parent
                                    enabled: Bluetooth.usbAvailable && !Bluetooth.connected && !settingsScreen.restoring
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: Bluetooth.connectUsb()
                                }
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: settingsScreen.tr("usb_refresh")
                            color: "#2980b9"
                            font.pixelSize: 12
                            font.underline: true
                                MouseArea {
                                    anchors.fill: parent
                                    enabled: !settingsScreen.restoring
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: Bluetooth.refreshTransports()
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
                        (modelData.connGated ? Bluetooth.connected : true)

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
    // the worker thread waits for the user's decision.
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
            color: "#1f2a2a"
            border.color: "#f1c40f"
            border.width: 1

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
                    color: "#ffffff"
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
                            Bluetooth.confirmRestoreBattery(true)
                        }
                    }

                    Button {
                        text: tr("cancel")
                        background: Rectangle {
                            color: "#c0392b"
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
                            Bluetooth.confirmRestoreBattery(false)
                        }
                    }
                }
            }
        }
    }
}
