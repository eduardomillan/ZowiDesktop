import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: moveScreen
    screenName: "ProjectMoveScreen"
    showBackButton: true

    title: tr("title")
    subtitle: tr("learning_description")

    function tr(source) { return Translator.translate("ProjectMoveScreen.qml", source) }

    // Format milliseconds to mm:ss
    function formatCountdown(ms) {
        if (ms <= 0) return ""
        var totalSeconds = Math.floor(ms / 1000)
        var minutes = Math.floor(totalSeconds / 60)
        var seconds = totalSeconds % 60
        return (minutes < 10 ? "0" + minutes : minutes) + ":" + (seconds < 10 ? "0" + seconds : seconds)
    }

    // Project data from Projects controller
    readonly property var project: Projects.getProject("move")
    readonly property bool completed: Projects.isCompleted("move")

    // Done icon
    property string doneIconSource: completed
        ? "qrc:/images/android/project_done_icon.png"
        : "qrc:/images/android/project_not_done_icon.png"

    // External link handler
    function openLink() {
        Qt.openUrlExternally(project.url)
    }

    // Quiz finished handler
    function onQuizFinished(allCorrect) {
        if (allCorrect) {
            msgBar.show(tr("quiz_passed"), Config.get("color_primary") || "#2d5a2d")
        } else {
            var remaining = Projects.getBlockadeRemainingMs("move")
            msgBar.show(tr("quiz_failed").arg(formatCountdown(remaining)), Config.get("color_warning") || "#e67e22")
        }
    }

    // Quiz blocked handler
    function onQuizBlocked(remainingMs) {
        msgBar.show(tr("quiz_blocked").arg(formatCountdown(remainingMs)), Config.get("color_warning") || "#e67e22")
    }

    // Content area
    Column {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 30
        spacing: 20

        // Project image
        Image {
            id: projectImage
            source: "qrc:/images/android/project_move_image.png"
            sourceSize.width: 300
            sourceSize.height: 300
            fillMode: Image.PreserveAspectFit
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // Done icon + title
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12

            Image {
                source: doneIconSource
                sourceSize.width: 32
                sourceSize.height: 32
                fillMode: Image.PreserveAspectFit
            }

            Text {
                text: tr("title")
                font.pixelSize: 24
                font.bold: true
                color: Config.get("color_primary") || "#2d5a2d"
                verticalAlignment: Text.AlignVCenter
            }
        }

        // Learning description
        Text {
            text: tr("learning_description")
            font.pixelSize: 16
            color: Config.get("color_primary") || "#2d5a2d"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            width: parent.width
        }

        // Project link button
        Button {
            id: linkButton
            width: parent.width * 0.6
            height: 44
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("project_link")
            background: Rectangle {
                radius: 22
                color: linkButton.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
                border.color: Config.get("color_accent") || "#21a69b"
                border.width: 2
            }
            contentItem: Text {
                text: parent.text
                color: Config.get("color_accent") || "#21a69b"
                font.bold: true
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: openLink()
        }

        // Quiz component
        QuizComponent {
            id: quizComponent
            projectId: "move"
            questions: project.questions
            onFinished: onQuizFinished(allCorrect)
            onBlocked: onQuizBlocked(remainingMs)
        }

        // Footer spacer
        Item { height: 20 }
    }

    // Message bar for feedback
    MessageBar {
        id: msgBar
    }
}