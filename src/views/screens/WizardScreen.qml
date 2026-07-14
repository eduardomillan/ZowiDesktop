// WizardScreen: Bluetooth pairing wizard entry.
// Presents a prompt to turn on Zowi before pairing.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: wizard
    screenName: "WizardScreen"

    title: tr("Welcome!")
    subtitle: tr("To control Zowi, first turn it on.")

    signal startClicked()
    signal dismissed()

    function tr(source) { return Translator.translate("WizardScreen.qml", source) }

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
                text: tr("It's already on")

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

                onClicked: wizard.startClicked()
            }

            Button {
                id: notReadyButton
                implicitWidth: 200
                height: 56
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
