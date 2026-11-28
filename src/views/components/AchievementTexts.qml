// AchievementTexts: the translated texts of the achievements (title,
// description, "how to unlock"), shared by the achievements screen, the unlock
// dialog and the locked-content hint. Non-visual: instantiate it where needed.
// Strings live in the "AchievementsScreen.qml" i18n context.
import QtQuick 2.15

QtObject {
    function tr(key) { return Translator.translate("AchievementsScreen.qml", key) }

    function title(id) { return tr("title_" + id) }

    function description(id) { return tr("desc_" + id) }

    // How to unlock it, spelling out the thresholds in force (they come from the
    // configuration, e.g. the days of use).
    function condition(id) {
        switch (id) {
        case "ascending_turn": return tr("cond_start")
        case "crusaito":       return tr("cond_second_launch")
        case "fart":           return tr("cond_days").arg(Achievements.daysOfUse)
        case "shake_leg":      return tr("cond_both")
        case "anxious":        return tr("cond_anxious").arg(Achievements.timelineChips)
        case "in_love":        return tr("cond_in_love").arg(Achievements.memoryScore)
        case "mouths_editor":  return tr("cond_mouths").arg(Achievements.mouthsLevel)
        default:
            // Projects award the rest: name the lesson that does.
            var project = Projects.projectTitleForAchievement(id)
            return project !== "" ? tr("cond_project").arg(project) : ""
        }
    }

    // Badge picture: the achievement's own, or the padlock one while locked.
    function badge(id, unlocked) {
        return unlocked ? "qrc:/images/android/" + id + "_badge.png"
                        : "qrc:/images/android/blocked_achievements_badge.png"
    }
}
