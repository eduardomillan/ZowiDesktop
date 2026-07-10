import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    id: splash
    color: "#f4f9f4"

    signal splashFinished()

    function tr(source) { return Translator.translate("SplashScreen.qml", source) }

    Column {
        anchors.centerIn: parent
        spacing: 20

        AnimatedZowi {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 180
            height: 180
            frameInterval: 900
            bounceStrength: 0.07
            swayAmplitude: 4
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("ZOWI")
            color: "#2d5a2d"
            font.pixelSize: 48
            font.bold: true
            font.family: "monospace"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("Desktop")
            color: "#2d5a2d"
            font.pixelSize: 18
            opacity: 0.7
        }
    }

    ProgressBar {
        id: progressBar
        anchors {
            bottom: parent.bottom
            bottomMargin: 60
            horizontalCenter: parent.horizontalCenter
        }
        width: 200
        height: 4
        indeterminate: true
        contentItem: Rectangle {
            radius: 2
            color: "#21a69b"
        }
        background: Rectangle {
            radius: 2
            color: "#000000"
            opacity: 0.2
        }
    }

    Timer {
        interval: 2000
        running: true
        onTriggered: splash.splashFinished()
    }
}
