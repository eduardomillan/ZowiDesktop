// GestureScreen: grid of body-language gestures for Zowi.
// Tap any gesture to play the corresponding animation (H command).
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "GestureScreen"
    title: tr("gestures_title")
    showBackButton: true

    function tr(source) { return Translator.translate("GestureScreen.qml", source) }
    function send(cmd) { if (Robot.connected) Robot.sendData(cmd) }

    Component.onCompleted: Robot.setDataPollingEnabled(false)
    Component.onDestruction: Robot.setDataPollingEnabled(true)

    property string selectedGesture: ""
    // Icon size (px). Change this locally to make the gestures larger/smaller.
    property real iconSize: 80
    property real cellSpacing: 20

    readonly property var gestureIdByName: ({
        "Happy": Commands.GestureHappy, "SuperHappy": Commands.GestureSuperHappy,
        "Sad": Commands.GestureSad, "Sleeping": Commands.GestureSleeping,
        "Fart": Commands.GestureFart, "Confused": Commands.GestureConfused,
        "Love": Commands.GestureLove, "Angry": Commands.GestureAngry,
        "Fretful": Commands.GestureFretful, "Magic": Commands.GestureMagic
    })

    readonly property var gestureOptions: [
        { name: "Happy",      normal: "qrc:/images/android/animation_happy_button.png",       pressed: "qrc:/images/android/pressed_animation_happy_button.png" },
        { name: "SuperHappy", normal: "qrc:/images/android/animation_super_happy_button.png",  pressed: "qrc:/images/android/pressed_animation_super_happy_button.png" },
        { name: "Sad",        normal: "qrc:/images/android/animation_sad_button.png",          pressed: "qrc:/images/android/pressed_animation_sad_button.png" },
        { name: "Sleeping",   normal: "qrc:/images/android/animation_sleppy_button.png",       pressed: "qrc:/images/android/pressed_animation_sleppy_button.png" },
        { name: "Fart",       normal: "qrc:/images/android/animation_fart_button.png",         pressed: "qrc:/images/android/pressed_animation_fart_button.png" },
        { name: "Confused",   normal: "qrc:/images/android/animation_confused_button.png",     pressed: "qrc:/images/android/pressed_animation_confused_button.png" },
        { name: "Love",       normal: "qrc:/images/android/animation_in_love_button.png",      pressed: "qrc:/images/android/pressed_animation_in_love_button.png" },
        { name: "Angry",      normal: "qrc:/images/android/animation_angry_button.png",        pressed: "qrc:/images/android/pressed_animation_angry_button.png" },
        { name: "Fretful",    normal: "qrc:/images/android/animation_anxious_button.png",      pressed: "qrc:/images/android/pressed_animation_anxious_button.png" },
        { name: "Magic",      normal: "qrc:/images/android/animation_magic_button.png",        pressed: "qrc:/images/android/pressed_animation_magic_button.png" }
    ]

    function selectGesture(name) {
        var id = gestureIdByName[name]
        if (id === undefined) return
        selectedGesture = name
        send(Commands.gestureById(id))
        console.log("[GestureScreen] " + name + " -> H command sent")
    }

    Item {
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        width: parent.width * 0.8
        height: parent.height

        clip: true

        Flow {
            anchors.centerIn: parent
            width: parent.width
            spacing: root.cellSpacing

            Repeater {
                model: root.gestureOptions

                Rectangle {
                    width: root.iconSize + 8
                    height: root.iconSize + 24
                    color: "transparent"

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        Image {
                            id: gestureImg
                            width: root.iconSize
                            height: root.iconSize
                            source: modelData.normal
                            sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
                            fillMode: Image.PreserveAspectFit
                            anchors.horizontalCenter: parent.horizontalCenter

                            MouseArea {
                                anchors.fill: parent
                                onPressed: gestureImg.source = modelData.pressed
                                onReleased: {
                                    gestureImg.source = modelData.normal
                                    root.selectGesture(modelData.name)
                                }
                            }
                        }

                        Text {
                            text: modelData.name
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
