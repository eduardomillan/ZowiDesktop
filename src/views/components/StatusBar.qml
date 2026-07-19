// Reusable connection status bar. Shows connecting / connected / unavailable
// state and, when connected, the robot's remaining battery percentage. Bind it
// wherever a status bar is needed (it reads the global `Bluetooth` and
// `Session` context properties).
import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    height: 36
    color: Robot.connecting
          ? "#eaf2fb"
          : (Robot.connected
              ? (Robot.battery >= 0 && Robot.battery < 50 ? "#fdecea" : "#e8f5e8")
              : "#fff3e0")

    function tr(source) { return Translator.translate("StatusBar.qml", source) }

    property bool hasSavedZowi: Session.loadActiveZowiDeviceAddress() !== ""
    property bool canReconnect: !Robot.connected && !Robot.connecting && root.hasSavedZowi

    property color dotColor: Robot.connecting
        ? "#2980b9"
        : (Robot.connected
            ? (Robot.battery >= 0 && Robot.battery < 50 ? "#c0392b" : "#2d5a2d")
            : "#e67e22")

    function statusText() {
        if (Robot.connecting) return root.tr("status_connecting")
        if (Robot.connected) {
            var name = Session.loadActiveZowiName() || Robot.deviceName || ""
            var mac = Robot.deviceAddress ? Robot.deviceAddress : ""
            function withMac(base) {
                return mac !== "" ? base + " · " + mac : base
            }
            if (Robot.battery >= 0)
                return withMac(name !== ""
                    ? root.tr("connected_name_battery").arg(name).arg(Robot.battery)
                    : root.tr("connected_battery").arg(Robot.battery))
            return withMac(name !== "" ? root.tr("connected_name").arg(name) : root.tr("connected"))
        }
        if (root.hasSavedZowi) return root.tr("status_unavailable")
        return root.tr("not_connected")
    }

    property string statusLabel: root.statusText()

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
            text: root.statusLabel
        }

        // Active transport pill (only meaningful while connected).
        Rectangle {
            visible: Robot.connected
            anchors.verticalCenter: parent.verticalCenter
            radius: 8
            height: 16
            width: transportText.implicitWidth + 14
            color: "#ffffff"
            border.color: root.dotColor
            border.width: 1
            opacity: 0.75

            Text {
                id: transportText
                anchors.centerIn: parent
                font.pixelSize: 10
                font.bold: true
                color: root.dotColor
                text: Robot.activeTransport === Robot.TransportUsb
                      ? root.tr("via_usb")
                      : root.tr("via_bluetooth")
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
            onClicked: Robot.connectToDevice(Session.loadActiveZowiDeviceAddress())
        }
    }

    Connections {
        target: Session
        function onSessionChanged() { root.statusLabel = root.statusText() }
    }
}
