import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: quizRoot

    property string projectId: ""
    property var questions: []  // array of {text: string, answers: [{text, correct}, ...]}

    signal finished(bool allCorrect)
    signal blocked(int remainingMs)

    function tr(source) { return Translator.translate("QuizComponent.qml", source) }

    property int currentQuestionIndex: 0
    property bool answered: false
    property bool allCorrect: true
    property int blockadeRemainingMsInternal: 0

    Timer {
        id: blockadeTimer
        interval: 1000
        repeat: true
        running: blockadeRemainingMsInternal > 0
        onTriggered: {
            blockadeRemainingMsInternal -= 1000
            if (blockadeRemainingMsInternal <= 0) {
                blockadeRemainingMsInternal = 0
            }
        }
    }

    function startQuiz() {
        answered = false
        allCorrect = true
        blockadeRemainingMsInternal = 0
        if (currentQuestionIndex !== 0)
            currentQuestionIndex = 0
        else
            populateAnswers()
    }

    function populateAnswers() {
        var children = answersColumn.children
        for (var c = children.length - 1; c >= 0; c--)
            children[c].destroy()
        if (currentQuestionIndex >= questions.length) return

        var q = questions[currentQuestionIndex]
        for (var i = 0; i < q.answers.length; i++) {
            var ans = q.answers[i]
            var btn = Qt.createQmlObject('import QtQuick 2.15; import QtQuick.Controls 2.15; Button { width: parent.width; height: 56; text: "' + ans.text + '"; font.pixelSize: 16; contentItem: Text { text: parent.text; color: "#ffffff"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter } background: Rectangle { radius: 28; color: parent.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : Config.get("color_accent") || "#21a69b" } }', answersColumn)
            if (btn) {
                btn.clicked.connect((function(idx) {
                    return function() { answerQuestion(idx) }
                })(i))
            }
        }
    }

    function answerQuestion(answerIndex) {
        if (answered) return
        answered = true

        var currentQ = questions[currentQuestionIndex]
        var selectedAnswer = currentQ.answers[answerIndex]
        var isCorrect = selectedAnswer.correct

        if (!isCorrect)
            allCorrect = false

        // Show result briefly then move to next or finish
        resultText.text = isCorrect ? tr("correct") : tr("incorrect")
        resultText.color = isCorrect ? Config.get("color_primary") || "#2d5a2d" : Config.get("color_error") || "#c0392b"
        resultText.visible = true

        nextTimer.restart()
    }

    function nextOrFinish() {
        resultText.visible = false
        answered = false

        if (currentQuestionIndex + 1 < questions.length) {
            currentQuestionIndex++
        } else {
            // Quiz finished
            if (allCorrect) {
                finished(true)
            } else {
                // Wrong answer: block quiz for configured duration
                var duration = Projects.getBlockadeDurationMs()
                Projects.blockQuiz(projectId, duration)
                blockadeRemainingMsInternal = duration
                blocked(duration)
                finished(false)
            }
        }
    }

    Timer {
        id: nextTimer
        interval: 1500
        repeat: false
        onTriggered: nextOrFinish()
    }

    // Computed: is quiz blocked?
    readonly property bool isBlocked: Projects.isQuizBlocked(projectId)

    // Computed: blockade remaining time (ms)
    readonly property int blockadeRemainingMs: Projects.getBlockadeRemainingMs(projectId)

    // Format ms to mm:ss
    function formatCountdown(ms) {
        if (ms <= 0) return ""
        var totalSeconds = Math.floor(ms / 1000)
        var minutes = Math.floor(totalSeconds / 60)
        var seconds = totalSeconds % 60
        return (minutes < 10 ? "0" + minutes : minutes) + ":" + (seconds < 10 ? "0" + seconds : seconds)
    }

    // Blockade countdown string mm:ss (binding, updates when internal or computed changes)
    property string blockadeCountdown: formatCountdown(blockadeRemainingMsInternal > 0 ? blockadeRemainingMsInternal : blockadeRemainingMs)

    Connections {
        target: Projects
        function onProjectsChanged() {
            // Re-evaluate blockade state
            if (isBlocked && blockadeRemainingMsInternal <= 0) {
                blockadeRemainingMsInternal = blockadeRemainingMs
            }
        }
    }

    // UI
    Column {
        anchors.centerIn: parent
        spacing: 20
        width: parent.width * 0.8

        // Question text
        Text {
            id: questionText
            width: parent.width
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 18
            color: Config.get("color_primary") || "#2d5a2d"
            text: currentQuestionIndex < questions.length ? questions[currentQuestionIndex].text : ""
        }

        // Result feedback
        Text {
            id: resultText
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 16
            visible: false
        }

        // Answers
        Column {
            id: answersColumn
            spacing: 12
            width: parent.width
        }

        // Blockade countdown (shown when blocked)
        Text {
            id: blockadeText
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 16
            color: Config.get("color_warning") || "#e67e22"
            visible: isBlocked
            text: tr("quiz_blocked").arg(blockadeCountdown)
        }

    }

    onCurrentQuestionIndexChanged: populateAnswers()
}
