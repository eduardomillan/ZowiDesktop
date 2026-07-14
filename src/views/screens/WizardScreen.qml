// WizardScreen: Bluetooth pairing wizard entry.
// Scans for nearby Zowis and displays them; clicking one saves the
// device info and transitions to WizardFoundScreen for pairing.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: wizard
    screenName: "WizardScreen"

    title: tr("Welcome!")
    subtitle: tr("To control Zowi, first turn it on.")

    signal deviceSelected(string name, string address)
    signal dismissed()

    property string macPrefix: Config.get("zowi_mac_prefix")

    function tr(source) { return Translator.translate("WizardScreen.qml", source) }

    ListModel { id: devicesModel }

    Item {
        anchors.fill: parent

        Rectangle {
            id: listBox
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                bottom: buttonRow.top
                bottomMargin: 15
            }
            color: "#ffffff"
            radius: 12
            clip: true

            ListView {
                id: deviceList
                anchors.fill: parent
                anchors.margins: 1
                model: devicesModel
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
                    text: tr("No devices found.\nMake sure Zowi is turned on.")
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
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            spacing: 20

            Button {
                id: scanButton
                implicitWidth: 200
                height: 44
                text: Bluetooth.scanning
                      ? tr("Scanning...")
                      : tr("Scan")

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
                    color: Bluetooth.scanning ? "#999999" : (scanButton.pressed ? "#17736c" : "#21a69b")
                }

                onClicked: {
                    if (Bluetooth.scanning)
                        Bluetooth.stopScan()
                    else
                        Bluetooth.startScan()
                }
            }

            Button {
                id: notReadyButton
                implicitWidth: 200
                height: 44
                text: tr("I don't have a Zowi")

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

                onClicked: wizard.dismissed()
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
                    Bluetooth.stopScan()
                    Session.saveActiveZowiDeviceAddress(address)
                    Session.saveActiveZowiName(deviceName)
                    wizard.deviceSelected(deviceName, address)
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
        target: Bluetooth

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
                devicesModel.append({ deviceName: tr("(no devices found)"), address: "" })
            }
        }

        function onErrorOccurred(message) {
            if (devicesModel.count === 0) {
                devicesModel.append({ deviceName: tr("(scan error: %1)").arg(message), address: "" })
            }
        }
    }

    Component.onCompleted: {
        devicesModel.clear()
        Bluetooth.startScan()
    }
}
