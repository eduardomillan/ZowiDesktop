// RankingDialog: shared top-10 ranking for the games (Zowi Says/Memory, Mouths,
// Timeline). Two modes, like Android's MakerBoxDialogRanking flow:
//   showScore(score)  -> if the score qualifies, ask for a nickname and save it,
//                        then show the list with the new entry highlighted;
//   showList()        -> just the list (corner ranking button).
// `game` is the Ranking controller id: "zowi_says" | "mouths" | "timeline".
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

Dialog {
    id: root
    property string game: ""

    property bool entering: false
    property int pendingScore: 0
    property int highlightPosition: 0
    property var entries: []

    readonly property int maxNameLength: 12
    readonly property int rowHeight: 44

    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 380
    padding: 24

    function tr(source) { return Translator.translate("RankingDialog.qml", source) }

    function refresh() { entries = Ranking.top(game) }

    function showList(highlight) {
        entering = false
        highlightPosition = highlight || 0
        refresh()
        open()
    }

    function showScore(score) {
        if (!Ranking.qualifies(game, score)) {
            showList(0)
            return
        }
        pendingScore = score
        entering = true
        highlightPosition = 0
        nameField.text = Session.getString("ranking_last_name", "")
        refresh()
        open()
    }

    function saveEntry() {
        var name = nameField.text.trim()
        Session.saveString("ranking_last_name", name)
        var position = Ranking.submit(game, name, pendingScore)
        entering = false
        highlightPosition = position
        refresh()
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
            text: root.tr("title")
            font.pixelSize: 20
            font.bold: true
            color: Config.get("color_primary") || "#2d5a2d"
        }

        // ── Name entry (score qualifies) ────────────────────────────────
        Column {
            visible: root.entering
            width: parent.width
            spacing: 12

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.tr("points_earned").arg(root.pendingScore)
                font.pixelSize: 22
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: root.tr("enter_name")
                font.pixelSize: 14
                color: Config.get("color_primary") || "#2d5a2d"
            }

            TextField {
                id: nameField
                width: parent.width
                maximumLength: root.maxNameLength
                horizontalAlignment: Text.AlignHCenter
                placeholderText: root.tr("name_placeholder")
                font.pixelSize: 16
                onAccepted: root.saveEntry()

                background: Rectangle {
                    radius: 12
                    color: "#ffffff"
                    border.color: Config.get("color_accent") || "#21a69b"
                    border.width: 2
                }
            }

            Button {
                id: saveBtn
                anchors.horizontalCenter: parent.horizontalCenter
                implicitWidth: 160
                implicitHeight: 44
                text: root.tr("save")

                contentItem: Text {
                    text: saveBtn.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 22
                    color: saveBtn.pressed ? Config.get("color_accent_pressed") || "#17736c"
                                           : (Config.get("color_accent") || "#21a69b")
                }

                onClicked: root.saveEntry()
            }
        }

        // ── Ranking list ────────────────────────────────────────────────
        Item {
            visible: !root.entering
            width: parent.width
            height: Math.max(root.rowHeight * 2, Math.min(root.entries.length, 10) * (root.rowHeight + 4))

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
                    color: modelData.position === root.highlightPosition
                           ? (Config.get("color_bg_hover") || "#e0f0e0") : "transparent"

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
                        text: modelData.playerName
                        font.pixelSize: 16
                        font.bold: modelData.position === root.highlightPosition
                        color: Config.get("color_primary") || "#2d5a2d"
                    }

                    Text {
                        id: pointsText
                        anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                        text: root.tr("points").arg(modelData.points)
                        font.pixelSize: 16
                        font.bold: true
                        color: Config.get("color_accent") || "#21a69b"
                    }
                }
            }
        }

        Button {
            id: closeBtn
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: 160
            implicitHeight: 44
            text: root.tr("close")

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

            onClicked: root.close()
        }
    }
}
