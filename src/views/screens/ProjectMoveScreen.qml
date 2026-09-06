import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: moveScreen
    screenName: "ProjectMoveScreen"
    showBackButton: true
    showRightButton: true
    rightButtonSource: doneIconSource
    onRightClicked: Projects.setCompleted("move", false)

    title: tr("title")
    subtitle: ""

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
    property bool completed: false
    property bool quizStarted: false
    footerHeight: 88

    property string doneIconSource: completed
        ? "qrc:/images/android/project_done_icon.png"
        : "qrc:/images/android/project_not_done_icon.png"

    function refreshCompleted() { completed = Projects.isCompleted("move") }

    Component.onCompleted: refreshCompleted()
    Connections {
        target: Projects
        function onProjectsChanged() { moveScreen.refreshCompleted() }
    }

    // External link handler
    function openLink() {
        Qt.openUrlExternally(project.url)
    }

    // Quiz finished handler
    function onQuizFinished(allCorrect) {
        quizStarted = false
        if (allCorrect) {
            Projects.setCompleted("move")
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
        visible: !moveScreen.quizStarted

        // Project image
        Rectangle {
            id: debugImage
            visible: moveScreen.debugBorders
            anchors.fill: projectImage
            border.color: "lightgray"
            border.width: 1
            color: "transparent"
            radius: 4
            z: 1000
        }
        Image {
            id: projectImage
            source: "qrc:/images/android/project_move_image.png"
            sourceSize.width: 300
            sourceSize.height: 300
            fillMode: Image.PreserveAspectFit
            anchors.horizontalCenter: parent.horizontalCenter
        }


        Text {
            id: rowTitle
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("title")
            font.pixelSize: 24
            font.bold: true
            color: Config.get("color_primary") || "#2d5a2d"
            horizontalAlignment: Text.AlignHCenter
            width: moveScreen.width * 0.8
        }

        // Learning description
        Rectangle {
            id: debugDesc
            visible: moveScreen.debugBorders
            anchors.fill: learningDesc
            border.color: "lightgray"
            border.width: 1
            color: "transparent"
            radius: 4
            z: 1000
        }
        Text {
            id: learningDesc
            text: tr("learning_description")
            font.pixelSize: 16
            color: Config.get("color_primary") || "#2d5a2d"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            width: moveScreen.width * 0.8
        }
    }

    QuizComponent {
        id: quizComponent
        anchors.fill: parent
        visible: moveScreen.quizStarted
        projectId: "move"
        questions: project.questions
        onFinished: onQuizFinished(allCorrect)
        onBlocked: onQuizBlocked(remainingMs)
    }

    footer: Item {
        anchors.fill: parent
        Row {
            anchors.centerIn: parent
            spacing: 20

            Button {
                id: testButton
                implicitWidth: 200
                height: 56
                text: moveScreen.tr("test")
                enabled: !moveScreen.quizStarted && Projects.isQuizEnabled()
                background: Rectangle {
                    color: testButton.pressed ? Config.get("color_warning_pressed") || "#d35400" : Config.get("color_warning") || "#e67e22"
                    radius: 28
                    opacity: testButton.enabled ? 1 : 0.5
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    if (quizComponent.isBlocked) {
                        onQuizBlocked(quizComponent.blockadeRemainingMs)
                        return
                    }
                    moveScreen.quizStarted = true
                    quizComponent.startQuiz()
                }
            }

            Button {
                id: learnMoreButton
                implicitWidth: 200
                height: 56
                text: moveScreen.tr("learn_more")
                background: Rectangle {
                    color: learnMoreButton.pressed ? Config.get("color_accent_pressed") || "#17736c" : Config.get("color_accent") || "#21a69b"
                    radius: 28
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: openLink()
            }
        }
    }

    MessageBar {
        id: msgBar
    }
}