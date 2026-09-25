// MovementSelector: wrapper for MovementSelectorContent, used as full-screen selector from GameTimelineScreen
// (or conditionally as dialog mode depending on timeline_selectors_as_dialogs config flag).
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "MovementSelector"
    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: true

    function tr(source) { return Translator.translate("MovementSelector.qml", source) }

    signal movementSelected(string name, string type)

    MovementSelectorContent {
        anchors.fill: parent
        onMovementSelected: (name, type) => root.movementSelected(name, type)
    }
}