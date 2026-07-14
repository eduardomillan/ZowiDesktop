// Reusable connection status bar. Shows connecting / connected / unavailable
// state and, when connected, the robot's remaining battery percentage. Bind it
// wherever a status bar is needed (it reads the global `Bluetooth` and
// `Session` context properties).
import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    height: 36
    color: Bluetooth.connecting
          ? "#eaf2fb"
          : (Bluetooth.connected
              ? (Bluetooth.battery >= 0 && Bluetooth.battery < 50 ? "#fdecea" : "#e8f5e8")
              : "#fff3e0")

    function tr(source) { return Translator.translate("StatusBar.qml", source) }

    property bool hasSavedZowi: Session.loadActiveZowiDeviceAddress() !== ""
    property bool canReconnect: !Bluetooth.connected && !Bluetooth.connecting && root.hasSavedZowi

    property color dotColor: Bluetooth.connecting
        ? "#2980b9"
        : (Bluetooth.connected
            ? (Bluetooth.battery >= 0 && Bluetooth.battery < 50 ? "#c0392b" : "#2d5a2d")
            : "#e67e22")

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
                if (Bluetooth.connecting) return root.tr("status_connecting")
                if (Bluetooth.connected) {
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
                if (root.hasSavedZowi) return root.tr("status_unavailable")
                return root.tr("not_connected")
            }
        }
    }

    Text {
        visible: root.canReconnect
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
            rightMargin: 12
        }
        text: root.tr("status_reconnect")
        color: "#2980b9"
        font.pixelSize: 12
        font.bold: true
        font.underline: true

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: Bluetooth.connectToDevice(Session.loadActiveZowiDeviceAddress())
        }
    }
}
