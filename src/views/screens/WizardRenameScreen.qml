// WizardRenameScreen: shown right after a successful pairing.
// Lets the user give the Zowi a friendly name. The default name comes from
// config.json ("zowi_default_name"). Sends the rename command over Bluetooth
// ("R <name>\r") and waits for the firmware's final ack (&&F) before leaving.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: renameScreen
    screenName: "WizardRenameScreen"

    title: tr("¡Dale un nombre!")
    subtitle: tr("Escribe un nombre para tu Zowi y pulsa Renombrar.")
    showBackButton: !renaming

    property string defaultName: Config.get("zowi_default_name") || "OpenZowi"
    property bool renaming: false
    property bool renameDone: false

    signal renamed(string name)

    function tr(source) { return Translator.translate("WizardRenameScreen.qml", source) }

    Column {
        anchors.centerIn: parent
        spacing: 25

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: Config.get("zowi_found_image")
            sourceSize.height: 160
            fillMode: Image.PreserveAspectFit
        }

        TextField {
            id: nameField
            anchors.horizontalCenter: parent.horizontalCenter
            width: 340
            height: 48
            text: renameScreen.defaultName
            placeholderText: tr("Nombre del Zowi")
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
            selectByMouse: true
            enabled: !renameScreen.renaming
            onAccepted: renameButton.clicked()
        }

        Button {
            id: renameButton
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: 260
            height: 56
            text: renameScreen.renaming ? tr("Renombrando...") : tr("Renombrar")
            enabled: !renameScreen.renaming && nameField.text.trim() !== ""

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
                color: renameButton.pressed ? "#17736c" : "#21a69b"
            }

            onClicked: {
                var name = nameField.text.trim()
                if (name === "")
                    return
                renameScreen.renaming = true
                statusText.text = tr("Enviando nombre a Zowi...")
                statusText.color = "#2d5a2d"
                statusText.visible = true
                renameTimer.restart()
                // Firmware command: "R <name>\r"
                Bluetooth.sendData("R " + name + "\r")
            }
        }

        Text {
            id: statusText
            anchors.horizontalCenter: parent.horizontalCenter
            text: ""
            font.pixelSize: 14
            visible: false
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: 340
        }
    }

    Timer {
        id: renameTimer
        interval: 8000
        onTriggered: {
            if (!renameScreen.renameDone) {
                renameScreen.renaming = false
                statusText.text = tr("No se pudo renombrar. Inténtalo de nuevo.")
                statusText.color = "#e74c3c"
                statusText.visible = true
            }
        }
    }

    Component.onCompleted: {
        nameField.selectAll()
        nameField.forceActiveFocus()
    }

    Connections {
        target: Bluetooth

        function onDataReceived(data) {
            if (renameScreen.renaming && !renameScreen.renameDone
                    && data.indexOf("&&F") !== -1) {
                renameScreen.renameDone = true
                renameTimer.stop()
                renameScreen.renaming = false
                statusText.text = tr("¡Listo! Zowi ahora se llama %1.").arg(nameField.text.trim())
                statusText.color = "#2d5a2d"
                statusText.visible = true
                renameScreen.renamed(nameField.text.trim())
            }
        }
    }
}
