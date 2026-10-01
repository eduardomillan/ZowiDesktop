// RankingDialog: the single global ranking. Every game (Zowi Says/Memory,
// Mouths, Timeline) adds its best score to the active player's total.
// Players are "Player-NNN" (NNN = 100-999): the number is both the nickname
// and the id, so no free text ever enters the ranking.
//   showList()  -> top 10 (the active player is always shown, highlighted),
//                  with "New player" and "Switch player" actions.
// Rankings can't be deleted from the app (admin only, via zowi_cli).
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Dialog {
    id: root

    property string mode: "list"      // "list" | "new" | "switch"
    property string tab: "local"      // "local" | "world" (only when the world ranking is available)
    readonly property bool worldTab: mode === "list" && OnlineRanking.available && tab === "world"
    property var entries: []          // top list
    property var allPlayers: []       // every local player (for switching)
    property int suggestion: 0        // free random number proposed to the user
    property string activeName: ""    // "Player-123" or "" when there is none yet
    readonly property int rowHeight: 44

    // Size: 40 % of the app window (width), and a height that follows the content
    // with a minimum of the same share of the window height. Change `sizeRatio`
    // here (or when instantiating the dialog) to resize it; it is clamped to
    // [0.2, maxRatio] and the dialog never grows past maxRatio of the window.
    property real sizeRatio: 0.4
    readonly property real maxRatio: 0.9
    readonly property int minWidth: 380
    readonly property real ratio: Math.min(maxRatio, Math.max(0.2, sizeRatio))
    readonly property real windowWidth: parent ? parent.width : 800
    readonly property real windowHeight: parent ? parent.height : 600
    property alias numberText: numberField.text  // exposed for tests

    // State of the number being typed in the "new player" form.
    readonly property int typedNumber: numberField.text.length === 3 ? parseInt(numberField.text) : 0
    readonly property bool typedValid: typedNumber > 0 && Ranking.isValidNumber(typedNumber)
    readonly property bool typedTaken: typedValid && !Ranking.isNumberFree(typedNumber)

    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    padding: 24
    width: Math.min(maxRatio * windowWidth, Math.max(minWidth, ratio * windowWidth))
    // implicitHeight = what the content asks for (list rows, buttons...).
    height: Math.min(maxRatio * windowHeight, Math.max(ratio * windowHeight, implicitHeight))

    function tr(source) { return Translator.translate("RankingDialog.qml", source) }

    function refresh() {
        entries = Ranking.top()
        allPlayers = Ranking.players()
        activeName = Ranking.activePlayerName()
    }

    function showList() {
        mode = "list"
        tab = "local"
        refresh()
        open()
    }

    function startNewPlayer() {
        suggestion = Ranking.suggestNumber()
        numberField.text = suggestion > 0 ? String(suggestion) : ""
        mode = "new"
    }

    function saveNewPlayer() {
        if (!typedValid || typedTaken) return
        if (Ranking.createPlayer(typedNumber) === 0) {  // 0 = created
            Ranking.setActivePlayer(typedNumber)
            mode = "list"
            refresh()
        }
    }

    function useSuggestion() {
        suggestion = Ranking.suggestNumber()
        numberField.text = suggestion > 0 ? String(suggestion) : ""
    }

    function selectPlayer(number) {
        if (Ranking.setActivePlayer(number)) {
            mode = "list"
            refresh()
        }
    }

    background: Rectangle {
        radius: 20
        color: "#ffffff"
        border.color: Config.get("color_accent") || "#21a69b"
        border.width: 2
    }

    contentItem: ColumnLayout {
        spacing: 16

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.mode === "new" ? root.tr("new_player")
                  : (root.mode === "switch" ? root.tr("switch_player")
                     : (root.worldTab ? root.tr("world_title") : root.tr("title")))
            font.pixelSize: 20
            font.bold: true
            color: Config.get("color_primary") || "#2d5a2d"
        }

        // ── Tabs: Local / World (only when the world ranking is available) ──
        Row {
            id: tabRow
            visible: root.mode === "list" && OnlineRanking.available
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            Repeater {
                model: [ { key: "local", label: root.tr("tab_local") },
                         { key: "world", label: root.tr("tab_world") } ]

                delegate: Rectangle {
                    width: 120
                    height: 34
                    radius: 17
                    color: root.tab === modelData.key ? (Config.get("color_accent") || "#21a69b") : "transparent"
                    border.color: Config.get("color_accent") || "#21a69b"
                    border.width: 2

                    Text {
                        anchors.centerIn: parent
                        text: modelData.label
                        font.pixelSize: 14
                        font.bold: true
                        color: root.tab === modelData.key ? "#ffffff" : (Config.get("color_primary") || "#2d5a2d")
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.tab = modelData.key
                            if (modelData.key === "world") OnlineRanking.refresh()
                        }
                    }
                }
            }
        }

        // ── World ranking (read-only list) ──────────────────────────────
        Item {
            visible: root.worldTab
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.max(root.rowHeight * 4, Math.min(OnlineRanking.world.length, 8) * (root.rowHeight + 4))
            Layout.minimumHeight: root.rowHeight * 3

            Column {
                anchors.centerIn: parent
                width: parent.width
                spacing: 10
                visible: OnlineRanking.worldState !== 2 || OnlineRanking.world.length === 0

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    font.pixelSize: 14
                    color: OnlineRanking.worldState === 3 ? "#d32f2f" : (Config.get("color_primary") || "#2d5a2d")
                    text: OnlineRanking.worldState === 1 ? root.tr("world_loading")
                          : (OnlineRanking.worldState === 3 ? root.tr("world_error")
                             : (OnlineRanking.worldState === 2 ? root.tr("world_empty") : ""))
                }

                Button {
                    id: retryBtn
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: OnlineRanking.worldState === 3
                    implicitWidth: 150
                    implicitHeight: 40
                    text: root.tr("world_retry")
                    contentItem: Text {
                        text: retryBtn.text
                        color: "#ffffff"
                        font.bold: true
                        font.pixelSize: 15
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 20
                        color: retryBtn.pressed ? Config.get("color_accent_pressed") || "#17736c"
                                                : (Config.get("color_accent") || "#21a69b")
                    }
                    onClicked: OnlineRanking.refresh()
                }
            }

            ListView {
                anchors.fill: parent
                visible: OnlineRanking.worldState === 2 && OnlineRanking.world.length > 0
                clip: true
                spacing: 4
                model: OnlineRanking.world
                interactive: contentHeight > height

                delegate: Rectangle {
                    width: ListView.view.width
                    height: root.rowHeight
                    radius: 10
                    color: modelData.own ? (Config.get("color_bg_hover") || "#e0f0e0") : "transparent"

                    Text {
                        id: worldPos
                        anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                        width: 34
                        text: modelData.position
                        font.pixelSize: 14
                        font.bold: true
                        color: Config.get("color_accent") || "#21a69b"
                    }
                    Text {
                        anchors {
                            left: worldPos.right; leftMargin: 6
                            right: worldPoints.left; rightMargin: 8
                            verticalCenter: parent.verticalCenter
                        }
                        elide: Text.ElideRight
                        text: modelData.name
                        font.pixelSize: 16
                        font.bold: modelData.own
                        color: Config.get("color_primary") || "#2d5a2d"
                    }
                    Text {
                        id: worldPoints
                        anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                        text: root.tr("points").arg(modelData.total)
                        font.pixelSize: 16
                        font.bold: true
                        color: Config.get("color_accent") || "#21a69b"
                    }
                }
            }
        }

        Text {
            visible: root.worldTab && OnlineRanking.worldState === 2 && OnlineRanking.generated.length > 0
            Layout.alignment: Qt.AlignHCenter
            text: root.tr("world_updated").arg(OnlineRanking.generated)
            font.pixelSize: 12
            color: Config.get("color_primary") || "#2d5a2d"
            opacity: 0.7
        }

        // ── Ranking list ────────────────────────────────────────────────
        Item {
            visible: root.mode === "list" && !root.worldTab
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.max(root.rowHeight * 2, Math.min(root.entries.length, 11) * (root.rowHeight + 4))
            Layout.minimumHeight: root.rowHeight * 2

            Text {
                anchors.centerIn: parent
                width: parent.width
                visible: root.entries.length === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: root.tr("empty")
                font.pixelSize: 14
                color: Config.get("color_primary") || "#2d5a2d"
            }

            ListView {
                anchors.fill: parent
                clip: true
                spacing: 4
                model: root.entries
                interactive: contentHeight > height

                delegate: Rectangle {
                    width: ListView.view.width
                    height: root.rowHeight
                    radius: 10
                    color: modelData.active ? (Config.get("color_bg_hover") || "#e0f0e0") : "transparent"

                    Item {
                        id: badge
                        anchors { left: parent.left; leftMargin: 8; verticalCenter: parent.verticalCenter }
                        width: 36
                        height: 36

                        Image {
                            anchors.fill: parent
                            source: "qrc:/images/android/ranking_icon.png"
                            sourceSize.width: 36
                            sourceSize.height: 36
                            fillMode: Image.PreserveAspectFit
                        }
                        Text {
                            anchors.centerIn: parent
                            text: modelData.position
                            font.pixelSize: 14
                            font.bold: true
                            color: "#ffffff"
                        }
                    }

                    Text {
                        anchors {
                            left: badge.right; leftMargin: 10
                            right: pointsText.left; rightMargin: 8
                            verticalCenter: parent.verticalCenter
                        }
                        elide: Text.ElideRight
                        text: modelData.name
                        font.pixelSize: 16
                        font.bold: modelData.active
                        color: Config.get("color_primary") || "#2d5a2d"
                    }

                    Text {
                        id: pointsText
                        anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                        text: root.tr("points").arg(modelData.total)
                        font.pixelSize: 16
                        font.bold: true
                        color: Config.get("color_accent") || "#21a69b"
                    }
                }
            }
        }

        Text {
            visible: root.mode === "list" && !root.worldTab && root.activeName.length > 0
            Layout.alignment: Qt.AlignHCenter
            text: root.tr("playing_as").arg(root.activeName)
            font.pixelSize: 14
            color: Config.get("color_primary") || "#2d5a2d"
        }

        Text {
            visible: root.mode === "list" && !root.worldTab
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: root.tr("how_points")
            font.pixelSize: 12
            color: Config.get("color_primary") || "#2d5a2d"
            opacity: 0.8
        }

        Row {
            visible: root.mode === "list" && !root.worldTab
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            Button {
                id: newBtn
                implicitWidth: root.allPlayers.length > 1 ? 150 : 200
                implicitHeight: 44
                text: root.tr("new_player")
                contentItem: Text {
                    text: newBtn.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 22
                    color: newBtn.pressed ? Config.get("color_accent_pressed") || "#17736c"
                                          : (Config.get("color_accent") || "#21a69b")
                }
                onClicked: root.startNewPlayer()
            }

            Button {
                id: switchBtn
                visible: root.allPlayers.length > 1
                implicitWidth: 150
                implicitHeight: 44
                text: root.tr("switch_player")
                contentItem: Text {
                    text: switchBtn.text
                    color: Config.get("color_primary") || "#2d5a2d"
                    font.bold: true
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 22
                    color: switchBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
                    border.color: Config.get("color_accent") || "#21a69b"
                    border.width: 2
                }
                onClicked: root.mode = "switch"
            }
        }

        // ── New player: choose a number ─────────────────────────────────
        Item {
            visible: root.mode === "new"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: newForm.implicitHeight
            Layout.minimumHeight: newForm.implicitHeight

            Column {
                id: newForm
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: root.tr("number_hint")
                font.pixelSize: 14
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Player-"
                    font.pixelSize: 22
                    font.bold: true
                    color: Config.get("color_primary") || "#2d5a2d"
                }

                TextField {
                    id: numberField
                    width: 90
                    maximumLength: 3
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 0; top: 999 }
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 22
                    font.bold: true
                    onAccepted: root.saveNewPlayer()

                    background: Rectangle {
                        radius: 12
                        color: "#ffffff"
                        border.color: root.typedTaken || (numberField.text.length > 0 && !root.typedValid)
                                      ? "#d32f2f" : (Config.get("color_accent") || "#21a69b")
                        border.width: 2
                    }
                }
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: text.length > 0
                font.pixelSize: 13
                color: "#d32f2f"
                text: root.typedTaken ? root.tr("number_taken").arg(root.suggestion)
                      : (numberField.text.length > 0 && !root.typedValid ? root.tr("number_invalid") : "")
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 12

                Button {
                    id: otherBtn
                    implicitWidth: 150
                    implicitHeight: 44
                    text: root.tr("other_number")
                    contentItem: Text {
                        text: otherBtn.text
                        color: Config.get("color_primary") || "#2d5a2d"
                        font.bold: true
                        font.pixelSize: 15
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 22
                        color: otherBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
                        border.color: Config.get("color_accent") || "#21a69b"
                        border.width: 2
                    }
                    onClicked: root.useSuggestion()
                }

                Button {
                    id: saveBtn
                    implicitWidth: 150
                    implicitHeight: 44
                    enabled: root.typedValid && !root.typedTaken
                    opacity: enabled ? 1.0 : 0.4
                    text: root.tr("save")
                    contentItem: Text {
                        text: saveBtn.text
                        color: "#ffffff"
                        font.bold: true
                        font.pixelSize: 15
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 22
                        color: saveBtn.pressed ? Config.get("color_accent_pressed") || "#17736c"
                                               : (Config.get("color_accent") || "#21a69b")
                    }
                    onClicked: root.saveNewPlayer()
                }
            }
            }  // newForm Column
        }

        // ── Switch player ───────────────────────────────────────────────
        ListView {
            visible: root.mode === "switch"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.min(root.allPlayers.length, 6) * (root.rowHeight + 4)
            Layout.minimumHeight: Math.min(root.allPlayers.length, 2) * (root.rowHeight + 4)
            clip: true
            spacing: 4
            model: root.allPlayers
            interactive: contentHeight > height

            delegate: Rectangle {
                width: ListView.view.width
                height: root.rowHeight
                radius: 10
                color: modelData.active ? (Config.get("color_bg_hover") || "#e0f0e0") : "transparent"
                border.color: Config.get("color_accent") || "#21a69b"
                border.width: 1

                Text {
                    anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                    text: modelData.name
                    font.pixelSize: 16
                    font.bold: modelData.active
                    color: Config.get("color_primary") || "#2d5a2d"
                }
                Text {
                    anchors { right: parent.right; rightMargin: 14; verticalCenter: parent.verticalCenter }
                    text: root.tr("points").arg(modelData.total)
                    font.pixelSize: 15
                    color: Config.get("color_accent") || "#21a69b"
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.selectPlayer(modelData.number)
                }
            }
        }

        Button {
            id: closeBtn
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 160
            implicitHeight: 44
            text: root.mode === "list" ? root.tr("close") : root.tr("back")

            contentItem: Text {
                text: closeBtn.text
                color: Config.get("color_primary") || "#2d5a2d"
                font.bold: true
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 22
                color: closeBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
                border.color: Config.get("color_accent") || "#21a69b"
                border.width: 2
            }

            onClicked: {
                if (root.mode === "list") root.close()
                else root.mode = "list"
            }
        }
    }
}
