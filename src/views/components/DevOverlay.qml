import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    visible: Config.devMode
    property bool collapsed: false
    width: 320
    height: Math.min(400, parent ? parent.height * 0.6 : 400)

    x: parent ? parent.width - width - 8 : 0
    y: parent ? parent.height * 0.15 : 8

    onCollapsedChanged: if (!collapsed) refreshSession()

    // ── Resize handles (only when expanded) ──
    // Resizing is computed in the parent coordinate system so the drag does not
    // fight the window's own size/position (the handle moves with the window,
    // which would otherwise make the local mouse delta unreliable).
    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 6
        cursorShape: Qt.SizeHorCursor
        enabled: !root.collapsed
        property int startPX
        property int startRX
        property int startW
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPX = p.x; startW = root.width; startRX = root.x + root.width
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            var nw = Math.max(200, Math.min(startW + (p.x - startPX),
                            root.parent ? root.parent.width - 16 : 2000))
            // Keep the right edge pinned under the cursor by sliding the left edge.
            root.x = Math.max(8, startRX - nw)
            root.width = nw
        }
    }
    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 6
        cursorShape: Qt.SizeVerCursor
        enabled: !root.collapsed
        property int startPY
        property int startBY
        property int startH
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPY = p.y; startH = root.height; startBY = root.y + root.height
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            var nh = Math.max(120, Math.min(startH + (p.y - startPY),
                            root.parent ? root.parent.height - 16 : 2000))
            root.y = Math.max(8, startBY - nh)
            root.height = nh
        }
    }
    MouseArea {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 12
        height: 12
        cursorShape: Qt.SizeFDiagCursor
        enabled: !root.collapsed
        property int startPX
        property int startPY
        property int startRX
        property int startBY
        property int startW
        property int startH
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPX = p.x; startPY = p.y
            startW = root.width; startH = root.height
            startRX = root.x + root.width; startBY = root.y + root.height
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            var nw = Math.max(200, Math.min(startW + (p.x - startPX),
                            root.parent ? root.parent.width - 16 : 2000))
            var nh = Math.max(120, Math.min(startH + (p.y - startPY),
                            root.parent ? root.parent.height - 16 : 2000))
            root.x = Math.max(8, startRX - nw)
            root.y = Math.max(8, startBY - nh)
            root.width = nw
            root.height = nh
        }
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
                    Layout.fillWidth: true
                    color: "#ddd"
                    font.pixelSize: 9
                    wrapMode: Text.WordWrap
                    text: {
                        var s = Robot.connected ? "● Connected" : "● Disconnected"
                        if (Robot.connected && Robot.deviceAddress)
                            s += "  " + Robot.deviceName + " (" + Robot.deviceAddress + ")"
                        return s
                    }
                }

                Text {
                    Layout.fillWidth: true
                    color: "#ddd"
                    font.pixelSize: 9
                    wrapMode: Text.WordWrap
                    text: "Battery: " + (Robot.battery >= 0 ? Robot.battery + "%" : "N/A")
                }

                Text {
                    Layout.fillWidth: true
                    color: "#ddd"
                    font.pixelSize: 9
                    wrapMode: Text.WordWrap
                    text: "Firmware (appId): " + (Robot.appId !== "" ? Robot.appId : "N/A")
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
                    Layout.preferredHeight: 100
                    Layout.minimumHeight: 40
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
                                width: sessionFlick.width
                                text: model.key + " = " + model.value
                                color: "#aaa"
                                font.pixelSize: 8
                                font.family: "monospace"
                                wrapMode: Text.WordWrap
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
                        text: "copy"

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

                        onClicked: {
                            var txt = ""
                            for (var i = 0; i < logModel.count; ++i)
                                txt += logModel.get(i).text + "\n"
                            Robot.copyText(txt)
                        }
                    }

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
                        width: logList.width
                        text: model.text
                        color: model.isError ? "#e74c3c" : "#aaa"
                        font.pixelSize: 8
                        font.family: "monospace"
                        wrapMode: Text.WordWrap
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                }
            }
        }
    }

    Connections {
        target: Robot

        function onDataReceived(data) {
            appendLog("> " + data, false)
        }

        function onErrorOccurred(message) {
            appendLog("! " + message, true)
        }

        function onConnectionChanged() {
            appendLog("~ " + (Robot.connected ? "connected" : "disconnected"), false)
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
