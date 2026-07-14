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

    function tr(source) { return Translator.translate("SettingsScreen.qml", source) }

    property var options: [
        { key: "restore",        desc: "restore_desc",        enabled: false, action: function() { msgBar.show(tr("restore_stub")) } },
        { key: "rename",         desc: "rename_desc",         enabled: true,  action: function() { settingsScreen.renameRequested() } },
        { key: "update",         desc: "update_desc",         enabled: false, action: function() { msgBar.show(tr("update_stub")) } },
        { key: "achievements",   desc: "achievements_desc",   enabled: false, action: function() { msgBar.show(tr("achievements_stub")) } },
        { key: "forget",         desc: "forget_desc",         enabled: true,  action: function() { settingsScreen.forgetZowi() } },
        { key: "calibrate",      desc: "calibrate_desc",      enabled: false, action: function() { msgBar.show(tr("calibrate_stub")) } },
        { key: "hospital",       desc: "hospital_desc",       enabled: true,  action: function() { Qt.openUrlExternally(Config.get("hospital_url")) } }
    ]

    function forgetZowi() {
        var addr = Session.loadActiveZowiDeviceAddress()
        if (!addr) addr = Bluetooth.deviceAddress
        if (!addr) {
            Session.saveActiveZowiDeviceAddress("")
            Session.saveActiveZowiName("")
            Session.saveWizardDismissed(false)
            msgBar.show(tr("reset_no_zowi"), "#c0392b")
            return
        }
        if (Bluetooth.connected) Bluetooth.disconnectFromDevice()
        Bluetooth.unpairDevice(addr)
        Session.saveActiveZowiDeviceAddress("")
        Session.saveActiveZowiName("")
        Session.saveWizardDismissed(false)
    }

    Connections {
        target: Bluetooth
        function onUnpairFinished(success, message) {
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
                    width: optionCol.width
                    height: 76
                    enabled: modelData.enabled
                    color: rowMouse.pressed ? "#e0f0e0" : (index % 2 ? "#ffffff" : "#f4f9f4")
                    opacity: modelData.enabled ? 1.0 : 0.45

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        onClicked: {
                            if (!modelData.enabled) return
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
                            color: modelData.enabled ? "#2d5a2d" : "#777777"
                        }
                        Text {
                            text: settingsScreen.tr(modelData.desc)
                            font.pixelSize: 12
                            color: modelData.enabled ? "#2d5a2d" : "#777777"
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
                        visible: modelData.enabled
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
