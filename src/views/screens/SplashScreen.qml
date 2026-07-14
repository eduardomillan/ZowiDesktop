// SplashScreen: Initial screen shown on app launch.
// Displays the Zowi logo and provides Continue/Quit buttons
// along with language selector flags (ES, CA, EN).
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

Rectangle {
    id: splash
    color: "#f4f9f4"

    signal splashFinished()
    signal quitRequested()

    property string screenName: "SplashScreen"

    function tr(source) { return Translator.translate("SplashScreen.qml", source) }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -60
        spacing: 20

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: Config.get("splash_image")
            sourceSize.width: 180
            sourceSize.height: 180
            fillMode: Image.PreserveAspectFit
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("zowi")
            color: "#2d5a2d"
            font.pixelSize: 48
            font.bold: true
            font.family: "monospace"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("desktop")
            color: "#2d5a2d"
            font.pixelSize: 18
            opacity: 0.7
        }
    }

    Row {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 150
        spacing: 20

        Button {
            id: continueButton
            implicitWidth: 200
            height: 50
            text: tr("continue")

            contentItem: Text {
                text: parent.text
                font.pixelSize: 18
                font.bold: true
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 25
                color: continueButton.pressed ? "#17736c" : "#21a69b"
            }

            onClicked: splash.splashFinished()
        }

        Button {
            id: quitButton
            visible: Config.get("button_quit_visible") === "true"
            implicitWidth: 200
            height: 50
            text: tr("quit")

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
                radius: 25
                color: "transparent"
                border.color: "#2d5a2d"
                border.width: 2
                opacity: 0.5
            }

            onClicked: splash.quitRequested()
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: langRow.top
        anchors.bottomMargin: 10
        text: tr("select_language")
        color: "#2d5a2d"
        font.pixelSize: 13
        opacity: 0.6
    }

    Row {
        id: langRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 52

        ComboBox {
            id: langCombo
            width: 160
            height: 36

            model: ListModel {
                ListElement { text: "Español"; locale: "es_ES" }
                ListElement { text: "Valencià"; locale: "ca_ES" }
                ListElement { text: "English"; locale: "en_US" }
            }
            textRole: "text"

            font.pixelSize: 14
            font.family: "monospace"

            Component.onCompleted: {
                var cur = Translator.currentLocale()
                for (var i = 0; i < model.count; ++i) {
                    if (model.get(i).locale === cur) {
                        currentIndex = i
                        return
                    }
                }
                currentIndex = 2 // fallback: English
            }

            onActivated: {
                var loc = model.get(currentIndex).locale
                if (Translator.currentLocale() !== loc)
                    Translator.load(loc)
            }

            contentItem: Text {
                text: langCombo.displayText
                color: "#2d5a2d"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font: langCombo.font
            }

            background: Rectangle {
                radius: 18
                border.color: "#2d5a2d"
                border.width: 1
                opacity: 0.5
                color: "transparent"
                implicitWidth: 160
                implicitHeight: 36
            }

            delegate: ItemDelegate {
                width: langCombo.width
                height: 36

                contentItem: Text {
                    text: model.text
                    color: "#2d5a2d"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    font: langCombo.font
                }

                background: Rectangle {
                    color: langCombo.highlightedIndex === index ? "#e0f0e0" : "transparent"
                }
            }

            popup: Popup {
                y: langCombo.height + 2
                width: langCombo.width
                padding: 0

                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: langCombo.delegateModel
                    currentIndex: langCombo.highlightedIndex

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                }

                background: Rectangle {
                    color: "#ffffff"
                    radius: 8
                    border.color: "#2d5a2d"
                    border.width: 1
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: Config.devMode

        MessageBar {
            id: msgBar
            duration: parseInt(Config.get("message_duration")) || 2000
        }

        Button {
            id: resetButton
            anchors {
                left: parent.left
                bottom: msgBar.top
                margins: 12
            }
            implicitWidth: 90
            height: 32
            text: tr("reset")

            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.pixelSize: 12
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 16
                color: resetButton.pressed ? "#d35400" : "#e67e22"
            }

            onClicked: {
                var addr = Session.loadActiveZowiDeviceAddress()
                if (!addr) addr = Bluetooth.deviceAddress
                if (!addr) {
                    // No registered Zowi: clear any stale app data and warn.
                    Session.saveActiveZowiDeviceAddress("")
                    Session.saveActiveZowiName("")
                    Session.saveWizardDismissed(false)
                    msgBar.show(tr("reset_no_zowi"), "#c0392b")
                    return
                }
                if (Bluetooth.connected) Bluetooth.disconnectFromDevice()
                Bluetooth.unpairDevice(addr)
                Session.saveActiveZowiDeviceAddress("")
                Session.saveActiveZowiName("")
                Session.saveWizardDismissed(false)
            }
        }
    }

    Connections {
        target: Bluetooth
        function onUnpairFinished(success, message) {
            if (success)
                msgBar.show(tr("unpair_success"))
            else
                msgBar.show(tr("unpair_app_only"))
        }
    }
}
