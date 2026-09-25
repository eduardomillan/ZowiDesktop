// GestureScreen: wrapper for GestureSelectorContent, used as full-screen selector from PadScreen and (conditionally) from GameTimelineScreen.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "GestureScreen"
    title: tr("gestures_title")
    showBackButton: true

    signal gestureSelected(string name)

    function tr(source) { return Translator.translate("GestureScreen.qml", source) }

    GestureSelectorContent {
        anchors.fill: parent
        onGestureSelected: (name) => root.gestureSelected(name)
    }
}
