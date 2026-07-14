// ScanScreen: Bluetooth device discovery and connection screen.
// Scans for nearby Zowi devices, lists them, and allows the user
// to select and connect to one via Bluetooth SPP.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: scan
    screenName: "ScanScreen"

    title: tr("Searching for Zowis...")
    subtitle: tr("Searching for your Zowi, this may take a few seconds...")

    showBackButton: false
    showDisconnectButton: true

    onBackClicked: scan.back()
    onDisconnectClicked: Bluetooth.disconnectFromDevice()

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
                    text: tr("No devices found.\nPress Scan to start searching.")
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
                text: "\u2190 " + tr("Back")

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

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: Bluetooth.scanning
                      ? tr("Searching for devices...")
                      : tr("Found %1 devices").arg(deviceList.count)
                color: "#2d5a2d"
                font.pixelSize: 12
                opacity: 0.5
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
                    Bluetooth.connectToDevice(address)
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

        function onConnectionChanged() {
            if (Bluetooth.connected) {
                Session.saveActiveZowiDeviceAddress(Bluetooth.deviceAddress)
                namePanel.open()
            }
        }

        function onErrorOccurred(message) {
            errorDialog.text = message
            errorDialog.open()
        }
    }

    Popup {
        id: errorDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
        height: 180
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property string text: ""

        Column {
            anchors.centerIn: parent
            spacing: 20

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: tr("Error")
                color: "#e74c3c"
                font.bold: true
                font.pixelSize: 18
                font.family: "monospace"
            }

            Text {
                width: errorDialog.width - 60
                text: errorDialog.text
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                color: "#2d5a2d"
                font.pixelSize: 13
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: tr("Accept")

                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 20
                    color: "#21a69b"
                    implicitWidth: 100
                    implicitHeight: 36
                }

                onClicked: errorDialog.close()
            }
        }
    }

    Popup {
        id: namePanel
        anchors.centerIn: parent
        width: parent.width * 0.6
        height: 260
        modal: true
        closePolicy: Popup.NoAutoClose

        property string lastValid: "OpenZowi"
        property bool suppress: false

        function isValidName(name) {
            return name.match(/^[A-Za-z]{1,10}$/) !== null
        }

        onOpened: {
            var dn = Bluetooth.deviceName
            nameField.text = (dn !== "" && dn !== "#") ? dn : "OpenZowi"
            lastValid = nameField.text
            warningText.visible = false
        }

        Column {
            anchors.centerIn: parent
            spacing: 14

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: tr("Nombre del robot")
                color: "#2d5a2d"
                font.bold: true
                font.pixelSize: 18
                font.family: "monospace"
            }

            TextField {
                id: nameField
                anchors.horizontalCenter: parent.horizontalCenter
                width: namePanel.width - 80
                maximumLength: 10
                placeholderText: tr("OpenZowi")
                selectByMouse: true

                background: Rectangle {
                    radius: 8
                    border.color: "#21a69b"
                    border.width: 1
                    color: "#ffffff"
                }

                onTextChanged: {
                    if (namePanel.suppress) { namePanel.suppress = false; return }
                    var t = nameField.text
                    if (t === "") {
                        namePanel.lastValid = ""
                        warningText.visible = false
                        return
                    }
                    if (namePanel.isValidName(t)) {
                        namePanel.lastValid = t
                        warningText.visible = false
                        return
                    }
                    warningText.text = tr("Solo letras (A-Z), máximo 10 caracteres")
                    warningText.visible = true
                    namePanel.suppress = true
                    nameField.text = namePanel.lastValid
                }
            }

            Text {
                id: warningText
                anchors.horizontalCenter: parent.horizontalCenter
                text: tr("Solo letras (A-Z), máximo 10 caracteres")
                color: "#e74c3c"
                font.pixelSize: 12
                visible: false
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 16

                Button {
                    implicitWidth: 150
                    height: 44
                    text: tr("Renombrar")

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
                        color: parent.pressed ? "#17736c" : "#21a69b"
                    }

                    onClicked: {
                        var nm = nameField.text.trim()
                        if (nm === "") nm = "OpenZowi"
                        if (!namePanel.isValidName(nm)) {
                            warningText.text = tr("Nombre no válido")
                            warningText.visible = true
                            return
                        }
                        Bluetooth.sendData("R " + nm + "\r\n")
                        Bluetooth.setDeviceName(nm)
                        Session.saveActiveZowiName(nm)
                        namePanel.close()
                        scan.deviceSelected(nm, Bluetooth.deviceAddress)
                    }
                }

                Button {
                    implicitWidth: 150
                    height: 44
                    text: tr("Cancelar")

                    contentItem: Text {
                        text: parent.text
                        color: "#2d5a2d"
                        font.bold: true
                        font.pixelSize: 14
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

                    onClicked: {
                        namePanel.close()
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        devicesModel.clear()
        if (scanOnStart) {
            Bluetooth.startScan()
        } else {
            var storedAddr = Session.loadActiveZowiDeviceAddress()
            if (storedAddr !== "") {
                Bluetooth.connectToDevice(storedAddr)
            }
        }
    }
}
