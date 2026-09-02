// CalibrationTrimColumn: a single servo's trim adjustment column (label,
// coarse/fine +/- buttons and current value). Used by CalibrationScreen for
// each of the four servos (YL, YR, RL, RR). Pure view: emits +/- signals and
// lets the parent apply + clamp + send the servo command.
import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: col
    property string label: ""
    property string shortLabel: ""
    property int value: 0

    signal plus()
    signal minus()
    signal plusCoarse()
    signal minusCoarse()

    width: 160
    height: 200
    implicitWidth: 160
    implicitHeight: 200

    function ac() { return Config.get("color_accent") || "#21a69b" }
    function danger() { return Config.get("color_danger") || "#e74c3c" }
    function primary() { return Config.get("color_primary") || "#2d5a2d" }

    Column {
        anchors.centerIn: parent
        spacing: 8

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: col.label
            color: col.primary()
            font.pixelSize: 15
            font.bold: true
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: 120
            height: 36
            text: "+10"
            background: Rectangle {
                color: col.ac()
                radius: 6
            }
            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: col.plusCoarse()
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: col.shortLabel + ": " + col.value + "°"
            color: col.primary()
            font.pixelSize: 18
            font.bold: true
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: 120
            height: 36
            text: "-10"
            background: Rectangle {
                color: col.danger()
                radius: 6
            }
            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: col.minusCoarse()
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10

            Button {
                implicitWidth: 52
                height: 30
                text: "+1"
                background: Rectangle {
                    color: col.ac()
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: col.plus()
            }

            Button {
                implicitWidth: 52
                height: 30
                text: "-1"
                background: Rectangle {
                    color: col.danger()
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: col.minus()
            }
        }
    }
}