// AchievementsScreen: the collection of achievements, grouped like the Android
// app (Movements / Animations / Games). Each row shows its badge (a padlock
// while locked), title, description and, while locked, how to unlock it. Three
// hidden corner areas (bottom-left, top-right, top-left, pressed in that order)
// are the original's easter egg that unlocks everything.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "AchievementsScreen"
    title: tr("title")
    subtitle: tr("subtitle").arg(Achievements.unlockedCount).arg(Achievements.total)
    showBackButton: true
    footerHeight: 0

    function tr(source) { return Translator.translate("AchievementsScreen.qml", source) }

    AchievementTexts { id: texts }

    // list() is read again whenever the unlocked set changes.
    property var entries: []
    function reload() { entries = Achievements.list() }
    Component.onCompleted: reload()
    Connections {
        target: Achievements
        function onAchievementsChanged() { root.reload() }
    }

    function sectionEntries(type) {
        var out = []
        for (var i = 0; i < entries.length; ++i)
            if (entries[i].type === type) out.push(entries[i])
        return out
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: column.height + 24
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: column
            width: Math.min(flick.width - 40, 760)
            x: (flick.width - width) / 2
            spacing: 18

            Repeater {
                model: ["movement", "animation", "game"]

                delegate: Rectangle {
                    id: section
                    required property string modelData
                    width: column.width
                    height: sectionCol.height + 24
                    radius: 16
                    color: "#ffffff"
                    border.color: Config.get("color_accent") || "#21a69b"
                    border.width: 1

                    Column {
                        id: sectionCol
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
                        spacing: 6

                        Text {
                            width: parent.width
                            text: root.tr("section_" + section.modelData)
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: 18
                            font.bold: true
                            color: Config.get("color_primary") || "#2d5a2d"
                        }

                        Repeater {
                            model: root.sectionEntries(section.modelData)

                            delegate: Item {
                                id: row
                                required property var modelData
                                width: sectionCol.width
                                height: 84

                                Image {
                                    id: badge
                                    anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 6 }
                                    width: 64
                                    height: 64
                                    source: texts.badge(row.modelData.id, row.modelData.unlocked)
                                    sourceSize.width: 168
                                    sourceSize.height: 168
                                    fillMode: Image.PreserveAspectFit
                                }
                                Column {
                                    anchors {
                                        left: badge.right; leftMargin: 14
                                        right: parent.right; rightMargin: 6
                                        verticalCenter: parent.verticalCenter
                                    }
                                    spacing: 2
                                    Text {
                                        width: parent.width
                                        text: texts.title(row.modelData.id)
                                        font.pixelSize: 16
                                        font.bold: true
                                        font.capitalization: Font.AllUppercase
                                        elide: Text.ElideRight
                                        color: row.modelData.unlocked ? (Config.get("color_primary") || "#2d5a2d") : "#8a8a8a"
                                    }
                                    Text {
                                        width: parent.width
                                        text: texts.description(row.modelData.id)
                                        font.pixelSize: 13
                                        elide: Text.ElideRight
                                        color: row.modelData.unlocked ? (Config.get("color_primary") || "#2d5a2d") : "#8a8a8a"
                                    }
                                    Text {
                                        width: parent.width
                                        visible: !row.modelData.unlocked
                                        text: texts.condition(row.modelData.id)
                                        font.pixelSize: 12
                                        font.italic: true
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                        color: "#8a8a8a"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Easter egg: three hidden corner areas, pressed in order ──────────
    // On the window corners (bottom-left, top-right, top-left), above everything
    // else. The overlay is parented to the screen itself (not to the clipped
    // content area) so the corner anchors are the real window corners. Does
    // nothing when the easter egg is switched off (`achievements_easter_egg`).
    Item {
        parent: root
        anchors.fill: parent
        z: 50
        visible: Achievements.easterEggEnabled

        MouseArea { width: 18; height: 18; anchors { left: parent.left; bottom: parent.bottom }
                    onPressed: Achievements.easterEggStep(1) }
        MouseArea { width: 18; height: 18; anchors { right: parent.right; top: parent.top }
                    onPressed: Achievements.easterEggStep(2) }
        MouseArea { width: 18; height: 18; anchors { left: parent.left; top: parent.top }
                    onPressed: Achievements.easterEggStep(3) }
    }
}
