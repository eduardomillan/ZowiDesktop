// RankingDialog: the single global ranking. Every game (Zowi Says/Memory,
// Mouths, Timeline) adds its best score to the active player's total.
// Players are "Player-NNN" (NNN = 100-999): the number is both the nickname
// and the id, so no free text ever enters the ranking.
//   showList()  -> top 10 (the active player is always shown, highlighted),
//                  with "New player" and "Switch player" actions.
// Rankings can't be deleted from the app (admin only, via zowi_cli).
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

Dialog {
    id: root

    property string mode: "list"      // "list" | "new" | "switch"
    property var entries: []          // top list
    property var allPlayers: []       // every local player (for switching)
    property int suggestion: 0        // free random number proposed to the user
    property string activeName: ""    // "Player-123" or "" when there is none yet
    readonly property int rowHeight: 44
    property alias numberText: numberField.text  // exposed for tests

    // State of the number being typed in the "new player" form.
    readonly property int typedNumber: numberField.text.length === 3 ? parseInt(numberField.text) : 0
    readonly property bool typedValid: typedNumber > 0 && Ranking.isValidNumber(typedNumber)
    readonly property bool typedTaken: typedValid && !Ranking.isNumberFree(typedNumber)

    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 380
    padding: 24

    function tr(source) { return Translator.translate("RankingDialog.qml", source) }

    function refresh() {
        entries = Ranking.top()
        allPlayers = Ranking.players()
        activeName = Ranking.activePlayerName()
    }

    function showList() {
        mode = "list"
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

    contentItem: Column {
        spacing: 16

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.mode === "new" ? root.tr("new_player")
                  : (root.mode === "switch" ? root.tr("switch_player") : root.tr("title"))
            font.pixelSize: 20
            font.bold: true
            color: Config.get("color_primary") || "#2d5a2d"
        }

        // ── Ranking list ────────────────────────────────────────────────
        Item {
            visible: root.mode === "list"
            width: parent.width
            height: Math.max(root.rowHeight * 2, Math.min(root.entries.length, 11) * (root.rowHeight + 4))

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
            visible: root.mode === "list" && root.activeName.length > 0
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.tr("playing_as").arg(root.activeName)
            font.pixelSize: 14
            color: Config.get("color_primary") || "#2d5a2d"
        }

        Text {
            visible: root.mode === "list"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: root.tr("how_points")
            font.pixelSize: 12
            color: Config.get("color_primary") || "#2d5a2d"
            opacity: 0.8
        }

        Row {
            visible: root.mode === "list"
            anchors.horizontalCenter: parent.horizontalCenter
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
        Column {
            visible: root.mode === "new"
            width: parent.width
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
        }

        // ── Switch player ───────────────────────────────────────────────
        ListView {
            visible: root.mode === "switch"
            width: parent.width
            height: Math.min(root.allPlayers.length, 6) * (root.rowHeight + 4)
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
            anchors.horizontalCenter: parent.horizontalCenter
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
