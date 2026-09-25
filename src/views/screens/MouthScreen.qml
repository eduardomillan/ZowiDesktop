// MouthScreen: wrapper for MouthSelectorContent, used as full-screen selector from PadScreen and (conditionally) from GameTimelineScreen.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "MouthScreen"
    title: tr("mouths_title")
    showBackButton: true

    signal mouthSelected(string name)

    function tr(source) { return Translator.translate("MouthScreen.qml", source) }

    MouthSelectorContent {
        anchors.fill: parent
        onMouthSelected: (name) => root.mouthSelected(name)
    }
}
