// ScanScreen: Bluetooth device discovery and connection screen.
// Scans for nearby Zowi devices, lists them, and allows the user
// to select and connect to one via Bluetooth SPP.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: scan
    screenName: "ScanScreen"

    title: tr("searching_title")
    subtitle: tr("searching_subtitle")

    showBackButton: false
    showDisconnectButton: true

    onBackClicked: scan.back()
    onDisconnectClicked: Robot.disconnectFromDevice()

    signal deviceSelected(string name, string address)
    signal back()

    property bool scanOnStart: true
    property string macPrefix: Config.get("zowi_mac_prefix")

    function tr(source) { return Translator.translate("ScanScreen.qml", source) }

    Item {
        anchors.fill: parent

        Rectangle {
            id: listBox
            anchors {
                top: parent.top
                bottom: buttonRow.top
                bottomMargin: 20
                horizontalCenter: parent.horizontalCenter
            }
            width: parent.width * 0.5
            color: "#ffffff"
            radius: 12
            clip: true

            ListView {
                id: deviceList
                anchors.fill: parent
                anchors.margins: 1
                model: ListModel { id: devicesModel }
                delegate: deviceDelegate
                spacing: 2

                ScrollBar.vertical: ScrollBar {
                    active: true
                    width: 8
                    policy: ScrollBar.AlwaysOn

                    contentItem: Rectangle {
                        radius: 4
                        color: "#21a69b"
                        opacity: 0.6
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: tr("no_devices_hint")
                    color: "#2d5a2d"
                    font.pixelSize: 14
                    opacity: 0.4
                    horizontalAlignment: Text.AlignHCenter
                    visible: deviceList.count === 0
                }
            }
        }

        Row {
            id: buttonRow
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }
            spacing: 20

            Button {
                id: backButton
                implicitWidth: 160
                height: 44
                text: "\u2190 " + tr("back")

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 14
                    font.bold: true
                    color: "#2d5a2d"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: 0.8
                }

                background: Rectangle {
                    radius: 22
                    color: "transparent"
                    border.color: "#2d5a2d"
                    border.width: 2
                    opacity: 0.5
                }

                onClicked: scan.back()
            }

            Button {
                id: scanButton
                implicitWidth: 160
                height: 44
                visible: Config.get("button_scan_visible") === "true"
                text: Robot.scanning
                      ? tr("scanning")
                      : tr("scan")

                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 22
                    color: Robot.scanning ? "#999999" : (scanButton.pressed ? "#17736c" : "#21a69b")
                }

                onClicked: {
                    if (Robot.scanning)
                        Robot.stopScan()
                    else
                        Robot.startScan()
                }
            }
        }
    }

    Component {
        id: deviceDelegate

        Rectangle {
            width: deviceList.width
            height: 56
            color: mouse.containsMouse ? "#eaf5ea" : "transparent"

            MouseArea {
                id: mouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    Robot.stopScan()
                    Session.saveActiveZowiDeviceAddress(address)
                    Session.saveActiveZowiName(deviceName)
                    scan.deviceSelected(deviceName, address)
                }
            }

            Row {
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: 15
                }
                spacing: 12

                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#21a69b"

                    Text {
                        anchors.centerIn: parent
                        text: "Z"
                        color: "#ffffff"
                        font.bold: true
                        font.pixelSize: 18
                        font.family: "monospace"
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    Text {
                        text: deviceName
                        color: "#2d5a2d"
                        font.pixelSize: 14
                        font.bold: true
                        elide: Text.ElideRight
                        width: deviceList.width - 130
                    }

                    Text {
                        text: address
                        color: "#2d5a2d"
                        font.pixelSize: 11
                        opacity: 0.5
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "\u203A"
                    color: "#21a69b"
                    font.pixelSize: 22
                    font.bold: true
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#e0e0e0"
            }
        }
    }

    Connections {
        target: Robot

        function onDeviceDiscovered(name, address) {
            for (var i = 0; i < devicesModel.count; i++) {
                if (devicesModel.get(i).address === address)
                    return;
            }

            var nameMatch = name.toLowerCase().indexOf("zowi") !== -1
            var macMatch = macPrefix !== "" &&
                           address.toUpperCase().indexOf(macPrefix.toUpperCase()) === 0

            if (nameMatch || macMatch) {
                devicesModel.append({ deviceName: name, address: address })
            }
        }

        function onScanFinished() {
            if (devicesModel.count === 0) {
                devicesModel.append({ deviceName: tr("no_devices_item"), address: "" })
            }
        }

        function onConnectionChanged() {
            if (Robot.connected) {
                Session.saveActiveZowiDeviceAddress(Robot.deviceAddress)
            }
        }
    }



    StackView.onActivated: {
        devicesModel.clear()
        Robot.startScan()
    }

    StackView.onDeactivated: Robot.stopScan()
}
