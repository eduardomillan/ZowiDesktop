// MouthEditorScreen: mouth editor. Draw Zowi's mouth dot-by-dot on a 6x5 LED
// grid; every change is sent live to the robot as an L command (same wire
// semantics as ZowiAppReborn's MouthsEditor: 30 grid cells become the 32-bit
// binary pattern "L 00<30bits>\r"). Footer offers "clear all" / "select all".
// The grid itself lives in the shared component MouthGrid.qml (also used by
// the "Pintabocas" game); this screen only adds the live-send + footer.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "MouthEditorScreen"
    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: true
    footerHeight: 0

    // The identity poll (E/I/B burst) drains the robot's command queue and can
    // delay/interrupt the live mouth updates — pause it while editing.
    Component.onCompleted: Robot.setDataPollingEnabled(false)
    Component.onDestruction: Robot.setDataPollingEnabled(true)

    function tr(source) { return Translator.translate("MouthEditorScreen.qml", source) }

    // Live-send: every grid change (patternChanged) rebuilds the 32-bit mouth
    // pattern and pushes it to the robot.
    function sendGrid() {
        if (Robot.connected) {
            var cmd = Commands.mouth(grid.matrix)
            Robot.sendData(cmd)
            console.log("[MouthEditorScreen] sent: " + cmd.trim())
        }
    }

    function clearAll() {
        grid.clearAll()
        console.log("[MouthEditorScreen] clear all")
    }

    function selectAll() {
        grid.selectAll()
        console.log("[MouthEditorScreen] select all")
    }

    MouthGrid {
        id: grid
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: -75
        }
        onPatternChanged: sendGrid()
    }

    // Bit-pattern display below the grid (copyable)
    Column {
        id: patternColumn
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: grid.bottom
            topMargin: 12
        }
        spacing: 4

        // Informative label (i18n)
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.tr("pattern_label")
            color: Config.get("color_primary") || "#2d5a2d"
            font.pixelSize: 13
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        // Pattern text with white background
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: patternText.implicitWidth + 24
            height: patternText.implicitHeight + 12
            color: "#ffffff"
            radius: 6
            border.color: Config.get("color_primary") || "#2d5a2d"
            border.width: 1

            TextEdit {
                id: patternText
                anchors.centerIn: parent
                text: grid.patternBits
                color: Config.get("color_primary") || "#2d5a2d"
                font.family: "monospace"
                font.pixelSize: 14
                horizontalAlignment: TextEdit.AlignHCenter
                verticalAlignment: TextEdit.AlignVCenter
                readOnly: true
                selectByMouse: true
            }
        }

        // Buttons below pattern text with 10% window height spacing
        Item {
            width: 1
            height: root.height * 0.1
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            Button {
                id: clearBtn
                implicitWidth: 170
                height: 44
                text: root.tr("clear_all")

                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 22
                    color: clearBtn.pressed ? Config.get("color_error") || "#c0392b" : Config.get("color_danger") || "#e74c3c"
                }

                onClicked: root.clearAll()
            }

            Button {
                id: selectAllBtn
                implicitWidth: 170
                height: 44
                text: root.tr("select_all")

                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 22
                    color: selectAllBtn.pressed ? Config.get("color_primary_pressed") || "#1f4a1f" : Config.get("color_primary") || "#2d5a2d"
                }

                onClicked: root.selectAll()
            }
        }
    }
}