// StartScreen: Entry screen with two options:
// Start the pairing wizard or learn more on the project website.
import QtQuick 6.0
import QtQuick.Controls 6.0
import "../components"

Rectangle {
    id: start
    color: "#f4f9f4"

    signal startWizard()
    signal knowMoreClicked()

    property string screenName: "StartScreen"

    function tr(source) { return Translator.translate("StartScreen.qml", source) }

    Column {
        anchors.centerIn: parent
        spacing: 20

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: Config.get("start_image")
            sourceSize.width: 180
            sourceSize.height: 180
            fillMode: Image.PreserveAspectFit
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("ZOWI")
            color: "#2d5a2d"
            font.pixelSize: 36
            font.bold: true
            font.family: "monospace"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("Your friendly robot companion")
            color: "#2d5a2d"
            font.pixelSize: 16
            opacity: 0.8
        }

        Item { width: 1; height: 10 }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            Button {
                id: startButton
                implicitWidth: 200
                height: 56
                text: tr("Empezar")

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 18
                    font.bold: true
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 28
                    color: startButton.pressed ? "#17736c" : "#21a69b"
                }

                onClicked: start.startWizard()
            }

            Button {
                id: knowMoreButton
                implicitWidth: 200
                height: 56
                text: tr("Saber más")

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

                onClicked: start.knowMoreClicked()
            }
        }
    }
}
