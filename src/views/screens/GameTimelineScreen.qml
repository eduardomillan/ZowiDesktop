// GameTimelineScreen: "1, 2, 3 ¡Acción!" — sequence editor.
// GUI phase: draft a multi-step routine (movements, animations, mouths), edit
// repetitions/duration per item, reorder by dragging, delete.
// Play/Stop is wired to Robot.connected but the actual playback core
// (MovementSequencer) is connected in a later phase. Mirror of ZowiAppReborn's
// TimelineActivity, see docs/project/screens/SCREEN_GAME_TIMELINE.md.
//
// Layout: horizontal scrollable strip of colored chips (200×180 each), centered
// vertically in the middle of the screen. Fixed add buttons (bottom-left) and
// Play/Stop toggle button (bottom-right). Drag-to-reorder: long-press on chip.
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

    // Metadata for each action: which controls to show (direction, duration)
    readonly property var movementDirectionNames: ["Crusaito"]
    function metaFor(type, name) {
        return {
            supportsDuration: type === "movement",
            supportsDirection: type === "movement" && root.movementDirectionNames.indexOf(name) !== -1
        }
    }

    // Local sequence model (persisted later via Session, key timeline_sequence)
    ListModel {
        id: timelineModel
    }

    function addCommand(type, name) {
        var meta = metaFor(type, name)
        timelineModel.append({
            "name": name,
            "type": type,
            "reps": 1,
            "duration": "Medium",
            "direction": meta.supportsDirection ? "Left" : "Front",
            "supportsDirection": meta.supportsDirection,
            "supportsDuration": meta.supportsDuration
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

    property bool isPlayingTimeline: false

    Item {
        anchors.fill: parent
        anchors.margins: 20

        // ─── Horizontal strip area, vertically centered ───────────────────────
        Item {
            id: stripArea
            anchors { top: parent.top; left: parent.left; right: parent.right; bottom: bottomBar.top }

            ListView {
                id: sequenceList
                anchors.centerIn: parent
                width: parent.width - 40      // 20dp side padding equivalent
                height: 180                   // Android strip height
                orientation: ListView.Horizontal
                visible: timelineModel.count > 0
                clip: true
                spacing: 10
                model: timelineModel

                delegate: Item {
                    id: wrapper
                    width: 200
                    height: 180
                    property bool dragging: false

                    TimelineChip {
                        id: chip
                        anchors.fill: parent
                        commandData: ({
                            name: model.name,
                            type: model.type,
                            reps: model.reps,
                            duration: model.duration,
                            direction: model.direction,
                            supportsDirection: model.supportsDirection,
                            supportsDuration: model.supportsDuration
                        })
                        onDeleteRequested: timelineModel.remove(index)
                        onRepsChanged: timelineModel.setProperty(index, "reps", reps)
                        onDurationChanged: timelineModel.setProperty(index, "duration", duration)
                        onDirectionChanged: timelineModel.setProperty(index, "direction", direction)
                    }

                    MouseArea {
                        anchors.fill: parent
                        drag.target: wrapper.dragging ? wrapper : undefined
                        drag.axis: Drag.XAxis
                        drag.threshold: 4
                        onPressAndHold: wrapper.dragging = true
                        onReleased: {
                            if (!wrapper.dragging) return
                            wrapper.dragging = false
                            var colW = wrapper.width + sequenceList.spacing
                            var newIndex = Math.round((wrapper.x + sequenceList.contentX) / colW)
                            newIndex = Math.max(0, Math.min(timelineModel.count - 1, newIndex))
                            if (newIndex !== index) timelineModel.move(index, newIndex, 1)
                            wrapper.x = 0
                        }
                    }
                }
            }

            // Empty state
            Column {
                anchors.centerIn: parent
                visible: timelineModel.count === 0
                spacing: 6
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.tr("title")
                    font.bold: true
                    font.pixelSize: 18
                    color: Config.get("color_primary") || "#2d5a2d"
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.tr("empty_timeline")
                    font.pixelSize: 14
                    color: "gray"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        // ─── Fixed bottom bar: add buttons (left) + Play/Stop (right) ─────────
        RowLayout {
            id: bottomBar
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            spacing: 10

            // Add buttons, fixed order: Mouth, Animation, Movement
            RowLayout {
                spacing: 10
                Button {
                    text: root.tr("add_mouth")
                    implicitHeight: 44
                    onClicked: root.mouthSelectorRequested()
                }
                Button {
                    text: root.tr("add_animation")
                    implicitHeight: 44
                    onClicked: root.animationSelectorRequested()
                }
                Button {
                    text: root.tr("add_movement")
                    implicitHeight: 44
                    onClicked: root.movementSelectorRequested()
                }
            }

            Item { Layout.fillWidth: true }  // spacer

            // Single circular Play/Stop toggle button
            Button {
                implicitWidth: 56
                implicitHeight: 56
                enabled: root.isPlayingTimeline || (Robot.connected && timelineModel.count > 0)
                contentItem: Item {
                    anchors.centerIn: parent
                    Image {
                        anchors.centerIn: parent
                        source: root.isPlayingTimeline ? "qrc:/images/android/ic_play_arrow_white_24dp.png"
                                                       : "qrc:/images/android/ic_play_arrow_white_24dp.png"
                        width: 24
                        height: 24
                        fillMode: Image.PreserveAspectFit
                        visible: !root.isPlayingTimeline
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        color: "#ffffff"
                        visible: root.isPlayingTimeline
                    }
                }
                background: Rectangle {
                    radius: 28
                    color: parent.down
                           ? (Config.get("color_accent_pressed") || "#17736c")
                           : (parent.enabled ? (Config.get("color_accent") || "#21a69b")
                                             : (Config.get("color_bg_disabled") || "#e6e6e6"))
                }
                onClicked: {
                    if (!root.isPlayingTimeline) {
                        root.isPlayingTimeline = true
                        root.playClicked(timelineModel.count)
                    } else {
                        root.isPlayingTimeline = false
                        root.stopClicked()
                    }
                }
            }
        }
    }
}