// TimelineItem: one row of the GameTimelineScreen sequence editor.
// Shows the command (movement / animation / mouth), its name and the editable
// properties: repetitions (always), duration (movements only). Emits signals so
// the owning screen can update its model. Deleting and drag-reordering are the
// owner's responsibility (deleteRequested is fired here, reordering is handled
// by the drag handle in GameTimelineScreen).
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    property var commandData: ({}) // { name, type, reps, duration, direction }
    property string screenContext: "GameTimelineScreen.qml"
    property bool playing: false

    signal deleteRequested()
    signal repetitionsChanged(int repetitions)
    signal durationChanged(string duration)

    function tr(source) {
        return Translator.translate(root.screenContext, source)
    }

    implicitWidth: 600
    implicitHeight: 72
    radius: 10
    color: root.playing
            ? (Config.get("color_accent") || "#21a69b")
            : (Config.get("color_bg_connected") || "#e8f5e8")
    border.color: Config.get("color_accent") || "#21a69b"
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        // Command category icon (movement / animation / mouth)
        Image {
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            source: root.commandData.type === "movement" ? "qrc:/images/android/pad_walk_forward.png"
                  : root.commandData.type === "animation" ? "qrc:/images/android/animation_happy_button.png"
                  : "qrc:/images/android/smile_button.png"
            sourceSize: Qt.size(36, 36)
            fillMode: Image.PreserveAspectFit
        }

        // Command name
        Text {
            text: root.commandData.name || "Unknown Command"
            font.pixelSize: 15
            font.bold: true
            color: root.playing ? "#ffffff" : (Config.get("color_primary") || "#2d5a2d")
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        // Repetitions
        RowLayout {
            spacing: 5
            Text {
                text: root.tr("repetitions")
                font.pixelSize: 12
                color: root.playing ? "#ffffff" : "gray"
            }
            SpinBox {
                id: repsSpin
                from: 1
                to: 10
                value: root.commandData.reps || 1
                width: 55
                onValueChanged: {
                    if (activeFocus || value !== (root.commandData.reps || 1))
                        root.repetitionsChanged(value)
                }
            }
        }

        // Duration (movements only)
        RowLayout {
            visible: root.commandData.type === "movement"
            spacing: 5
            Text {
                text: root.tr("duration")
                font.pixelSize: 12
                color: root.playing ? "#ffffff" : "gray"
            }
            ComboBox {
                id: durationCombo
                model: ["Slow", "Medium", "Fast"]
                currentIndex: root.commandData.duration === "Slow" ? 0 : (root.commandData.duration === "Fast" ? 2 : 1)
                width: 110
                onCurrentIndexChanged: {
                    var val = model[currentIndex]
                    if (val !== root.commandData.duration)
                        root.durationChanged(val)
                }
            }
        }

        // Delete button
        Button {
            text: "✕"
            implicitWidth: 30
            implicitHeight: 30
            flat: true
            onClicked: root.deleteRequested()
        }
    }
}