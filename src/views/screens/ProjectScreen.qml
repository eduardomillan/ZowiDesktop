import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

// Generic project screen: shared layout for all learning projects. Parametrised
// by projectId; content (page HTML, quiz, strings) is loaded per project from
// the Projects controller. Subclassing ScreenTemplate keeps the visual shell
// (status bar, header, back/right buttons, footer) identical across projects.
ScreenTemplate {
    id: projectScreen
    screenName: "ProjectScreen"
    showBackButton: true
    showRightButton: true
    rightButtonSource: doneIconSource
    onRightClicked: Projects.setCompleted(projectId, false)

    property string projectId: ""
    title: project.title || tr("title")
    subtitle: ""

    // Emitted when the (optional) third footer button is pressed. `target` is
    // the project's configured action_target (e.g. "gamepad"); main.qml maps
    // it to a concrete screen (pop to Home first, then push the destination).
    signal actionRequested(string target)

    function tr(source) { return Translator.translate("ProjectScreen.qml", source) }

    // Format milliseconds to mm:ss
    function formatCountdown(ms) {
        if (ms <= 0) return ""
        var totalSeconds = Math.floor(ms / 1000)
        var minutes = Math.floor(totalSeconds / 60)
        var seconds = totalSeconds % 60
        return (minutes < 10 ? "0" + minutes : minutes) + ":" + (seconds < 10 ? "0" + seconds : seconds)
    }

    // Project data from Projects controller
    readonly property var project: Projects.getProject(projectId)
    property bool completed: false
    property bool quizStarted: false
    property string contentHtml: ""
    property real thumbHeight: 120
    footerHeight: 88

    property string doneIconSource: completed
        ? "qrc:/images/android/project_done_icon.png"
        : "qrc:/images/android/project_not_done_outline_icon.png"

    function refreshCompleted() { completed = Projects.isCompleted(projectId) }

    function loadContentHtml() {
        contentHtml = Projects.loadHtml(projectId, Translator.currentLocale())
    }

    Component.onCompleted: {
        refreshCompleted()
        loadContentHtml()
    }
    Connections {
        target: Projects
        function onProjectsChanged() { projectScreen.refreshCompleted() }
    }
    Connections {
        target: Translator
        function onLanguageChanged() { projectScreen.loadContentHtml() }
    }

    function openLink() {
        Qt.openUrlExternally(project.url || tr("url"))
    }

    // Quiz finished handler
    function onQuizFinished(allCorrect) {
        quizStarted = false
        if (allCorrect) {
            Projects.setCompleted(projectId)
            msgBar.show(tr("quiz_passed"), Config.get("color_primary") || "#2d5a2d")
        } else {
            var remaining = Projects.getBlockadeRemainingMs(projectId)
            msgBar.show(tr("quiz_failed").arg(formatCountdown(remaining)), Config.get("color_warning") || "#e67e22")
        }
    }

    // Quiz blocked handler
    function onQuizBlocked(remainingMs) {
        msgBar.show(tr("quiz_blocked").arg(formatCountdown(remainingMs)), Config.get("color_warning") || "#e67e22")
    }

    Item {
        id: contentPanel
        anchors.fill: parent
        visible: !projectScreen.quizStarted

        Flickable {
            id: articleFlick
            width: parent.width * 0.9
            height: parent.height * 0.9
            anchors.centerIn: parent
            clip: true
            contentWidth: width
            contentHeight: articleCol.height
            boundsBehavior: Flickable.StopAtBounds

            Rectangle {
                visible: projectScreen.debugBorders
                anchors.fill: articleCol
                border.color: "lightgray"
                border.width: 1
                color: "transparent"
                radius: 4
                z: 1000
            }

            Column {
                id: articleCol
                width: articleFlick.width
                spacing: 16

                Image {
                    id: projectThumb
                    source: project && project.image ? project.image : ""
                    width: parent.width
                    height: projectScreen.thumbHeight
                    anchors.horizontalCenter: parent.horizontalCenter
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    id: articleText
                    width: parent.width
                    text: projectScreen.contentHtml
                    textFormat: Text.RichText
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    color: Config.get("color_primary") || "#2d5a2d"
                    font.pixelSize: 16
                    onLinkActivated: Qt.openUrlExternally(link)
                }
            }
        }
    }

    QuizComponent {
        id: quizComponent
        anchors.fill: parent
        visible: projectScreen.quizStarted
        projectId: projectScreen.projectId
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
                id: learnMoreButton
                implicitWidth: 200
                height: 56
                text: projectScreen.tr("learn_more")
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

            Button {
                id: testButton
                implicitWidth: 200
                height: 56
                text: projectScreen.tr("test")
                enabled: !projectScreen.quizStarted && Projects.isQuizEnabled()
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
                    projectScreen.quizStarted = true
                    quizComponent.startQuiz()
                }
            }

            // Optional third button: shown only when the project configures an
            // action_target (project.json); label comes from the project's
            // strings/<locale>.json ("action_label"). Routes through main.qml.
            Button {
                id: actionButton
                visible: project.actionTarget && project.actionTarget !== ""
                implicitWidth: 200
                height: 56
                text: project.actionLabel || project.actionTarget || ""
                background: Rectangle {
                    color: actionButton.pressed ? Config.get("color_primary_pressed") || "#1f4a1f" : Config.get("color_primary") || "#2d5a2d"
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
                onClicked: projectScreen.actionRequested(project.actionTarget)
            }
        }
    }

    MessageBar {
        id: msgBar
    }
}