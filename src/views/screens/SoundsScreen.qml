// SoundsScreen: grid of firmware melodies for Zowi.
// Tap a melody to play it (K command). Badge icon by default; Zowi-face icon
// while that melody is playing, until the robot's final ack (&&F%%).
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "SoundsScreen"
    title: tr("sounds_title")
    showBackButton: true

    function tr(source) { return Translator.translate("SoundsScreen.qml", source) }
    function send(cmd) { if (Robot.connected) Robot.sendData(cmd) }

    Component.onCompleted: Robot.setDataPollingEnabled(false)
    Component.onDestruction: Robot.setDataPollingEnabled(true)

    property string playingMelody: ""
    property real iconSize: 80
    property real cellSpacing: 16

    readonly property var melodyIdByName: ({
        "Connection": Commands.MelodyConnection,
        "Disconnection": Commands.MelodyDisconnection,
        "Surprise": Commands.MelodySurprise,
        "OhOoh": Commands.MelodyOhOoh,
        "OhOoh2": Commands.MelodyOhOoh2,
        "Cuddly": Commands.MelodyCuddly,
        "Sleeping": Commands.MelodySleeping,
        "Happy": Commands.MelodyHappy,
        "SuperHappy": Commands.MelodySuperHappy,
        "HappyShort": Commands.MelodyHappyShort,
        "Sad": Commands.MelodySad,
        "Confused": Commands.MelodyConfused,
        "Fart1": Commands.MelodyFart1,
        "Fart2": Commands.MelodyFart2,
        "Fart3": Commands.MelodyFart3,
        "Mode1": Commands.MelodyMode1,
        "Mode2": Commands.MelodyMode2,
        "Mode3": Commands.MelodyMode3,
        "ButtonPushed": Commands.MelodyButtonPushed
    })

    readonly property var melodyOptions: [
        { name: "Connection",    label: "melody_connection",     badge: "qrc:/images/sounds/melody_connection_badge.png",     face: "qrc:/images/sounds/melody_connection.png" },
        { name: "Disconnection", label: "melody_disconnection",  badge: "qrc:/images/sounds/melody_disconnection_badge.png",  face: "qrc:/images/sounds/melody_disconnection.png" },
        { name: "Surprise",      label: "melody_surprise",       badge: "qrc:/images/sounds/melody_surprise_badge.png",       face: "qrc:/images/sounds/melody_surprise.png" },
        { name: "OhOoh",         label: "melody_oh_ooh",         badge: "qrc:/images/sounds/melody_oh_ooh_badge.png",         face: "qrc:/images/sounds/melody_oh_ooh.png" },
        { name: "OhOoh2",        label: "melody_oh_ooh2",        badge: "qrc:/images/sounds/melody_oh_ooh2_badge.png",        face: "qrc:/images/sounds/melody_oh_ooh2.png" },
        { name: "Cuddly",        label: "melody_cuddly",         badge: "qrc:/images/sounds/melody_cuddly_badge.png",         face: "qrc:/images/sounds/melody_cuddly.png" },
        { name: "Sleeping",      label: "melody_sleeping",       badge: "qrc:/images/sounds/melody_sleeping_badge.png",       face: "qrc:/images/sounds/melody_sleeping.png" },
        { name: "Happy",         label: "melody_happy",          badge: "qrc:/images/sounds/melody_happy_badge.png",          face: "qrc:/images/sounds/melody_happy.png" },
        { name: "SuperHappy",    label: "melody_super_happy",    badge: "qrc:/images/sounds/melody_super_happy_badge.png",    face: "qrc:/images/sounds/melody_super_happy.png" },
        { name: "HappyShort",    label: "melody_happy_short",    badge: "qrc:/images/sounds/melody_happy_short_badge.png",    face: "qrc:/images/sounds/melody_happy_short.png" },
        { name: "Sad",           label: "melody_sad",            badge: "qrc:/images/sounds/melody_sad_badge.png",            face: "qrc:/images/sounds/melody_sad.png" },
        { name: "Confused",      label: "melody_confused",       badge: "qrc:/images/sounds/melody_confused_badge.png",       face: "qrc:/images/sounds/melody_confused.png" },
        { name: "Fart1",         label: "melody_fart1",          badge: "qrc:/images/sounds/melody_fart1_badge.png",          face: "qrc:/images/sounds/melody_fart1.png" },
        { name: "Fart2",         label: "melody_fart2",          badge: "qrc:/images/sounds/melody_fart2_badge.png",          face: "qrc:/images/sounds/melody_fart2.png" },
        { name: "Fart3",         label: "melody_fart3",          badge: "qrc:/images/sounds/melody_fart3_badge.png",          face: "qrc:/images/sounds/melody_fart3.png" },
        { name: "Mode1",         label: "melody_mode1",          badge: "qrc:/images/sounds/melody_mode1_badge.png",          face: "qrc:/images/sounds/melody_mode1.png" },
        { name: "Mode2",         label: "melody_mode2",          badge: "qrc:/images/sounds/melody_mode2_badge.png",          face: "qrc:/images/sounds/melody_mode2.png" },
        { name: "Mode3",         label: "melody_mode3",          badge: "qrc:/images/sounds/melody_mode3_badge.png",          face: "qrc:/images/sounds/melody_mode3.png" },
        { name: "ButtonPushed",  label: "melody_button_pushed",  badge: "qrc:/images/sounds/melody_button_pushed_badge.png",  face: "qrc:/images/sounds/melody_button_pushed.png" }
    ]

    function selectMelody(name) {
        if (playingMelody !== "")
            return
        var id = melodyIdByName[name]
        if (id === undefined)
            return
        playingMelody = name
        send(Commands.sing(id))
        console.log("[SoundsScreen] " + name + " -> K command sent")
    }

    Connections {
        target: Robot
        function onFinalAckReceived() {
            if (root.playingMelody !== "")
                root.playingMelody = ""
        }
        function onConnectionChanged() {
            if (!Robot.connected)
                root.playingMelody = ""
        }
    }

    Item {
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        width: parent.width * 0.85
        height: parent.height
        clip: true

        Flow {
            anchors.centerIn: parent
            width: parent.width
            spacing: root.cellSpacing

            Repeater {
                model: root.melodyOptions

                Rectangle {
                    width: root.iconSize + 8
                    height: root.iconSize + 24
                    color: "transparent"

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        Image {
                            id: melodyImg
                            width: root.iconSize
                            height: root.iconSize
                            source: root.playingMelody === modelData.name
                                    ? modelData.face
                                    : modelData.badge
                            sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
                            fillMode: Image.PreserveAspectFit
                            anchors.horizontalCenter: parent.horizontalCenter
                            opacity: (root.playingMelody !== ""
                                      && root.playingMelody !== modelData.name) ? 0.45 : 1.0

                            MouseArea {
                                anchors.fill: parent
                                enabled: root.playingMelody === ""
                                onClicked: root.selectMelody(modelData.name)
                            }
                        }

                        Text {
                            text: root.tr(modelData.label)
                            color: Config.get("color_primary") || "#2d5a2d"
                            font.pixelSize: Math.max(9, root.iconSize * 0.16)
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: root.iconSize + 8
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
