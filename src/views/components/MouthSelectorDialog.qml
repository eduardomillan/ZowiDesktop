// MouthSelectorDialog: modal dialog wrapper for MouthSelectorContent,
// shown from GameTimelineScreen when timeline_selectors_as_dialogs config flag is enabled.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: root

    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    readonly property real sizeRatio: parseFloat(Config.get("timeline_selector_dialog_size_ratio")) || 0.8
    width: (parent ? parent.width : 800) * sizeRatio
    height: (parent ? parent.height : 600) * sizeRatio
    padding: 20

    function tr(source) { return Translator.translate("MouthScreen.qml", source) }

    property int closeButtonWidth: 160
    property int closeButtonHeight: 44

    signal mouthSelected(string name)

    background: Rectangle {
        radius: 20
        color: "#ffffff"
        border.color: Config.get("color_primary") || "#2d5a2d"
        border.width: 2
    }

    contentItem: ColumnLayout {
        spacing: 12

        Text {
            text: root.tr("mouths_title")
            font.bold: true
            font.pixelSize: 18
            color: Config.get("color_primary") || "#2d5a2d"
            Layout.alignment: Qt.AlignHCenter
        }

        Loader {
            active: root.visible
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: MouthSelectorContent {
                onMouthSelected: (name) => {
                    root.mouthSelected(name)
                    root.close()
                }
            }
        }

        Button {
            text: root.tr("close")
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: root.closeButtonWidth
            implicitHeight: root.closeButtonHeight
            onClicked: root.close()
            background: Rectangle {
                color: parent.down ? (Config.get("color_accent_pressed") || "#17736c") : (Config.get("color_accent") || "#21a69b")
                radius: 8
            }
            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.bold: true
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
