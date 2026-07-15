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

    function tr(source) { return Translator.translate("SettingsScreen.qml", source) }

    // `connGated: true` options are only usable while a Zowi is connected
    // (mirrors ZowiAppReborn's zowiDependantViews: rename, restore firmware,
    // calibrate). The rest are always available.
    property var options: [
        { key: "restore",        desc: "restore_desc",        connGated: true,  action: function() { msgBar.show(tr("restore_stub")) } },
        { key: "rename",         desc: "rename_desc",         connGated: true,  action: function() { settingsScreen.renameRequested() } },
        { key: "update",         desc: "update_desc",         connGated: false, action: function() { msgBar.show(tr("update_stub")) } },
        { key: "achievements",   desc: "achievements_desc",   connGated: false, action: function() { msgBar.show(tr("achievements_stub")) } },
        { key: "forget",         desc: "forget_desc",         connGated: false, action: function() { settingsScreen.forgetZowi() } },
        { key: "calibrate",      desc: "calibrate_desc",      connGated: true,  action: function() { msgBar.show(tr("calibrate_stub")) } },
        { key: "hospital",       desc: "hospital_desc",       connGated: false, action: function() { Qt.openUrlExternally(Config.get("hospital_url")) } }
    ]

    function forgetZowi() {
        if (msgBar.visible) return
        var addr = Session.loadActiveZowiDeviceAddress()
        if (!addr) addr = Bluetooth.deviceAddress
        if (!addr) {
            // Nothing registered to forget: just drop any stale app data.
            settingsScreen.forgetting = false
            Session.saveActiveZowiDeviceAddress("")
            Session.saveActiveZowiName("")
            Session.saveWizardDismissed(false)
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
            if (success)
                msgBar.show(tr("unpair_success"))
            else
                msgBar.show(tr("unpair_app_only"))
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: optionCol.height
        clip: true

        Column {
            id: optionCol
            width: parent.width

            Repeater {
                model: settingsScreen.options
                delegate: Rectangle {
                    id: optionRow
                    // Connection-dependent options are enabled only while a
                    // Zowi is connected; the rest are always enabled. Any
                    // option that pops a message bar is locked while a message
                    // is visible, and re-evaluates its state once it clears.
                    property bool effectiveEnabled: (!msgBar.visible) &&
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
}
