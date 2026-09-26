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
    property int cornerButtonSize: 88  // Configurable corner button size
    property real helpDialogWidthRatio: 0.5  // Help dialog width as % of window width

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

    function serializeModel() {
        var items = []
        for (var i = 0; i < timelineModel.count; i++) {
            items.push(timelineModel.get(i))
        }
        return items
    }

    function loadSequence() {
        var items = Timeline.loadSequence()
        timelineModel.clear()
        for (var i = 0; i < items.length; i++) {
            timelineModel.append(items[i])
        }
        if (items.length > 0) {
            console.log("[Timeline] Loaded " + items.length + " commands from persistence")
        }
    }

    function saveSequence() {
        Timeline.saveSequence(serializeModel())
    }

    function clearTimeline() {
        timelineModel.clear()
        console.log("[Timeline] Timeline cleared")
    }

    Component.onCompleted: {
        root.playClicked.connect(onPlayClicked)
        root.stopClicked.connect(onStopClicked)
    }

    function onPlayClicked(count) {
        Timeline.play(serializeModel())
    }

    function onStopClicked() {
        Timeline.stop()
    }

    property bool isPlayingTimeline: Timeline.isPlaying
    readonly property int addButtonSize: 80
    property int playClearButtonSize: 80
    property real buttonSpacingRatio: 0.03  // % of window width; adjust for testing

    // TimelineChip sizes: responsive to available space (strip area, not full screen)
    property real chipWidthRatio: 0.15            // % of screen width
    property real chipHeightRatio: 0.60           // % of strip area height (stripArea, not full screen)
    property real chipIconSizeRatio: 0.50         // % of chipWidth
    property real chipDeleteButtonSizeRatio: 0.35 // % of chipWidth
    property real chipButtonWidthRatio: 0.30      // % of chipWidth (reps/duration/direction buttons)
    property real chipButtonHeightRatio: 0.28     // % of chipWidth (reps/duration/direction buttons)

    property int chipWidth: Math.max(120, Math.round(root.width * chipWidthRatio))
    property int chipHeight: Math.max(110, Math.round(stripArea.height * chipHeightRatio))
    property int chipIconSize: Math.round(chipWidth * chipIconSizeRatio)
    property int chipDeleteButtonSize: Math.round(chipWidth * chipDeleteButtonSizeRatio)
    property int chipButtonWidth: Math.round(chipWidth * chipButtonWidthRatio)
    property int chipButtonHeight: Math.round(chipWidth * chipButtonHeightRatio)

    // Use dialogs or full-screen selectors for timeline item selection
    readonly property bool useDialogSelectors: Config.get("timeline_selectors_as_dialogs") === "true"

    // ─── Corner buttons: Ranking + Help (top-right) ─────────────────────────
    corner: Row {
        spacing: -10

        Button {
            id: rankingBtn
            width: root.cornerButtonSize
            height: root.cornerButtonSize
            enabled: false

            contentItem: Image {
                source: "qrc:/images/android/ranking_button.png"
                sourceSize.width: 56
                sourceSize.height: 56
                fillMode: Image.PreserveAspectFit
            }

            background: Rectangle {
                radius: 44
                color: rankingBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
            }
        }

        Button {
            id: helpBtn
            width: root.cornerButtonSize
            height: root.cornerButtonSize

            contentItem: Image {
                source: "qrc:/images/android/how_to_play_button.png"
                sourceSize.width: 56
                sourceSize.height: 56
                fillMode: Image.PreserveAspectFit
            }

            background: Rectangle {
                radius: 44
                color: helpBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
            }

            onClicked: helpDialog.open()
        }
    }

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
                height: root.chipHeight       // Dynamic: follows chip size
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
                    property int itemIndex: index
                    transform: Translate { id: dragTranslate; x: 0 }

                    TimelineChip {
                        id: chip
                        anchors.fill: parent
                        chipWidth: root.chipWidth
                        chipHeight: root.chipHeight
                        chipIconSize: root.chipIconSize
                        chipDeleteButtonSize: root.chipDeleteButtonSize
                        chipButtonWidth: root.chipButtonWidth
                        chipButtonHeight: root.chipButtonHeight
                        commandData: ({
                            name: model.name,
                            type: model.type,
                            reps: model.reps,
                            duration: model.duration,
                            direction: model.direction,
                            supportsDirection: model.supportsDirection,
                            supportsDuration: model.supportsDuration
                        })
                        onDeleteRequested: {
                            console.log("[Timeline] Delete requested for index:", wrapper.itemIndex, "count:", timelineModel.count)
                            timelineModel.remove(wrapper.itemIndex)
                            console.log("[Timeline] After delete, count:", timelineModel.count)
                        }
                        onRepsChanged: {
                            timelineModel.setProperty(wrapper.itemIndex, "reps", reps)
                        }
                        onDurationChanged: {
                            timelineModel.setProperty(wrapper.itemIndex, "duration", duration)
                        }
                        onDirectionChanged: {
                            timelineModel.setProperty(wrapper.itemIndex, "direction", direction)
                        }
                    }

                    // Delete button (red rectangle) - directly in wrapper to ensure it captures clicks
                    Button {
                        anchors { top: parent.top; right: parent.right; topMargin: 4; rightMargin: 4 }
                        width: root.chipDeleteButtonSize
                        height: root.chipDeleteButtonSize
                        flat: true
                        z: 100

                        contentItem: Image {
                            source: parent.down ? "qrc:/images/android/pressed_delete_button.png"
                                                 : "qrc:/images/android/delete_button.png"
                            fillMode: Image.PreserveAspectFit
                        }
                        background: Item {}

                        onClicked: {
                            timelineModel.remove(wrapper.itemIndex)
                        }
                    }

                    MouseArea {
                        id: dragMouse
                        anchors { top: parent.top; left: parent.left; right: parent.right }
                        height: chip.iconTileHeight
                        drag.target: undefined
                        drag.threshold: 4
                        hoverEnabled: false

                        property int startX: 0
                        property bool canDrag: false
                        property int minDragDelta: 8

                        Timer {
                            id: holdTimer
                            interval: 200
                            onTriggered: {
                                dragMouse.canDrag = true
                                dragMouse.preventStealing = true
                            }
                        }

                        onPressed: {
                            startX = mouseX
                            dragTranslate.x = 0
                            canDrag = false
                            holdTimer.start()
                        }

                        onPositionChanged: {
                            if (!pressed) return
                            var delta = Math.abs(mouseX - startX)
                            if (!canDrag && delta > minDragDelta) {
                                holdTimer.stop()
                                return
                            }
                            if (!canDrag) return
                            dragTranslate.x = mouseX - startX
                        }

                        onReleased: {
                            holdTimer.stop()
                            dragMouse.preventStealing = false
                            if (!canDrag || dragTranslate.x === 0) {
                                dragTranslate.x = 0
                                return
                            }
                            canDrag = false

                            var colW = wrapper.width + sequenceList.spacing
                            var worldPos = wrapper.mapToItem(sequenceList, dragTranslate.x, 0).x
                            var newIndex = Math.round(worldPos / colW)
                            newIndex = Math.max(0, Math.min(timelineModel.count - 1, newIndex))
                            if (newIndex !== wrapper.itemIndex) {
                                timelineModel.move(wrapper.itemIndex, newIndex, 1)
                            }
                            dragTranslate.x = 0
                        }

                        onCanceled: {
                            holdTimer.stop()
                            dragMouse.preventStealing = false
                            canDrag = false
                            dragTranslate.x = 0
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

            // Clear Timeline button (left side before Play/Stop)
            Button {
                implicitWidth: root.playClearButtonSize
                implicitHeight: root.playClearButtonSize
                enabled: timelineModel.count > 0
                opacity: enabled ? 1.0 : 0.4
                contentItem: Image {
                    source: parent.down ? "qrc:/images/android/pressed_delete_timeline_button.png"
                                         : "qrc:/images/android/delete_timeline_button.png"
                    sourceSize.width: root.playClearButtonSize
                    sourceSize.height: root.playClearButtonSize
                    fillMode: Image.PreserveAspectFit
                }
                background: Item {}
                ToolTip.visible: hovered
                ToolTip.text: root.tr("clear_timeline") || "Clear Timeline"
                onClicked: {
                    root.clearTimeline()
                }
            }

            // Single circular Play/Stop toggle button
            Button {
                implicitWidth: root.playClearButtonSize
                implicitHeight: root.playClearButtonSize
                enabled: root.isPlayingTimeline || (Robot.connected && timelineModel.count > 0)
                opacity: enabled ? 1.0 : 0.4
                contentItem: Image {
                    source: parent.down ? "qrc:/images/android/pressed_play_button.png"
                                         : "qrc:/images/android/play_button.png"
                    sourceSize.width: root.playClearButtonSize
                    sourceSize.height: root.playClearButtonSize
                    fillMode: Image.PreserveAspectFit
                }
                background: Item {}
                onClicked: {
                    if (!root.isPlayingTimeline) {
                        root.playClicked(timelineModel.count)
                    } else {
                        root.stopClicked()
                    }
                }
            }
        }
    }

    // Dialogs for timeline item selection (shown when useDialogSelectors flag is true)
    MouthSelectorDialog {
        id: mouthDialog
        executePreviewed: false
        onMouthSelected: (name) => root.addCommand("mouth", name)
    }

    GestureSelectorDialog {
        id: gestureDialog
        executePreviewed: false
        onGestureSelected: (name) => root.addCommand("animation", name)
    }

    MovementSelectorDialog {
        id: movementDialog
        onMovementSelected: (name, type) => root.addCommand(type, name)
    }

    // ─── Help Dialog ────────────────────────────────────────────────────────
    Dialog {
        id: helpDialog
        modal: true
        parent: Overlay.overlay
        width: Math.round(root.width * root.helpDialogWidthRatio)
        anchors.centerIn: parent

        property real helpContentH: helpTitle.height + helpImg.height
                                    + helpText.implicitHeight + helpCloseBtn.height
                                    + 3 * helpCol.spacing
        height: Math.ceil(helpContentH) + 48 + Math.round(helpContentH * 0.05)

        background: Rectangle {
            radius: 20
            color: "#ffffff"
            border.color: Config.get("color_accent") || "#21a69b"
            border.width: 2
        }

        contentItem: Column {
            id: helpCol
            spacing: 18
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 24
            }

            Text {
                id: helpTitle
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.tr("title")
                font.pixelSize: 20
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Image {
                id: helpImg
                anchors.horizontalCenter: parent.horizontalCenter
                width: 130
                height: 130
                source: "qrc:/images/android/timeline_button.png"
                sourceSize.width: 130
                sourceSize.height: 130
                fillMode: Image.PreserveAspectFit
            }

            Text {
                id: helpText
                anchors {
                    left: parent.left
                    right: parent.right
                }
                text: {
                    var baseText = root.tr("how_to_play_text")
                    var zowiName = Session.getString("activeZowiName", "Zowi")
                    return baseText.replace("ZOWINAME", zowiName)
                }
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 14
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Button {
                id: helpCloseBtn
                anchors.horizontalCenter: parent.horizontalCenter
                implicitWidth: 160
                implicitHeight: 44
                text: root.tr("close")

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
                    color: helpCloseBtn.pressed ? Config.get("color_accent_pressed") || "#17736c" : (Config.get("color_accent") || "#21a69b")
                }

                onClicked: helpDialog.close()
            }
        }
    }
}