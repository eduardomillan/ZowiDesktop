// GameTimelineScreen: "1, 2, 3 ¡Acción!" — sequence editor.
// GUI phase: draft a multi-step routine (movements, animations, mouths), edit
// repetitions/duration per item, reorder by dragging the left handle, delete.
// Play/Stop is wired to Robot.connected but the actual playback core
// (MovementSequencer) is connected in a later phase. Mirror of ZowiAppReborn's
// TimelineActivity, see docs/project/screens/SCREEN_GAME_TIMELINE.md.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "GameTimelineScreen"
    title: tr("title")
    showBackButton: true

    function tr(source) { return Translator.translate("GameTimelineScreen.qml", source) }

    // Local sequence model (persisted later via Session, key timeline_sequence)
    ListModel {
        id: timelineModel
    }

    function addCommand(type, name) {
        timelineModel.append({
            "name": name,
            "type": type,
            "reps": 1,
            "duration": "Medium",
            "direction": "Forward"
        })
        console.log("[Timeline] added " + type + ": " + name + " (count=" + timelineModel.count + ")")
    }

    signal movementSelectorRequested()
    signal animationSelectorRequested()
    signal mouthSelectorRequested()
    signal playClicked(int count)
    signal stopClicked()

    function onMovementSelected(name, type) {
        addCommand(type, name)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // ─── Play / Stop control bar ─────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                text: root.tr("play_button")
                enabled: Robot.connected && timelineModel.count > 0
                Layout.fillWidth: true
                implicitHeight: 44
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 22
                    color: parent.enabled
                           ? (parent.pressed ? (Config.get("color_accent_pressed") || "#17736c")
                                             : (Config.get("color_accent") || "#21a69b"))
                           : (Config.get("color_bg_disabled") || "#e6e6e6")
                }
                onClicked: root.playClicked(timelineModel.count)
            }

            Button {
                text: root.tr("stop_button")
                implicitHeight: 44
                Layout.fillWidth: true
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 22
                    color: parent.pressed ? (Config.get("color_accent_pressed") || "#17736c")
                                         : (Config.get("color_accent") || "#21a69b")
                }
                onClicked: root.stopClicked()
            }
        }

        // ─── Sequence list ───────────────────────────────────────────────────
        ListView {
            id: sequenceList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: timelineModel

            delegate: Item {
                id: wrapper
                width: sequenceList.width
                height: 72

                TimelineItem {
                    id: row
                    anchors {
                        left: parent.left
                        leftMargin: 30   // leave room for the drag handle
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                    }
                    commandData: ({
                        name: model.name,
                        type: model.type,
                        reps: model.reps,
                        duration: model.duration,
                        direction: model.direction
                    })

                    onDeleteRequested: timelineModel.remove(index)
                    onRepetitionsChanged: timelineModel.setProperty(index, "reps", repetitions)
                    onDurationChanged: timelineModel.setProperty(index, "duration", duration)
                }

                // Drag handle (left). Dragging it vertically reorders the item.
                Rectangle {
                    id: handle
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    width: 24
                    radius: 6
                    color: handleMouse.containsMouse
                           ? (Config.get("color_bg_hover") || "#e0f0e0")
                           : (Config.get("color_bg_disabled") || "#e6e6e6")
                    border.color: Config.get("color_accent") || "#21a69b"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "☰"
                        font.pixelSize: 14
                        color: Config.get("color_primary") || "#2d5a2d"
                    }

                    MouseArea {
                        id: handleMouse
                        anchors.fill: parent
                        cursorShape: Qt.SizeVerCursor
                        drag.target: wrapper
                        drag.axis: Drag.YAxis
                        drag.threshold: 4

                        onReleased: {
                            // Reorder based on the drop Y position, then snap back.
                            var rowH = wrapper.height + sequenceList.spacing
                            var newIndex = Math.round((wrapper.y + sequenceList.contentY) / rowH)
                            newIndex = Math.max(0, Math.min(timelineModel.count - 1, newIndex))
                            if (newIndex !== index && newIndex >= 0 && newIndex < timelineModel.count)
                                timelineModel.move(index, newIndex, 1)
                            wrapper.y = 0
                        }
                    }
                }
            }

            // Empty state
            footer: Item {
                visible: timelineModel.count === 0
                width: parent.width
                height: 80

                Text {
                    anchors.centerIn: parent
                    text: root.tr("empty_timeline")
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 14
                    color: "gray"
                    wrapMode: Text.WordWrap
                }
            }
        }

        // ─── Add command bar ─────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            spacing: 10

            Button {
                text: root.tr("add_movement")
                Layout.fillWidth: true
                implicitHeight: 44
                onClicked: root.movementSelectorRequested()
            }

            Button {
                text: root.tr("add_animation")
                Layout.fillWidth: true
                implicitHeight: 44
                onClicked: root.animationSelectorRequested()
            }

            Button {
                text: root.tr("add_mouth")
                Layout.fillWidth: true
                implicitHeight: 44
                onClicked: root.mouthSelectorRequested()
            }
        }
    }
}