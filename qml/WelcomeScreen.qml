import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: welcome
    color: "#702076"

    property bool skipToDemo: false

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

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 140
            height: 140

            Rectangle {
                anchors.fill: parent
                radius: 70
                clip: true
                color: "#21a69b"

                Image {
                    anchors.centerIn: parent
                    width: 140
                    height: 140
                    source: "qrc:/images/zowi_hello.png"
                    fillMode: Image.PreserveAspectCrop
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "ZOWI"
            color: "#ffffff"
            font.pixelSize: 36
            font.bold: true
            font.family: "monospace"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Zowi says hi!"
            color: "#ffffff"
            font.pixelSize: 16
            opacity: 0.8
        }

        Item { width: 1; height: 10 }

        Button {
            id: startButton
            anchors.horizontalCenter: parent.horizontalCenter
            width: 280
            height: 56
            text: "START"

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
            anchors.horizontalCenter: parent.horizontalCenter
            width: 280
            height: 56
            text: "I HAVEN'T GOT A ZOWI"

            contentItem: Text {
                text: parent.text
                font.pixelSize: 14
                font.bold: true
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                opacity: 0.8
            }

            background: Rectangle {
                radius: 28
                color: "transparent"
                border.color: "#ffffff"
                border.width: 2
                opacity: 0.5
            }

            onClicked: welcome.enterDemoMode()
        }
    }

    Text {
        anchors {
            bottom: parent.bottom
            bottomMargin: 40
            horizontalCenter: parent.horizontalCenter
        }
        text: "Letter to parents"
        color: "#ffffff"
        font.pixelSize: 14
        opacity: 0.6

        MouseArea {
            anchors.fill: parent
            onClicked: letterDialog.open()
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
                text: "Letter to parents"
                font.pixelSize: 22
                font.bold: true
                color: "#702076"
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                width: parent.width
                text: "Dear parents,"
                font.pixelSize: 14
                font.bold: true
                wrapMode: Text.WordWrap
            }

            Text {
                width: parent.width
                text: "Zowi is a friendly robot that helps children learn about technology and programming in a fun way."
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                lineHeight: 1.5
            }

            Text {
                width: parent.width
                text: "The Zowi Team"
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignRight
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Accept"

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
