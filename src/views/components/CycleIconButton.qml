// CycleIconButton: tap-to-advance icon button.
// `states` is an ordered array of { value, icon, pressedIcon }.
// Tapping advances to the next entry, wrapping to the first.
// Mirrors Android's MultipleStatesButton.
import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: root
    property var states: []          // [{ value, icon, pressedIcon }]
    property var currentValue
    property string toolTip: ""
    property int buttonWidth: 64     // Configurable width
    property int buttonHeight: 48    // Configurable height
    property int currentIndex: {
        for (var i = 0; i < states.length; i++) {
            if (states[i].value === currentValue) return i
        }
        return 0
    }
    signal valueChanged(var value)

    implicitWidth: buttonWidth
    implicitHeight: buttonHeight
    flat: true
    ToolTip.visible: hovered
    ToolTip.text: root.toolTip
    contentItem: Image {
        source: (root.states[root.currentIndex] || {})[root.down ? "pressedIcon" : "icon"] || ""
        fillMode: Image.PreserveAspectFit
    }
    background: Item {}

    onClicked: {
        if (states.length === 0) return
        var next = (currentIndex + 1) % states.length
        currentValue = states[next].value
        valueChanged(currentValue)
    }
}
