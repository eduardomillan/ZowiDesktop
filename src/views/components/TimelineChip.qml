// TimelineChip: one tile in the horizontal GameTimelineScreen strip.
// 200x180 solid-color chip (color by type) with a centered white glyph icon,
// a top-right delete overlay, and up to 3 below-icon CycleIconButtons
// (reps always, duration for movements, direction only when supportsDirection).
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Item {
    id: root
    property var commandData: ({})
    property int chipWidth: 200              // Configurable chip width
    property int chipHeight: 180             // Configurable chip height
    property int chipIconSize: 72            // Configurable icon size
    property int chipDeleteButtonSize: 40    // Configurable delete button size
    property int chipButtonWidth: 64         // Configurable button width (reps/duration/direction)
    property int chipButtonHeight: 48        // Configurable button height (reps/duration/direction)

    implicitWidth: chipWidth
    implicitHeight: chipHeight

    signal deleteRequested()
    signal repsChanged(int reps)
    signal durationChanged(string duration)
    signal directionChanged(string direction)

    function tr(source) { return Translator.translate("GameTimelineScreen.qml", source) }

    // Map action name to icon file
    function resolveIcon(type, name) {
        var iconMap = {
            movement: {
                "Walk Forward":      "qrc:/images/android/pad_walk_forward.png",
                "Walk Backward":     "qrc:/images/android/pad_walk_backward.png",
                "Turn Left":         "qrc:/images/android/pad_turn_left_button.png",
                "Turn Right":        "qrc:/images/android/pad_turn_right_button.png",
                "Moonwalker Left":   "qrc:/images/android/pad_moonwalker_left.png",
                "Moonwalker Right":  "qrc:/images/android/pad_moonwalker_right.png",
                "Bend Forward":      "qrc:/images/android/pad_bend_button.png",
                "Shake Leg":         "qrc:/images/android/pad_shake_leg_button.png",
                "Up/Down":           "qrc:/images/android/pad_updown_button.png",
                "Jitter":            "qrc:/images/android/pad_jitter_button.png",
                "Swing":             "qrc:/images/android/pad_swing_button.png",
                "Flapping":          "qrc:/images/android/pad_flapping_button.png",
                "Crusaito":          "qrc:/images/android/pad_crusaito_button.png"
            },
            animation: {
                "Happy":       "qrc:/images/android/animation_happy_icon.png",
                "SuperHappy":  "qrc:/images/android/animation_super_happy_icon.png",
                "Sad":         "qrc:/images/android/animation_sad_icon.png",
                "Sleeping":    "qrc:/images/android/animation_sleepy_icon.png",
                "Fart":        "qrc:/images/android/animation_fart_icon.png",
                "Confused":    "qrc:/images/android/animation_confused_icon.png",
                "Love":        "qrc:/images/android/animation_in_love_icon.png",
                "Angry":       "qrc:/images/android/animation_angry_icon.png",
                "Fretful":     "qrc:/images/android/animation_anxious_icon.png",
                "Magic":       "qrc:/images/android/animation_magic_icon.png",
                "Wave":        "qrc:/images/android/animation_wave_icon.png",
                "Victory":     "qrc:/images/android/interrogation_icon.png",
                "Fail":        "qrc:/images/android/interrogation_icon.png"
            },
            mouth: {
                "Smile":         "qrc:/images/android/smile_icon.png",
                "HappyOpen":     "qrc:/images/android/happy_open_icon.png",
                "HappyClosed":   "qrc:/images/android/interrogation_icon.png",
                "Heart":         "qrc:/images/android/heart_icon.png",
                "BigSurprise":   "qrc:/images/android/big_surprise_icon.png",
                "SmallSurprise": "qrc:/images/android/small_surprise_icon.png",
                "TongueOut":     "qrc:/images/android/tongue_out_icon.png",
                "Vamp1":         "qrc:/images/android/vamp1_icon.png",
                "Vamp2":         "qrc:/images/android/vamp2_icon.png",
                "LineMouth":     "qrc:/images/android/line_mouth_icon.png",
                "Confused":      "qrc:/images/android/confused_icon.png",
                "DiagLeft":      "qrc:/images/android/diagonal_icon.png",
                "DiagRight":     "qrc:/images/android/interrogation_icon.png",
                "Sad":           "qrc:/images/android/sad_icon.png",
                "SadOpen":       "qrc:/images/android/sad_open_icon.png",
                "SadClosed":     "qrc:/images/android/sad_closed_icon.png",
                "Ok":            "qrc:/images/android/ok_mouth_icon.png",
                "X":             "qrc:/images/android/x_mouth_icon.png",
                "Interrogation": "qrc:/images/android/interrogation_icon.png",
                "Thunder":       "qrc:/images/android/thunder_icon.png",
                "Culito":        "qrc:/images/android/culito_icon.png",
                "Angry":         "qrc:/images/android/angry_icon.png"
            }
        }
        var typeMap = iconMap[type] || {}
        return typeMap[name] || "qrc:/images/android/interrogation_icon.png"
    }

    readonly property var typeColors: ({
        movement:  { base: Config.get("color_accent") || "#21a69b", dark: Config.get("color_accent_pressed") || "#17736c" },
        animation: { base: "#eb0028", dark: "#a4001b" },
        mouth:     { base: "#f6a000", dark: "#ab6f00" }
    })
    readonly property var colors: typeColors[commandData.type] || typeColors.movement
    readonly property var iconSource: resolveIcon(commandData.type, commandData.name)

    ColumnLayout {
        anchors.fill: parent
        spacing: 4
        anchors.margins: 0

        // Icon tile: rounded corners, dark bottom lip; fills remaining height after buttons
        Rectangle {
            id: tile
            Layout.preferredWidth: root.chipWidth
            Layout.fillHeight: true
            radius: 14
            color: root.colors.base

            Rectangle {  // bottom "pressed" lip, ~8% height
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                height: parent.height * 0.08
                radius: 14
                color: root.colors.dark
            }

            Image {
                anchors.centerIn: parent
                source: root.iconSource
                sourceSize: Qt.size(root.chipIconSize, root.chipIconSize)
                width: root.chipIconSize
                height: root.chipIconSize
                fillMode: Image.PreserveAspectFit
            }

            ToolTip.visible: chipMouse.containsMouse
            ToolTip.text: root.commandData.name || ""

            MouseArea { id: chipMouse; anchors.fill: parent; hoverEnabled: true; z: -1 }
        }

        // Pill row: reps always, duration if supportsDuration, direction if supportsDirection
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 2

            CycleIconButton {
                buttonWidth: root.chipButtonWidth
                buttonHeight: root.chipButtonHeight
                states: [
                    { value: 1,  icon: "qrc:/images/android/steps_1_button.png",  pressedIcon: "qrc:/images/android/pressed_steps_1_button.png" },
                    { value: 5,  icon: "qrc:/images/android/steps_5_button.png",  pressedIcon: "qrc:/images/android/pressed_steps_5_button.png" },
                    { value: 10, icon: "qrc:/images/android/steps_10_button.png", pressedIcon: "qrc:/images/android/pressed_steps_10_button.png" }
                ]
                currentValue: root.commandData.reps || 1
                toolTip: root.tr("repetitions")
                onValueChanged: root.repsChanged(value)
            }

            CycleIconButton {
                visible: !!root.commandData.supportsDuration
                buttonWidth: root.chipButtonWidth
                buttonHeight: root.chipButtonHeight
                states: [
                    { value: "Slow",   icon: "qrc:/images/android/speed_low_button.png",    pressedIcon: "qrc:/images/android/pressed_speed_low_button.png" },
                    { value: "Medium", icon: "qrc:/images/android/speed_medium_button.png", pressedIcon: "qrc:/images/android/pressed_speed_medium_button.png" },
                    { value: "Fast",   icon: "qrc:/images/android/speed_fast_button.png",   pressedIcon: "qrc:/images/android/pressed_speed_fast_button.png" }
                ]
                currentValue: root.commandData.duration || "Medium"
                toolTip: root.tr("duration")
                onValueChanged: root.durationChanged(value)
            }

            CycleIconButton {
                visible: !!root.commandData.supportsDirection
                buttonWidth: root.chipButtonWidth
                buttonHeight: root.chipButtonHeight
                states: [
                    { value: "Left",  icon: "qrc:/images/android/direction_left_button.png",  pressedIcon: "qrc:/images/android/pressed_direction_left_button.png" },
                    { value: "Right", icon: "qrc:/images/android/direction_right_button.png", pressedIcon: "qrc:/images/android/pressed_direction_right_button.png" }
                ]
                currentValue: root.commandData.direction || "Left"
                toolTip: root.tr("dir")
                onValueChanged: root.directionChanged(value)
            }
        }
    }

}
