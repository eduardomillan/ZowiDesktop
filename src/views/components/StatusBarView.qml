// Status bar view: face icon + name/state on the left, chips on the right
// (battery, MAC, transport, firmware, Reconnect) and a detail panel on click.
// Pure presentation: it takes plain properties and knows nothing about `Robot`
// or `Session`; StatusBar.qml binds them. The preview screen
// (StatusBarPreviewScreen.qml) drives it directly to show every state.
// Design notes: docs/project/STATUSBAR_HOWTO.md.
import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    // Height from config.json (`status_bar_height`, default 64). Everything
    // inside is drawn for 48 px and scaled by `k`, so the contents follow it.
    readonly property int barHeight:
        Math.max(36, Math.min(96, parseInt(Config.get("status_bar_height")) || 64))
    readonly property real k: barHeight / 48
    height: barHeight

    function tr(source) { return Translator.translate("StatusBar.qml", source) }

    // ---- Inputs (mirror what Robot / Session expose) ----
    property bool connected: false
    property bool connecting: false
    property bool demo: false
    property bool transportLost: false
    property bool hasSaved: false
    property string name: ""
    property string mac: ""
    property int battery: -1
    property int lowBatteryThreshold: 50
    property string transport: "bt"      // "bt" | "usb"
    property string appId: ""
    readonly property string baseAppId: "ZOWI_BASE_v2"

    signal reconnectClicked()

    // The detail panel, exposed so previews/tests can open it.
    property alias detailsPopup: details

    // ---- Derived state ----
    // Same rule as RobotController::isBatteryLow().
    readonly property bool batteryLow: connected && battery >= 0 && battery < lowBatteryThreshold
    // Connected, but running a firmware that is not the base one.
    readonly property bool altered: connected && appId !== "" && appId !== baseAppId
    readonly property bool canReconnect: !connected && !connecting && !transportLost && hasSaved

    // Low battery wins over altered: it is the safety-relevant one.
    readonly property string face: demo ? "demo"
        : connecting ? "disconnected"
        : connected ? (batteryLow ? "low_battery" : (altered ? "altered" : "connected"))
        : "disconnected"

    readonly property color cBlue: Config.get("color_fg_connecting") || "#2980b9"
    readonly property color cError: Config.get("color_error") || "#c0392b"
    readonly property color cWarn: Config.get("color_warning") || "#e67e22"
    readonly property color cOk: Config.get("color_primary") || "#2d5a2d"
    readonly property color cGrey: "#616161"   // demo: the original grey face

    readonly property color fg: connecting ? cBlue
        : demo ? cOk
        : connected ? (batteryLow ? cError : (altered ? cWarn : cOk))
        : transportLost ? cError : cGrey   // disconnected: neutral, like its grey face
    readonly property color textColor: Qt.darker(fg, 1.25)

    color: connecting ? (Config.get("color_bg_connecting") || "#eaf2fb")
        : demo ? "transparent"   // demo: no background, outline face
        : connected ? (batteryLow ? (Config.get("color_bg_low_battery") || "#fdecea")
                                  : (altered ? (Config.get("color_bg_disconnected") || "#fff3e0")
                                             : (Config.get("color_bg_connected") || "#e8f5e8")))
        : transportLost ? (Config.get("color_bg_low_battery") || "#fdecea")
        : "#f0f1f2"   // disconnected / unregistered: neutral (orange is now only 'altered')

    readonly property string stateText: demo ? tr("status_demo")
        : connecting ? tr("status_connecting")
        : connected ? (batteryLow ? tr("status_low_battery") : (altered ? tr("status_custom_firmware") : tr("connected")))
        : transportLost ? tr("status_transport_lost").arg(transport === "usb" ? tr("via_usb") : tr("via_bluetooth"))
        : hasSaved ? tr("status_unavailable")
        : tr("not_connected")

    // ---- Responsive chip visibility ----
    // The whole group (face, name + state, chips) is centred in the bar. Chips
    // are kept by priority while the group fits: Reconnect, battery, MAC,
    // transport, firmware. Narrowing the window drops them from the right
    // (firmware first) until only the face, the name and the state are left.
    // Chip widths do not depend on visibility, so there are no binding loops.
    readonly property real sideMargin: Math.round(12 * k)
    readonly property real chipsGap: Math.round(14 * k)
    readonly property real faceSize: Math.round(32 * k)
    readonly property real textGap: Math.round(10 * k)
    readonly property real textNaturalWidth: Math.max(nameText.implicitWidth, stateLabel.implicitWidth)
    // Name + state never get wider than what is left next to the face.
    readonly property real textWidth: Math.min(textNaturalWidth, width - 2 * sideMargin - faceSize - textGap)
    readonly property real availableForChips:
        width - 2 * sideMargin - faceSize - textGap - textNaturalWidth - chipsGap
    readonly property real groupWidth: faceSize + textGap + textWidth
        + (chipsRow.width > 0 ? chipsGap + chipsRow.width : 0)
    readonly property real groupX: Math.max(sideMargin, (width - groupWidth) / 2)
    // Strict cut-off: once a chip does not fit, every lower-priority one is
    // hidden too (no lonely chip left after a bigger one disappears).
    readonly property bool showReconnect: reconnectBtn.wanted
        && reconnectBtn.width <= availableForChips
    readonly property bool cutAfterReconnect: reconnectBtn.wanted && !showReconnect
    readonly property real usedAfterReconnect: showReconnect ? reconnectBtn.width + 6 : 0
    readonly property bool showBattery: batteryChip.wanted && !cutAfterReconnect
        && usedAfterReconnect + batteryChip.width <= availableForChips
    readonly property bool cutAfterBattery: cutAfterReconnect || (batteryChip.wanted && !showBattery)
    readonly property real usedAfterBattery: usedAfterReconnect + (showBattery ? batteryChip.width + 6 : 0)
    readonly property bool showMac: macChip.wanted && !cutAfterBattery
        && usedAfterBattery + macChip.width <= availableForChips
    readonly property bool cutAfterMac: cutAfterBattery || (macChip.wanted && !showMac)
    readonly property real usedAfterMac: usedAfterBattery + (showMac ? macChip.width + 6 : 0)
    readonly property bool showTransport: transportChip.wanted && !cutAfterMac
        && usedAfterMac + transportChip.width <= availableForChips
    readonly property bool cutAfterTransport: cutAfterMac || (transportChip.wanted && !showTransport)
    readonly property real usedAfterTransport: usedAfterMac + (showTransport ? transportChip.width + 6 : 0)
    readonly property bool showFirmware: fwChip.wanted && !cutAfterTransport
        && usedAfterTransport + fwChip.width <= availableForChips

    // Click anywhere on the bar -> detail panel (declared first so the
    // Reconnect button, on top, keeps its own clicks).
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: details.opened ? details.close() : details.open()
    }

    // ---- Face + ring ----
    Item {
        id: faceBox
        x: root.groupX
        anchors.verticalCenter: parent.verticalCenter
        width: root.faceSize
        height: root.faceSize

        Image {
            visible: !root.demo
            anchors.fill: parent
            source: "qrc:/images/android/zowi_" + root.face + "_icon.png"
            sourceSize.width: 96
            sourceSize.height: 96
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
        }

        // Demo: the original face drawn as an outline (no fill, green lines).
        // Coordinates are those of the 224 px original icon.
        Canvas {
            id: demoFace
            visible: root.demo
            anchors.fill: parent
            property color stroke: root.cOk
            onStrokeChanged: requestPaint()
            onWidthChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                var k = width / 224
                ctx.scale(k, k)
                ctx.strokeStyle = stroke
                ctx.fillStyle = stroke
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                // outer circle
                ctx.lineWidth = 2.2 / k
                ctx.beginPath(); ctx.arc(112, 112, 112 - 1.1 / k, 0, 2 * Math.PI); ctx.stroke()
                // head + lower flap
                ctx.lineWidth = 1.7 / k
                function rrect(x, y, w, h, r) {
                    ctx.beginPath()
                    ctx.moveTo(x + r, y); ctx.lineTo(x + w - r, y)
                    ctx.quadraticCurveTo(x + w, y, x + w, y + r)
                    ctx.lineTo(x + w, y + h - r)
                    ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
                    ctx.lineTo(x + r, y + h)
                    ctx.quadraticCurveTo(x, y + h, x, y + h - r)
                    ctx.lineTo(x, y + r)
                    ctx.quadraticCurveTo(x, y, x + r, y)
                    ctx.closePath(); ctx.stroke()
                }
                rrect(60, 68, 103, 67, 8)
                rrect(60, 140, 103, 19, 6)
                // eyes
                ctx.beginPath(); ctx.arc(92, 111, 8, 0, 2 * Math.PI); ctx.fill()
                ctx.beginPath(); ctx.arc(132, 111, 8, 0, 2 * Math.PI); ctx.fill()
                // smile
                ctx.beginPath(); ctx.moveTo(101, 145); ctx.lineTo(112, 153); ctx.lineTo(123, 145); ctx.stroke()
            }
        }

        Rectangle {
            id: ring
            visible: root.connecting || root.transportLost
            anchors.centerIn: parent
            width: Math.round(root.faceSize * 1.25)
            height: width
            radius: width / 2
            color: "transparent"
            border.width: 2
            border.color: root.connecting ? root.cBlue : root.cError

            SequentialAnimation on opacity {
                running: root.connecting
                loops: Animation.Infinite
                NumberAnimation { to: 0.25; duration: 650; easing.type: Easing.InOutQuad }
                NumberAnimation { to: 1.0; duration: 650; easing.type: Easing.InOutQuad }
                onRunningChanged: if (!running) ring.opacity = 1.0
            }
        }
    }

    // ---- Name + state ----
    Column {
        anchors.verticalCenter: parent.verticalCenter
        x: faceBox.x + faceBox.width + root.textGap
        width: root.textWidth
        spacing: 1

        Text {
            id: nameText
            width: parent.width
            text: root.name !== "" ? root.name : "Zowi"
            font.pixelSize: Math.round(15 * root.k)
            font.bold: true
            color: root.textColor
            elide: Text.ElideRight
        }
        Text {
            id: stateLabel
            width: parent.width
            text: root.stateText
            font.pixelSize: Math.round(12 * root.k)
            color: root.textColor
            opacity: 0.85
            elide: Text.ElideRight
        }
    }

    // ---- Chips ----
    Row {
        id: chipsRow
        anchors.verticalCenter: parent.verticalCenter
        x: root.groupX + root.faceSize + root.textGap + root.textWidth + root.chipsGap
        spacing: Math.round(6 * root.k)

        // Battery: icon drawn in code (there is no original battery bitmap).
        Rectangle {
            id: batteryChip
            readonly property bool wanted: root.connected && root.battery >= 0
            readonly property color c: root.batteryLow ? root.cError : root.cOk
            visible: root.showBattery
            height: Math.round(24 * root.k)
            radius: height / 2
            width: batRow.implicitWidth + Math.round(18 * root.k)
            color: c          // red when low, green otherwise

            Row {
                id: batRow
                anchors.centerIn: parent
                spacing: Math.round(5 * root.k)

                Item {
                    width: Math.round(24 * root.k)
                    height: Math.round(12 * root.k)
                    anchors.verticalCenter: parent.verticalCenter

                  // The icon is drawn for 24x12 and scaled with the bar.
                  Item {
                    width: 24
                    height: 12
                    scale: root.k
                    transformOrigin: Item.TopLeft

                    Rectangle {
                        width: 21
                        height: 12
                        radius: 2
                        color: "transparent"
                        border.width: 1.5
                        border.color: "#ffffff"

                        Rectangle {
                            x: 2.5
                            y: 2.5
                            height: 7
                            width: Math.max(1, 15 * Math.max(0, Math.min(100, root.battery)) / 100)
                            radius: 1
                            color: "#ffffff"
                        }
                    }
                    Rectangle {
                        x: 21
                        y: 3.5
                        width: 2
                        height: 5
                        radius: 1
                        color: "#ffffff"
                    }
                  }
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.battery + "%"
                    font.pixelSize: Math.round(12 * root.k)
                    font.bold: true
                    color: "#ffffff"
                }
            }
        }

        Rectangle {
            id: transportChip
            readonly property bool wanted: root.connected
            visible: root.showTransport
            height: Math.round(24 * root.k)
            radius: height / 2
            width: trText.implicitWidth + Math.round(18 * root.k)
            // BT: yellow text on blue. USB: brown text on yellow.
            readonly property color yellow: Config.get("color_warning_text") || "#f1c40f"
            color: root.transport === "usb" ? yellow : root.cBlue
            border.width: 0

            Text {
                id: trText
                anchors.centerIn: parent
                text: root.transport === "usb" ? "USB" : "BT"
                font.pixelSize: Math.round(11 * root.k)
                font.bold: true
                color: root.transport === "usb" ? "#5d3a1a" : transportChip.yellow
            }
        }

        Rectangle {
            id: fwChip
            readonly property bool wanted: root.connected && root.appId !== ""
            // Highlighted when it is not the base firmware.
            readonly property color c: root.altered ? root.cWarn : (Config.get("color_firmware") || "#8a6d1f")
            visible: root.showFirmware
            height: Math.round(24 * root.k)
            radius: height / 2
            width: fwText.implicitWidth + Math.round(18 * root.k)
            color: root.altered ? "#fff3e0" : "#ffffff"
            border.width: root.altered ? 2 : 1
            border.color: c

            Text {
                id: fwText
                anchors.centerIn: parent
                text: root.appId
                font.pixelSize: Math.round(11 * root.k)
                font.bold: true
                color: Qt.darker(fwChip.c, 1.2)
            }
        }

        // MAC: always visible while known (decision 2 in the HOWTO).
        Rectangle {
            id: macChip
            readonly property bool wanted: root.mac !== "" && !root.demo
            visible: root.showMac
            height: Math.round(24 * root.k)
            radius: height / 2
            width: macText.implicitWidth + Math.round(18 * root.k)
            color: "#ffffff"
            border.width: 1
            border.color: root.cBlue

            Text {
                id: macText
                anchors.centerIn: parent
                text: root.mac
                font.pixelSize: Math.round(11 * root.k)
                font.bold: true
                color: root.cBlue
            }
        }

        Rectangle {
            id: reconnectBtn
            readonly property bool wanted: root.canReconnect
            visible: root.showReconnect
            height: Math.round(28 * root.k)
            radius: height / 2
            width: rcText.implicitWidth + Math.round(28 * root.k)
            color: rcArea.pressed ? Qt.darker("#ffffff", 1.08) : "#ffffff"
            border.width: 1.5
            border.color: root.cBlue

            Text {
                id: rcText
                anchors.centerIn: parent
                text: root.tr("status_reconnect")
                font.pixelSize: Math.round(12 * root.k)
                font.bold: true
                color: root.cBlue
            }
            MouseArea {
                id: rcArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.reconnectClicked()
            }
        }
    }

    // ---- Detail panel (in-window popup, no window-type popup: Wayland) ----
    Popup {
        id: details
        parent: root
        // Right under the bar, centred horizontally on it.
        x: Math.round((root.width - width) / 2)
        y: root.height + 2
        width: Math.min(340, root.width - 24)
        padding: 14
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: 10
            color: "#ffffff"
            border.width: 1
            border.color: root.fg
        }

        contentItem: Column {
            spacing: 6

            Text {
                text: (root.name !== "" ? root.name : "Zowi") + " — " + root.stateText
                font.pixelSize: 14
                font.bold: true
                color: root.textColor
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Rectangle {
                visible: root.canReconnect && !root.showReconnect
                height: 28
                radius: 14
                width: rc2.implicitWidth + 28
                color: "#ffffff"
                border.width: 1.5
                border.color: root.cBlue
                Text {
                    id: rc2
                    anchors.centerIn: parent
                    text: root.tr("status_reconnect")
                    font.pixelSize: 12
                    font.bold: true
                    color: root.cBlue
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { details.close(); root.reconnectClicked() }
                }
            }

            Repeater {
                model: [
                    [root.tr("detail_mac"), root.mac !== "" ? root.mac : "—"],
                    [root.tr("detail_transport"), root.connected
                        ? (root.transport === "usb" ? root.tr("via_usb") : root.tr("via_bluetooth")) : "—"],
                    [root.tr("detail_firmware"), root.appId !== ""
                        ? (root.altered ? root.tr("detail_firmware_custom") : root.tr("detail_firmware_base")).arg(root.appId)
                        : "—"],
                    [root.tr("detail_battery"), root.connected && root.battery >= 0
                        ? root.tr("detail_battery_value").arg(root.battery).arg(root.lowBatteryThreshold)
                        : "—"]
                ]
                delegate: Row {
                    spacing: 8
                    Text {
                        width: 110
                        text: modelData[0]
                        font.pixelSize: 12
                        color: "#666666"
                    }
                    Text {
                        width: 190
                        text: modelData[1]
                        font.pixelSize: 12
                        color: "#222222"
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
