// WizardFoundScreen: shown after the user selects a Zowi device.
// Displays an image, pairing instructions, and a single pair button.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: wizardFound
    screenName: "WizardFoundScreen"

    title: tr("¡Aquí está!")
    subtitle: tr("Para controlar a Zowi, introduce el código 1234")

    property string pairingCode: Config.get("pairing_code") || "1234"
    property bool pairingAttempt: false

    signal paired()

    function tr(source) { return Translator.translate("WizardFoundScreen.qml", source) }

    Timer {
        id: pairingTimer
        interval: 10000
        onTriggered: {
            if (wizardFound.pairingAttempt) {
                wizardFound.pairingAttempt = false
                errorText.visible = true
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 25

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: Config.get("zowi_found_image")
            sourceSize.height: 200
            fillMode: Image.PreserveAspectFit
        }

        Item { width: 1; height: 10 }

        Button {
            id: pairButton
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: 260
            height: 56
            text: tr("Emparejar con %1").arg(wizardFound.pairingCode)

            contentItem: Text {
                text: parent.text
                font.pixelSize: 16
                font.bold: true
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 28
                color: pairButton.pressed ? "#17736c" : "#21a69b"
            }

            onClicked: {
                if (Bluetooth.connected) {
                    wizardFound.paired()
                } else {
                    var addr = Session.loadActiveZowiDeviceAddress()
                    if (addr) {
                        wizardFound.pairingAttempt = true
                        errorText.visible = false
                        pairingTimer.restart()
                        Bluetooth.connectToDevice(addr)
                    }
                }
            }
        }

        Text {
            id: errorText
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("No se pudo conectar. Asegúrate de que el Zowi está encendido y cerca.")
            color: "#e74c3c"
            font.pixelSize: 13
            visible: false
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: pairButton.implicitWidth
        }
    }

    Timer {
        id: connectTimer
        interval: 800
        onTriggered: {
            var addr = Session.loadActiveZowiDeviceAddress()
            if (addr !== "") {
                wizardFound.pairingAttempt = true
                errorText.visible = false
                pairingTimer.restart()
                Bluetooth.connectToDevice(addr)
            }
        }
    }

    Component.onCompleted: connectTimer.start()

    Connections {
        target: Bluetooth

        function onConnectionChanged() {
            if (Bluetooth.connected) {
                pairingTimer.stop()
                wizardFound.pairingAttempt = false
                wizardFound.paired()
            } else if (wizardFound.pairingAttempt) {
                pairingTimer.stop()
                wizardFound.pairingAttempt = false
                errorText.visible = true
            }
        }

        function onErrorOccurred(message) {
            if (wizardFound.pairingAttempt) {
                pairingTimer.stop()
                wizardFound.pairingAttempt = false
                errorText.visible = true
            }
        }
    }
}
