// ScreenTemplate: shared layout providing StatusBar + centered title/subtitle
// header + optional corner navigation buttons + a flexible content area.
// Screens set title/subtitle and place their content as children.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

Rectangle {
    id: root
    color: "#f4f9f4"

    property alias title: titleText.text
    property alias subtitle: subtitleText.text
    property string screenName: ""
    property bool showBackButton: false
    property bool showDisconnectButton: false

    signal backClicked()
    signal disconnectClicked()

    default property alias content: contentArea.data

    property alias footer: footerArea.data

    function tr(source) { return Translator.translate("ScreenTemplate.qml", source) }

    StatusBar {
        id: statusBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 5
        }
    }

    Button {
        id: backBtn
        visible: root.showBackButton
        width: 40
        height: 40
        anchors {
            top: statusBar.bottom
            left: parent.left
            margins: 12
        }

        contentItem: Text {
            text: "\u2190"
            font.pixelSize: 22
            color: "#2d5a2d"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 20
            color: backBtn.pressed ? "#e0f0e0" : "transparent"
            border.color: "#2d5a2d"
            border.width: 1
            opacity: 0.4
        }

        onClicked: root.backClicked()
    }

    Button {
        id: disconnectBtn
        visible: root.showDisconnectButton && Robot.connected
        implicitWidth: 100
        height: 32
        anchors {
            top: statusBar.bottom
            right: parent.right
            margins: 12
        }

        text: root.tr("disconnect")

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

        onClicked: root.disconnectClicked()
    }

    Column {
        id: header
        anchors {
            top: statusBar.bottom
            horizontalCenter: parent.horizontalCenter
        }
        topPadding: 30
        spacing: 10

        Text {
            id: titleText
            color: "#2d5a2d"
            font.pixelSize: 36
            font.bold: true
            font.family: "monospace"
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            id: subtitleText
            color: "#2d5a2d"
            font.pixelSize: 16
            opacity: 0.8
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }

    Item {
        id: contentArea
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: footerArea.top
            margins: 30
        }
        clip: true
    }

    Item {
        id: footerArea
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 0
    }
}
