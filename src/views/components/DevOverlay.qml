import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// DevOverlay: a DEV panel that stays contained inside the main window.
// It is movable (drag via the "DEV" header) and resizable on all 4 sides +
// corners. Geometry is persisted in the session. Shown/hidden with Ctrl+D
// (Config.devOverlayVisible), toggled globally from main.qml.
Item {
    id: root
    visible: Config.devOverlayVisible

    readonly property int minHeight: 300
    readonly property int defaultPixelSize: 10
    readonly property int headPixelSize: 11
    readonly property int buttonPixelSize: 11
    readonly property int titlePixelSize: 12

    width: 320
    height: parent ? parent.height / 2 : minHeight
    x: 0
    y: 0

    // Default position: right side of the parent window.
    Component.onCompleted: {
        x = Math.max(0, parent.width - width - 8)
        y = 8
    }

    // Keep the panel contained within the parent window.
    function clamp() {
        if (!parent) return
        x = Math.max(0, Math.min(x, parent.width - width))
        y = Math.max(0, Math.min(y, parent.height - height))
        width = Math.max(200, Math.min(width, parent.width))
        height = Math.max(minHeight, Math.min(height, parent.height))
    }

    onXChanged: { clamp() }
    onYChanged: { clamp() }
    onWidthChanged: { clamp() }
    onHeightChanged: { clamp() }

    onVisibleChanged: if (visible) refreshSession()

    // ── Resize handles (edges + corners) ──
    // Position/drag math runs in the parent coordinate system so it does not
    // fight the window's own layout.
    MouseArea { // left edge
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 6
        cursorShape: Qt.SizeHorCursor
        property int startPX
        property int startLX
        property int startW
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPX = p.x; startW = root.width; startLX = root.x
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            var nw = Math.max(200, Math.min(startW + (startLX - p.x), root.parent.width))
            root.x = Math.max(0, startLX + (startW - nw))
            root.width = nw
        }
    }
    MouseArea { // right edge
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 6
        cursorShape: Qt.SizeHorCursor
        property int startPX
        property int startW
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPX = p.x; startW = root.width
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            root.width = Math.max(200, Math.min(startW + (p.x - startPX), root.parent.width - root.x))
        }
    }
    MouseArea { // top edge
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 6
        cursorShape: Qt.SizeVerCursor
        property int startPY
        property int startTY
        property int startH
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPY = p.y; startH = root.height; startTY = root.y
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            var nh = Math.max(120, Math.min(startH + (startTY - p.y), root.parent.height))
            root.y = Math.max(0, startTY + (startH - nh))
            root.height = nh
        }
    }
    MouseArea { // bottom edge
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 6
        cursorShape: Qt.SizeVerCursor
        property int startPY
        property int startH
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPY = p.y; startH = root.height
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            root.height = Math.max(120, Math.min(startH + (p.y - startPY), root.parent.height - root.y))
        }
    }
    MouseArea { // top-left corner
        anchors.left: parent.left
        anchors.top: parent.top
        width: 12
        height: 12
        cursorShape: Qt.SizeFDiagCursor
        property int startPX
        property int startPY
        property int startLX
        property int startTY
        property int startW
        property int startH
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPX = p.x; startPY = p.y
            startW = root.width; startH = root.height
            startLX = root.x; startTY = root.y
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            var nw = Math.max(200, Math.min(startW + (startLX - p.x), root.parent.width))
            var nh = Math.max(120, Math.min(startH + (startTY - p.y), root.parent.height))
            root.x = Math.max(0, startLX + (startW - nw))
            root.y = Math.max(0, startTY + (startH - nh))
            root.width = nw
            root.height = nh
        }
    }
    MouseArea { // top-right corner
        anchors.right: parent.right
        anchors.top: parent.top
        width: 12
        height: 12
        cursorShape: Qt.SizeBDiagCursor
        property int startPX
        property int startPY
        property int startTY
        property int startW
        property int startH
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPX = p.x; startPY = p.y
            startW = root.width; startH = root.height
            startTY = root.y
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            var nw = Math.max(200, Math.min(startW + (p.x - startPX), root.parent.width - root.x))
            var nh = Math.max(120, Math.min(startH + (startTY - p.y), root.parent.height))
            root.y = Math.max(0, startTY + (startH - nh))
            root.width = nw
            root.height = nh
        }
    }
    MouseArea { // bottom-left corner
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: 12
        height: 12
        cursorShape: Qt.SizeBDiagCursor
        property int startPX
        property int startPY
        property int startLX
        property int startW
        property int startH
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPX = p.x; startPY = p.y
            startW = root.width; startH = root.height
            startLX = root.x
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            var nw = Math.max(200, Math.min(startW + (startLX - p.x), root.parent.width))
            var nh = Math.max(120, Math.min(startH + (p.y - startPY), root.parent.height - root.y))
            root.x = Math.max(0, startLX + (startW - nw))
            root.width = nw
            root.height = nh
        }
    }
    MouseArea { // bottom-right corner
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 12
        height: 12
        cursorShape: Qt.SizeFDiagCursor
        property int startPX
        property int startPY
        property int startW
        property int startH
        onPressed: (mouse) => {
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            startPX = p.x; startPY = p.y
            startW = root.width; startH = root.height
        }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var p = mapToItem(root.parent, mouse.x, mouse.y)
            var nw = Math.max(200, Math.min(startW + (p.x - startPX), root.parent.width - root.x))
            var nh = Math.max(120, Math.min(startH + (p.y - startPY), root.parent.height - root.y))
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
        clip: true

        // Header: title + drag to move + close button. Fixed at the top so it
        // never shifts when the panel is resized.
        Column {
            id: header
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 6
            spacing: 0

            RowLayout {
                width: parent.width
                height: 18
                spacing: 6

                MouseArea {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    cursorShape: Qt.OpenHandCursor
                    drag.target: root
                    drag.axis: Drag.XAndYAxis
                    drag.threshold: 4
                    onPressed: cursorShape = Qt.ClosedHandCursor
                    onReleased: cursorShape = Qt.OpenHandCursor

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Debug"
                        color: "#e74c3c"
                        font.bold: true
                        font.pixelSize: root.titlePixelSize
                    }
                }

                Button {
                    width: 18
                    height: 18
                    text: "✕"
                    font.pixelSize: root.headPixelSize

                    contentItem: Text {
                        text: parent.text
                        color: "#ccc"
                        font.pixelSize: root.headPixelSize
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: parent.pressed ? "#666" : "#444"
                        radius: 3
                    }

                    onClicked: Config.devOverlayVisible = false
                }
            }
        }

        ColumnLayout {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                top: header.bottom
                topMargin: 2
                leftMargin: 6
                rightMargin: 6
                bottomMargin: 6
            }
            spacing: 4

            Text {
                Layout.fillWidth: true
                color: "#ddd"
                font.pixelSize: root.defaultPixelSize
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
                font.pixelSize: root.defaultPixelSize
                wrapMode: Text.WordWrap
                text: "Battery: " + (Robot.battery >= 0 ? Robot.battery + "%" : "N/A")
            }

            Text {
                Layout.fillWidth: true
                color: "#ddd"
                font.pixelSize: root.defaultPixelSize
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
                    font.pixelSize: root.headPixelSize
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    implicitWidth: 40
                    implicitHeight: 16
                    font.pixelSize: root.buttonPixelSize
                    text: "refresh"

                    contentItem: Text {
                        text: parent.text
                        color: "#ccc"
                        font.pixelSize: root.buttonPixelSize
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
                Layout.fillHeight: false
                Layout.preferredHeight: sessionCol.height
                Layout.minimumHeight: 40
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
                            font.pixelSize: root.defaultPixelSize
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
                    font.pixelSize: root.headPixelSize
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    implicitWidth: 40
                    implicitHeight: 16
                    font.pixelSize: root.buttonPixelSize
                    text: "copy"

                    contentItem: Text {
                        text: parent.text
                        color: "#ccc"
                        font.pixelSize: root.buttonPixelSize
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
                    font.pixelSize: root.buttonPixelSize
                    text: "clear"

                    contentItem: Text {
                        text: parent.text
                        color: "#ccc"
                        font.pixelSize: root.buttonPixelSize
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
                    font.pixelSize: root.defaultPixelSize
                    font.family: "monospace"
                    wrapMode: Text.WordWrap
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
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
            if (root.visible)
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
