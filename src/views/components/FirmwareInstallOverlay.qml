import QtQuick 2.15
import QtQuick.Controls 2.15

// Shared firmware-install overlay: progress box (Phase 2) + low-battery
// confirmation dialog (Phase 3) for ANY firmware flash. The host screen owns
// the state (`active`, `progress`, `batteryLow` fed from Robot's restore
// signals) and the finished-feedback (MessageBar); this component only renders
// the visual shell and routes the dialog buttons back to Robot, exactly like
// the restore flow in SettingsScreen.

Item {
    id: root
    visible: root.active

    // Host-driven state (see SettingsScreen / ProjectScreen Connections).
    property bool active: false
    property int progress: 0
    property bool batteryLow: false

    // Localized text supplied by the host (Translator is per-screen context).
    property string progressText: ""   // e.g. "Alarm %1%"
    property string titleText: ""      // low-battery confirmation title
    property string confirmText: ""
    property string cancelText: ""

    // Progress (Phase 2): occupies the same bottom position as the MessageBar
    // while a firmware restore runs, covering it (z above). The status text
    // (yellow) sits above the progress bar.
    Rectangle {
        id: progressBox
        visible: root.active && !root.batteryLow
        z: 1
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 56
        color: Config.get("color_accent_pressed") || "#17736c"

        Text {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                topMargin: 6
            }
            horizontalAlignment: Text.AlignHCenter
            text: root.progressText.length ? root.progressText.arg(root.progress) : ""
            color: Config.get("color_warning_text") || "#f1c40f"
            font.pixelSize: 13
            font.bold: true
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 40
                rightMargin: 40
                bottom: parent.bottom
                bottomMargin: 10
            }
            height: 10
            radius: 5
            color: "#0f4f4a"

            Rectangle {
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                }
                width: Math.max(2, parent.width * (root.progress / 100.0))
                radius: 5
                color: Config.get("color_accent") || "#21a69b"
            }
        }
    }

    // Phase 3: low-battery confirmation dialog shown over the progress bar while
    // the restore waits for the user's decision. Styled to match the app theme:
    // light app background, a warning-yellow panel with a dark-green border.
    Rectangle {
        id: batteryDialog
        visible: root.batteryLow
        anchors.fill: parent
        color: "transparent"

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 360)
            height: confirmColumn.height + 36
            radius: 12
            color: "#fdfbe7"
            border.color: Config.get("color_primary") || "#2d5a2d"
            border.width: 2

            Column {
                id: confirmColumn
                anchors.centerIn: parent
                width: parent.width - 36
                spacing: 14

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: root.titleText
                    color: Config.get("color_primary") || "#2d5a2d"
                    font.pixelSize: 14
                    font.bold: true
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 16

                    Button {
                        text: root.confirmText
                        background: Rectangle {
                            color: Config.get("color_accent") || "#21a69b"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#ffffff"
                            font.pixelSize: 13
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            root.batteryLow = false
                            Robot.confirmRestoreBattery(true)
                        }
                    }

                    Button {
                        text: root.cancelText
                        background: Rectangle {
                            color: Config.get("color_danger") || "#e74c3c"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#ffffff"
                            font.pixelSize: 13
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            root.batteryLow = false
                            Robot.confirmRestoreBattery(false)
                        }
                    }
                }
            }
        }
    }
}