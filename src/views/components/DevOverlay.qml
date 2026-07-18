import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    visible: Config.devMode
    property bool collapsed: true
    width: collapsed ? 72 : 320
    height: collapsed ? 28 : Math.min(400, parent ? parent.height * 0.6 : 400)

    x: parent ? parent.width - width - 8 : 0
    y: 8

    onCollapsedChanged: if (!collapsed) refreshSession()

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

            // Header: always visible. Click to toggle, drag to move.
            MouseArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 16
                cursorShape: Qt.OpenHandCursor
                drag.target: root
                drag.axis: Drag.XAndYAxis
                drag.threshold: 4
                property point pressPos
                function handlePress(mouse) {
                    pressPos = Qt.point(mouse.x, mouse.y)
                    cursorShape = Qt.ClosedHandCursor
                }
                function handleRelease(mouse) {
                    cursorShape = Qt.OpenHandCursor
                    if (Math.abs(mouse.x - pressPos.x) < 4 &&
                        Math.abs(mouse.y - pressPos.y) < 4)
                        root.collapsed = !root.collapsed
                }
                onPressed: (mouse) => handlePress(mouse)
                onReleased: (mouse) => handleRelease(mouse)

                RowLayout {
                    anchors.fill: parent
                    spacing: 6

                    Text {
                        text: "DEV"
                        color: "#e74c3c"
                        font.bold: true
                        font.pixelSize: 11
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: root.collapsed ? "▶" : "▼"
                        color: "#888"
                        font.pixelSize: 10
                    }
                }
            }

            // Detailed content — hidden when collapsed
            ColumnLayout {
                visible: !root.collapsed
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                Text {
                    color: "#ddd"
                    font.pixelSize: 9
                    elide: Text.ElideRight
                    text: {
                        var s = Bluetooth.connected ? "● Connected" : "● Disconnected"
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
                    text: Session.loadActiveZowiName() + " / " + Session.loadActiveZowiDeviceAddress()
                }

                Rectangle {
                    height: 1
                    color: "#555"
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "SESSION"
                        color: "#3498db"
                        font.pixelSize: 9
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        implicitWidth: 40
                        implicitHeight: 16
                        font.pixelSize: 8
                        text: "refresh"

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

                        onClicked: refreshSession()
                    }
                }

                Flickable {
                    id: sessionFlick
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    Layout.fillHeight: false
                    clip: true
                    contentWidth: sessionCol.width
                    contentHeight: sessionCol.height
                    onVisibleChanged: if (visible) refreshSession()
                    Component.onCompleted: if (visible) refreshSession()

                    Column {
                        id: sessionCol
                        width: childrenRect.width
                        spacing: 2

                        Repeater {
                            model: ListModel { id: sessionModel }
                            delegate: Text {
                                width: paintedWidth
                                text: model.key + " = " + model.value
                                color: "#aaa"
                                font.pixelSize: 8
                                font.family: "monospace"
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
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

    Connections {
        target: Session

        function onSessionChanged() {
            if (!root.collapsed)
                refreshSession()
        }
    }

    function appendLog(text, isError) {
        logModel.append({text: text, isError: isError})
        if (logModel.count > 200) {
            logModel.remove(0, logModel.count - 200)
        }
        logList.positionViewAtEnd()
    }

    function refreshSession() {
        sessionModel.clear()
        var ks = Session.keys()
        for (var i = 0; i < ks.length; ++i) {
            sessionModel.append({ key: ks[i], value: Session.getRaw(ks[i]) })
        }
    }
}
