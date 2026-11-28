// Preview screen for StatusBarView (the redesigned status bar)
// (docs/project/STATUSBAR_HOWTO.md). Preview with:
//   ./src/views/tests/preview-statusbar.sh
// Pick a state and a window width to see how the bar behaves.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

Rectangle {
    id: root
    color: Config.get("color_bg_app") || "#f4f9f4"

    readonly property string mac: "98:D3:31:FD:12:AB"
    readonly property var states: [
        { label: "Connected", connected: true, name: "Pepe", mac: mac, battery: 82, transport: "bt", appId: "ZOWI_BASE_v2", saved: true },
        { label: "Connected (USB)", connected: true, name: "Pepe", mac: mac, battery: 82, transport: "usb", appId: "ZOWI_BASE_v2", saved: true },
        { label: "Low battery", connected: true, name: "Pepe", mac: mac, battery: 18, transport: "bt", appId: "ZOWI_BASE_v2", saved: true },
        { label: "Altered (custom FW)", connected: true, name: "Pepe", mac: mac, battery: 82, transport: "bt", appId: "ZOWI_CUSTOM_v1", saved: true },
        { label: "Low battery + custom FW", connected: true, name: "Pepe", mac: mac, battery: 18, transport: "bt", appId: "ZOWI_CUSTOM_v1", saved: true },
        { label: "Connecting", connecting: true, name: "Pepe", mac: mac, saved: true },
        { label: "Disconnected (saved)", name: "Pepe", mac: mac, saved: true },
        { label: "No registered Zowi" },
        { label: "Transport lost", transportLost: true, name: "Pepe", mac: mac, transport: "usb", saved: true },
        { label: "Demo", demo: true }
    ]

    property int current: 0
    property int barWidth: 0   // 0 = follow the window; otherwise a simulated width

    function pick(i) {
        current = i
        var s = states[i]
        bar.connected = !!s.connected
        bar.connecting = !!s.connecting
        bar.demo = !!s.demo
        bar.transportLost = !!s.transportLost
        bar.hasSaved = !!s.saved
        bar.name = s.name || ""
        bar.mac = s.mac || ""
        bar.battery = s.battery !== undefined ? s.battery : -1
        bar.transport = s.transport || "bt"
        bar.appId = s.appId || ""
        batterySlider.value = bar.battery >= 0 ? bar.battery : 80
    }
    // --step N (headless captures): N % 100 = state index, N / 100 = width index.
    Component.onCompleted: {
        var n = (typeof PreviewStep !== "undefined") ? PreviewStep : 0
        var widths = [0, 800, 640, 520, 420, 320]
        barWidth = widths[Math.min(5, Math.floor(n / 100))]
        pick(n % 100)
    }

    // The bar under test, at the simulated width.
    Item {
        id: stage
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: bar.height + 10

        StatusBarView {
            id: bar
            y: 5
            width: root.barWidth > 0 ? Math.min(root.barWidth, root.width) : root.width
        }
    }

    Column {
        anchors { top: stage.bottom; topMargin: 24; left: parent.left; right: parent.right; margins: 24 }
        spacing: 18

        Text {
            text: "Status bar — preview"
            font.pixelSize: 20
            font.bold: true
            color: Config.get("color_primary") || "#2d5a2d"
        }

        Text { text: "State"; font.pixelSize: 13; font.bold: true; color: "#444444" }
        Flow {
            width: parent.width
            spacing: 8
            Repeater {
                model: root.states
                delegate: Rectangle {
                    height: 30
                    radius: 15
                    width: lbl.implicitWidth + 24
                    color: root.current === index ? (Config.get("color_accent") || "#21a69b") : "#ffffff"
                    border.width: 1
                    border.color: Config.get("color_accent") || "#21a69b"
                    Text {
                        id: lbl
                        anchors.centerIn: parent
                        text: modelData.label
                        font.pixelSize: 12
                        color: root.current === index ? "#ffffff" : "#222222"
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.pick(index)
                    }
                }
            }
        }

        Text { text: "Window width (Auto follows the real window; resize it freely)"; font.pixelSize: 13; font.bold: true; color: "#444444" }
        Row {
            spacing: 8
            Repeater {
                model: [0, 800, 640, 520, 420, 320]
                delegate: Rectangle {
                    height: 30
                    radius: 15
                    width: 64
                    color: root.barWidth === modelData ? "#444444" : "#ffffff"
                    border.width: 1
                    border.color: "#444444"
                    Text {
                        anchors.centerIn: parent
                        text: modelData === 0 ? "Auto" : modelData
                        font.pixelSize: 12
                        color: root.barWidth === modelData ? "#ffffff" : "#222222"
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.barWidth = modelData
                    }
                }
            }
        }

        Text { text: "Battery level (connected states)"; font.pixelSize: 13; font.bold: true; color: "#444444" }
        Row {
            spacing: 12
            Slider {
                id: batterySlider
                width: 300
                from: 0
                to: 100
                stepSize: 1
                enabled: bar.connected
                onMoved: bar.battery = Math.round(value)
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: bar.connected ? Math.round(batterySlider.value) + "%  (low below " + bar.lowBatteryThreshold + "%)" : "—"
                font.pixelSize: 12
                color: "#444444"
            }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            color: "#666666"
            text: "Click the bar to open the detail panel. Resize the window (or pick a width) to see the chips drop from the right, firmware first, until only the face, name and state remain."
        }
    }
}
