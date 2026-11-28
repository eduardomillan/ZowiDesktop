// AchievementUnlockDialog: the "You have unlocked: ..." notice shown when an
// achievement is earned (mirrors the Android MakerBoxDialogAchievement). Several
// unlocks at once are shown one after another. main.qml owns the single instance.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

Dialog {
    id: root
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var queue: []          // ids waiting to be shown
    property string currentId: ""

    // Created once with main.qml and not reloaded on a language change, so its
    // texts re-evaluate through `_rev`.
    property int _rev: 0
    function tr(source) { var r = _rev; return Translator.translate("main.qml", source) }
    Connections {
        target: Translator
        function onLanguageChanged() { root._rev++ }
    }

    AchievementTexts { id: texts }

    // Queue an achievement; opens the dialog when idle.
    function show(id) {
        queue.push(id)
        if (!visible) next()
    }
    function next() {
        if (queue.length === 0) return
        currentId = queue.shift()
        open()
    }
    onClosed: next()

    width: Math.round(Math.min(parent ? parent.width * 0.9 : 400, 400))
    padding: 22
    height: content.implicitHeight + 2 * padding

    background: Rectangle {
        radius: 20
        color: "#ffffff"
        border.color: Config.get("color_accent") || "#21a69b"
        border.width: 2
    }

    contentItem: Column {
        id: content
        spacing: 12
        width: parent ? parent.width : 0

        Text {
            width: parent.width
            text: root.tr("achievement_unlocked_title")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 22
            font.bold: true
            color: Config.get("color_primary") || "#2d5a2d"
        }
        Text {
            width: parent.width
            text: root.tr("achievement_unlocked_text")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: Config.get("color_primary") || "#2d5a2d"
            opacity: 0.8
        }
        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            // Empty until there is an achievement to show (the dialog is created
            // at startup with none), so no "_badge.png" is requested.
            source: root.currentId !== "" ? texts.badge(root.currentId, true) : ""
            sourceSize.width: 168
            sourceSize.height: 168
            width: 110
            height: 110
            fillMode: Image.PreserveAspectFit
        }
        Text {
            width: parent.width
            text: texts.title(root.currentId)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: 20
            font.bold: true
            color: Config.get("color_accent") || "#21a69b"
        }
        Text {
            width: parent.width
            text: texts.description(root.currentId)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: 14
            color: Config.get("color_primary") || "#2d5a2d"
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 200
            height: 48
            text: root.tr("achievement_continue")
            onClicked: root.close()
            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.pixelSize: 16
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: height / 2
                color: Config.get("color_accent") || "#21a69b"
            }
        }
    }
}
