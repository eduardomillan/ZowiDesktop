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
    subtitle: tr("subtitle")
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
    readonly property int addButtonSize: 80
    property real buttonSpacingRatio: 0.03  // % of window width; adjust for testing

    // TimelineChip configurable sizes
    property int chipWidth: 200
    property int chipHeight: 180
    property int chipIconSize: 72
    property int chipDeleteButtonSize: 40

    // Use dialogs or full-screen selectors for timeline item selection
    readonly property bool useDialogSelectors: Config.get("timeline_selectors_as_dialogs") === "true"

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
                    width: root.chipWidth
                    height: root.chipHeight
                    property bool dragging: false

                    TimelineChip {
                        id: chip
                        anchors.fill: parent
                        chipWidth: root.chipWidth
                        chipHeight: root.chipHeight
                        chipIconSize: root.chipIconSize
                        chipDeleteButtonSize: root.chipDeleteButtonSize
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
                spacing: root.width * root.buttonSpacingRatio
                Image {
                    id: addMouthBtn
                    width: root.addButtonSize
                    height: root.addButtonSize
                    source: "qrc:/images/android/smile_button.png"
                    sourceSize.width: root.addButtonSize
                    sourceSize.height: root.addButtonSize
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        id: addMouthArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onPressed: addMouthBtn.source = "qrc:/images/android/pressed_smile_button.png"
                        onReleased: {
                            addMouthBtn.source = "qrc:/images/android/smile_button.png"
                            if (root.useDialogSelectors) mouthDialog.open()
                            else root.mouthSelectorRequested()
                        }
                    }
                    ToolTip.visible: addMouthArea.containsMouse
                    ToolTip.text: root.tr("add_mouth")
                }
                Image {
                    id: addAnimationBtn
                    width: root.addButtonSize
                    height: root.addButtonSize
                    source: "qrc:/images/android/animation_happy_button.png"
                    sourceSize.width: root.addButtonSize
                    sourceSize.height: root.addButtonSize
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        id: addAnimationArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onPressed: addAnimationBtn.source = "qrc:/images/android/pressed_animation_happy_button.png"
                        onReleased: {
                            addAnimationBtn.source = "qrc:/images/android/animation_happy_button.png"
                            if (root.useDialogSelectors) gestureDialog.open()
                            else root.animationSelectorRequested()
                        }
                    }
                    ToolTip.visible: addAnimationArea.containsMouse
                    ToolTip.text: root.tr("add_animation")
                }
                Image {
                    id: addMovementBtn
                    width: root.addButtonSize
                    height: root.addButtonSize
                    source: "qrc:/images/android/choreography_button.png"
                    sourceSize.width: root.addButtonSize
                    sourceSize.height: root.addButtonSize
                    fillMode: Image.PreserveAspectFit

                    MouseArea {
                        id: addMovementArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onPressed: addMovementBtn.source = "qrc:/images/android/pressed_choreography_button.png"
                        onReleased: {
                            addMovementBtn.source = "qrc:/images/android/choreography_button.png"
                            if (root.useDialogSelectors) movementDialog.open()
                            else root.movementSelectorRequested()
                        }
                    }
                    ToolTip.visible: addMovementArea.containsMouse
                    ToolTip.text: root.tr("add_movement")
                }
            }

            Item { Layout.fillWidth: true }  // spacer

            // Single circular Play/Stop toggle button
            Button {
                implicitWidth: root.addButtonSize
                implicitHeight: root.addButtonSize
                enabled: root.isPlayingTimeline || (Robot.connected && timelineModel.count > 0)
                contentItem: Item {
                    anchors.centerIn: parent
                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/images/android/ic_play_arrow_white_24dp.png"
                        width: root.addButtonSize * 0.43
                        height: root.addButtonSize * 0.43
                        fillMode: Image.PreserveAspectFit
                        visible: !root.isPlayingTimeline
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: root.addButtonSize * 0.29
                        height: root.addButtonSize * 0.29
                        color: "#ffffff"
                        visible: root.isPlayingTimeline
                    }
                }
                background: Rectangle {
                    radius: root.addButtonSize / 2
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

    // Dialogs for timeline item selection (shown when useDialogSelectors flag is true)
    MouthSelectorDialog {
        id: mouthDialog
        onMouthSelected: (name) => root.addCommand("mouth", name)
    }

    GestureSelectorDialog {
        id: gestureDialog
        onGestureSelected: (name) => root.addCommand("animation", name)
    }

    MovementSelectorDialog {
        id: movementDialog
        onMovementSelected: (name, type) => root.addCommand(type, name)
    }
}