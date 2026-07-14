import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    visible: Config.devMode
    width: 320
    height: Math.min(400, parent ? parent.height * 0.6 : 400)

    Component.onCompleted: {
        x = (parent ? parent.width : 0) - width - 8
        y = 8
    }

    Rectangle {
        anchors.fill: parent
        color: "#cc1a1a2e"
        radius: 8
        border.color: "#444"
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: "DEV"
                    color: "#e74c3c"
                    font.bold: true
                    font.pixelSize: 11
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "\u22EE\u22EE"
                    color: "#555"
                    font.pixelSize: 10
                }
            }

            Text {
                color: "#ddd"
                font.pixelSize: 9
                elide: Text.ElideRight
                text: {
                    var s = Bluetooth.connected ? "\u25cf Connected" : "\u25cf Disconnected"
                    if (Bluetooth.deviceAddress)
                        s += "  " + Bluetooth.deviceName + " (" + Bluetooth.deviceAddress + ")"
                    return s
                }
            }

            Text {
                color: "#ddd"
                font.pixelSize: 9
                text: "Battery: " + (Bluetooth.battery >= 0 ? Bluetooth.battery + "%" : "N/A")
            }

            Text {
                color: "#aaa"
                font.pixelSize: 9
                elide: Text.ElideRight
                text: "Zowi: " + Session.loadActiveZowiName() + " / " + Session.loadActiveZowiDeviceAddress()
            }

            Rectangle {
                height: 1
                color: "#555"
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: "LOG"
                    color: "#e67e22"
                    font.pixelSize: 9
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    implicitWidth: 40
                    implicitHeight: 16
                    font.pixelSize: 8
                    text: "clear"

                    contentItem: Text {
                        text: parent.text
                        color: "#ccc"
                        font.pixelSize: 8
                        horizontalAlignment: Text.AlignHCenter
                    }

                    background: Rectangle {
                        color: "#444"
                        radius: 3
                    }

                    onClicked: logModel.clear()
                }
            }

            ListView {
                id: logList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                model: ListModel { id: logModel }

                delegate: Text {
                    text: model.text
                    color: model.isError ? "#e74c3c" : "#aaa"
                    font.pixelSize: 8
                    font.family: "monospace"
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }
        }
    }

    MouseArea {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 24
        drag.target: root
        drag.axis: Drag.XAndYAxis
        cursorShape: Qt.OpenHandCursor
        onPressedChanged: cursorShape = pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    }

    Connections {
        target: Bluetooth

        function onDataReceived(data) {
            appendLog("> " + data, false)
        }

        function onErrorOccurred(message) {
            appendLog("! " + message, true)
        }

        function onConnectionChanged() {
            appendLog("~ " + (Bluetooth.connected ? "connected" : "disconnected"), false)
        }
    }

    function appendLog(text, isError) {
        logModel.append({text: text, isError: isError})
        if (logModel.count > 200) {
            logModel.remove(0, logModel.count - 200)
        }
        logList.positionViewAtEnd()
    }
}
