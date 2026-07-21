// WizardScreen: Bluetooth pairing wizard entry.
// Presents a prompt to turn on Zowi before pairing.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: wizard
    screenName: "WizardScreen"

    title: tr("welcome")
    subtitle: tr("turn_on_hint")

    signal startClicked()
    signal dismissed()

    property bool _pendingStart: false

    function tr(source) { return Translator.translate("WizardScreen.qml", source) }

    Timer {
        id: usbWarnTimer
        interval: 3000
        onTriggered: {
            if (wizard._pendingStart) {
                wizard._pendingStart = false
                wizard.startClicked()
            }
        }
    }

    footer: MessageBar {
        id: usbWarnBar
        duration: 3000
        textColor: "#2d5a2d"
    }

    Column {
        anchors.centerIn: parent
        spacing: 20

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: Config.get("welcome_image")
            sourceSize.height: 200
            fillMode: Image.PreserveAspectFit
        }

        Item { width: 1; height: 10 }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            Button {
                id: startButton
                implicitWidth: 200
                height: 56
                text: tr("already_on")

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
                    color: startButton.pressed ? "#17736c" : "#21a69b"
                }

                onClicked: {
                    if (Robot.usbAvailable && Robot.bluetoothAvailable) {
                        usbWarnBar.show(tr("usb_recommend_disconnect"), "#f1c40f")
                        wizard._pendingStart = true
                        usbWarnTimer.restart()
                    } else {
                        wizard.startClicked()
                    }
                }
            }

            Button {
                id: notReadyButton
                implicitWidth: 200
                height: 56
                text: tr("no_zowi")

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
                    radius: 28
                    color: "transparent"
                    border.color: "#2d5a2d"
                    border.width: 2
                    opacity: 0.5
                }

                onClicked: wizard.dismissed()
            }
        }
    }
}
