// MouthScreen: grid of LED-matrix mouth expressions for Zowi.
// Tap any mouth to send the L command; the expression stays until changed.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "MouthScreen"
    title: tr("mouths_title")
    showBackButton: true

    function tr(source) { return Translator.translate("MouthScreen.qml", source) }
    function send(cmd) { if (Robot.connected) Robot.sendData(cmd) }

    Component.onCompleted: Robot.setDataPollingEnabled(false)
    Component.onDestruction: Robot.setDataPollingEnabled(true)

    property string selectedMouth: ""
    // Icon size (px). Change this locally to make the mouths larger/smaller.
    property real iconSize: 80
    property real cellSpacing: 20

    readonly property var mouthIdByName: ({
        "Smile": Commands.MouthSmile, "HappyOpen": Commands.MouthHappyOpen,
        "Heart": Commands.MouthHeart, "BigSurprise": Commands.MouthBigSurprise,
        "SmallSurprise": Commands.MouthSmallSurprise, "TongueOut": Commands.MouthTongueOut,
        "Vamp1": Commands.MouthVamp1, "Vamp2": Commands.MouthVamp2,
        "LineMouth": Commands.MouthLineMouth, "Confused": Commands.MouthConfused,
        "Diagonal": Commands.MouthDiagonal, "Sad": Commands.MouthSad,
        "SadOpen": Commands.MouthSadOpen, "SadClosed": Commands.MouthSadClosed,
        "Ok": Commands.MouthOk, "X": Commands.MouthX,
        "Interrogation": Commands.MouthInterrogation, "Thunder": Commands.MouthThunder,
        "Culito": Commands.MouthCulito, "Angry": Commands.MouthAngry
    })

    readonly property var mouthOptions: [
        { name: "Smile",         normal: "qrc:/images/android/smile_button.png",         pressed: "qrc:/images/android/pressed_smile_button.png" },
        { name: "HappyOpen",     normal: "qrc:/images/android/happy_open_button.png",     pressed: "qrc:/images/android/pressed_happy_open_button.png" },
        { name: "Heart",         normal: "qrc:/images/android/heart_button.png",          pressed: "qrc:/images/android/pressed_heart_button.png" },
        { name: "BigSurprise",   normal: "qrc:/images/android/big_surprise_button.png",   pressed: "qrc:/images/android/pressed_big_surprise_button.png" },
        { name: "SmallSurprise", normal: "qrc:/images/android/small_surprise_button.png", pressed: "qrc:/images/android/pressed_small_surprise_button.png" },
        { name: "TongueOut",     normal: "qrc:/images/android/tongue_out_button.png",     pressed: "qrc:/images/android/pressed_tongue_out_button.png" },
        { name: "Vamp1",         normal: "qrc:/images/android/vamp1_button.png",          pressed: "qrc:/images/android/pressed_vamp1_button.png" },
        { name: "Vamp2",         normal: "qrc:/images/android/vamp2_button.png",          pressed: "qrc:/images/android/pressed_vamp2_button.png" },
        { name: "LineMouth",     normal: "qrc:/images/android/line_mouth_button.png",     pressed: "qrc:/images/android/pressed_line_mouth_button.png" },
        { name: "Confused",      normal: "qrc:/images/android/confused_button.png",       pressed: "qrc:/images/android/pressed_confused_button.png" },
        { name: "Diagonal",      normal: "qrc:/images/android/diagonal_button.png",       pressed: "qrc:/images/android/pressed_diagonal_button.png" },
        { name: "Sad",           normal: "qrc:/images/android/sad_button.png",            pressed: "qrc:/images/android/pressed_sad_button.png" },
        { name: "SadOpen",       normal: "qrc:/images/android/sad_open_button.png",       pressed: "qrc:/images/android/pressed_sad_open_button.png" },
        { name: "SadClosed",     normal: "qrc:/images/android/sad_closed_button.png",     pressed: "qrc:/images/android/pressed_sad_closed_button.png" },
        { name: "Ok",            normal: "qrc:/images/android/ok_mouth_button.png",       pressed: "qrc:/images/android/pressed_ok_mouth_button.png" },
        { name: "X",             normal: "qrc:/images/android/x_mouth_button.png",        pressed: "qrc:/images/android/pressed_x_mouth_button.png" },
        { name: "Interrogation", normal: "qrc:/images/android/interrogation_button.png",  pressed: "qrc:/images/android/pressed_interrogation_button.png" },
        { name: "Thunder",       normal: "qrc:/images/android/thunder_button.png",        pressed: "qrc:/images/android/pressed_thunder_button.png" },
        { name: "Culito",        normal: "qrc:/images/android/culito_button.png",         pressed: "qrc:/images/android/pressed_culito_button.png" },
        { name: "Angry",         normal: "qrc:/images/android/angry_button.png",          pressed: "qrc:/images/android/pressed_angry_button.png" }
    ]

    function selectMouth(name) {
        var id = mouthIdByName[name]
        if (id === undefined) return
        selectedMouth = name
        send(Commands.mouthById(id))
        console.log("[MouthScreen] " + name + " -> L command sent")
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
                model: root.mouthOptions

                Rectangle {
                    width: root.iconSize + 8
                    height: root.iconSize + 24
                    color: "transparent"

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        Image {
                            id: mouthImg
                            width: root.iconSize
                            height: root.iconSize
                            source: modelData.normal
                            sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
                            fillMode: Image.PreserveAspectFit
                            anchors.horizontalCenter: parent.horizontalCenter

                            MouseArea {
                                anchors.fill: parent
                                onPressed: mouthImg.source = modelData.pressed
                                onReleased: {
                                    mouthImg.source = modelData.normal
                                    root.selectMouth(modelData.name)
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
