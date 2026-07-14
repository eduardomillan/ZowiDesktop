// Reusable connection status bar. Shows connected/disconnected state and,
// when connected, the robot's remaining battery percentage. Bind it wherever a
// status bar is needed (it reads the global `Bluetooth` context property).
import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    height: 36
    color: Bluetooth.connected
          ? (Bluetooth.battery >= 0 && Bluetooth.battery < 50 ? "#fdecea" : "#e8f5e8")
          : "#fff3e0"

    function tr(source) { return Translator.translate("StatusBar.qml", source) }

    property color dotColor: Bluetooth.connected
        ? (Bluetooth.battery >= 0 && Bluetooth.battery < 50 ? "#c0392b" : "#2d5a2d")
        : "#e67e22"

    Row {
        anchors.centerIn: parent
        spacing: 8

        Rectangle {
            width: 8
            height: 8
            radius: 4
            anchors.verticalCenter: parent.verticalCenter
            color: root.dotColor
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            color: root.dotColor
            font.pixelSize: 12
            opacity: 0.8
            text: {
                if (!Bluetooth.connected) return root.tr("not_connected")
                var name = Session.loadActiveZowiName() || Bluetooth.deviceName || ""
                var mac = Bluetooth.deviceAddress ? Bluetooth.deviceAddress : ""
                function withMac(base) {
                    return mac !== "" ? base + " · " + mac : base
                }
                if (Bluetooth.battery >= 0)
                    return withMac(name !== ""
                        ? root.tr("connected_name_battery").arg(name).arg(Bluetooth.battery)
                        : root.tr("connected_battery").arg(Bluetooth.battery))
                return withMac(name !== "" ? root.tr("connected_name").arg(name) : root.tr("connected"))
            }
        }
    }
}
