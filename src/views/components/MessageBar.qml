import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    visible: false
    opacity: 0

    anchors {
        left: parent.left
        right: parent.right
        bottom: parent.bottom
    }
    height: 40
    color: "#17736c"

    property alias message: msgText.text
    property color textColor: "#f1c40f"
    property int duration: 2000

    signal dismissed()

    Text {
        id: msgText
        anchors.centerIn: parent
        color: root.textColor
        font.pixelSize: 13
        font.bold: true
    }

    function show(text, bg) {
        msgText.text = text || ""
        if (msgText.text === "") return
        root.color = (bg !== undefined) ? bg : "#17736c"
        root.visible = true
        root.opacity = 1
        hideTimer.restart()
    }

    Timer {
        id: hideTimer
        interval: root.duration
        onTriggered: fadeOut.start()
    }

    NumberAnimation {
        id: fadeOut
        target: root
        property: "opacity"
        to: 0
        duration: 300
        onFinished: {
            root.visible = false
            msgText.text = ""
            root.dismissed()
        }
    }
}
