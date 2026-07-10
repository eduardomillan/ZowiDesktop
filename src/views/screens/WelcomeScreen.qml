import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    id: welcome
    color: "#f4f9f4"

    property bool skipToDemo: false

    function tr(source) { return Translator.translate("WelcomeScreen.qml", source) }

    Component.onCompleted: {
        if (skipToDemo) {
            enterDemoMode()
        }
    }

    signal enterDemoMode()
    signal startWizard()

    Column {
        anchors.centerIn: parent
        spacing: 30

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/images/scratch/sprite2.png"
            sourceSize.height: 200
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
            text: tr("Zowi says hi!")
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
                implicitWidth: 180
                height: 56
                text: tr("START")

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

                onClicked: welcome.startWizard()
            }

            Button {
                id: demoButton
                implicitWidth: 180
                height: 56
                text: tr("I HAVEN'T GOT A ZOWI")

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

                onClicked: welcome.enterDemoMode()
            }

            Button {
                id: letterButton
                implicitWidth: 180
                height: 56
                text: tr("Letter to parents")

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

                onClicked: letterDialog.open()
            }
        }
    }

    Popup {
        id: letterDialog
        anchors.centerIn: parent
        width: parent.width * 0.6
        height: parent.height * 0.5
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Column {
            anchors.fill: parent
            anchors.margins: 30
            spacing: 20

            Text {
                width: parent.width
                text: tr("Letter to parents")
                font.pixelSize: 22
                font.bold: true
                color: "#2d5a2d"
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                width: parent.width
                text: tr("Dear parents,")
                font.pixelSize: 14
                font.bold: true
                wrapMode: Text.WordWrap
                color: "#2d5a2d"
            }

            Text {
                width: parent.width
                text: tr("Zowi is a friendly robot that helps children learn about technology and programming in a fun way.")
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                lineHeight: 1.5
                color: "#2d5a2d"
            }

            Text {
                width: parent.width
                text: tr("The Zowi Team")
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignRight
                color: "#2d5a2d"
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
                    implicitWidth: 120
                    implicitHeight: 40
                }

                onClicked: letterDialog.close()
            }
        }
    }
}
