// ScanScreen: Bluetooth device discovery and connection screen.
// Scans for nearby Zowi devices, lists them, and allows the user
// to select and connect to one via Bluetooth SPP.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

Rectangle {
    id: scan
    color: "#f4f9f4"

    property bool scanOnStart: true
    property string macPrefix: Config.get("zowi_mac_prefix")

    signal deviceSelected(string name, string address)
    signal back()

    property string screenName: "ScanScreen"

    function tr(source) { return Translator.translate("ScanScreen.qml", source) }

    // Connection status bar (top of the screen)
    StatusBar {
        id: statusBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 5
        }
    }

    Column {
        id: topColumn
        anchors {
            top: statusBar.bottom
            left: parent.left
            right: parent.right
            margins: 30
        }
        spacing: 20

        Row {
            width: parent.width
            spacing: 15

            Button {
                id: backButton
                width: 40
                height: 40

                contentItem: Text {
                    text: "\u2190"
                    font.pixelSize: 22
                    color: "#2d5a2d"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 20
                    color: backButton.pressed ? "#e0f0e0" : "transparent"
                    border.color: "#2d5a2d"
                    border.width: 1
                    opacity: 0.4
                }

                onClicked: scanScreen.back()
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: tr("Search for Zowi")
                color: "#2d5a2d"
                font.pixelSize: 24
                font.bold: true
                font.family: "monospace"
            }

            Item {
                width: Math.max(8, parent.width - backButton.width - 320)
                height: 1
            }

            Button {
                id: disconnectBtn
                visible: Bluetooth.connected
                implicitWidth: 100
                height: 32

                text: tr("Disconnect")

                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 16
                    color: disconnectBtn.pressed ? "#c0392b" : "#e74c3c"
                }

                onClicked: Bluetooth.disconnectFromDevice()
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#2d5a2d"
            opacity: 0.15
        }

        Text {
            width: parent.width
            text: tr("Make sure Bluetooth is enabled on your computer and Zowi is turned on.")
            color: "#2d5a2d"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            opacity: 0.6
            lineHeight: 1.4
        }

        Text {
            width: parent.width
            text: tr("If a PIN is requested, enter: 1234")
            color: "#e67e22"
            font.pixelSize: 12
            font.italic: true
            wrapMode: Text.WordWrap
            opacity: 0.8
        }

        Row {
            width: parent.width
            spacing: 15

            Button {
                id: scanButton
                implicitWidth: 160
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

    Rectangle {
        anchors {
            top: topColumn.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: 20
            leftMargin: 30
            rightMargin: 30
        }

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
            // Check for duplicates
            for (var i = 0; i < devicesModel.count; i++) {
                if (devicesModel.get(i).address === address)
                    return;
            }

            // Filter: name contains "Zowi" OR address starts with MAC prefix
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
                // Configure the robot name before finishing setup.
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

        // Last accepted (valid) value, used to revert invalid live input.
        property string lastValid: "OpenZowi"
        // Guards against onTextChanged re-entrancy when we revert programmatically.
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

                // Live filtering: only A-Za-z and <=10 chars. Invalid input is
                // reverted to the last valid value and a warning is shown.
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
                        if (nm === "") nm = "OpenZowi" // empty -> default, no validation
                        if (!namePanel.isValidName(nm)) {
                            warningText.text = tr("Nombre no válido")
                            warningText.visible = true
                            return
                        }
                        Bluetooth.sendData("R " + nm + "\r\n") // configure the robot
                        Bluetooth.setDeviceName(nm)
                        Session.saveActiveZowiName(nm)
                        namePanel.close()
                        scanScreen.deviceSelected(nm, Bluetooth.deviceAddress)
                    }
                }

                Button {
                    implicitWidth: 150
                    height: 44
                    text: tr("Continuar")

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
                        // Proceed accepting the chosen name (no rename command sent).
                        Bluetooth.setDeviceName(nm)
                        Session.saveActiveZowiName(nm)
                        namePanel.close()
                        scanScreen.deviceSelected(nm, Bluetooth.deviceAddress)
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
                // Auto-connect to the previously paired device
                Bluetooth.connectToDevice(storedAddr)
            }
        }
    }
}
