import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

Item {
    id: quizRoot

    property string projectId: ""
    property var questions: []  // array of {text: string, answers: [{text, correct}, ...]}

    signal finished(bool allCorrect)
    signal blocked(int remainingMs)

    function tr(source) { return Translator.translate("QuizComponent.qml", source) }

    property int _currentQuestionIndex: 0
    property bool _answered: false
    property bool _allCorrect: true
    property int _blockadeRemainingMs: 0

    Timer {
        id: blockadeTimer
        interval: 1000
        repeat: true
        running: _blockadeRemainingMs > 0
        onTriggered: {
            _blockadeRemainingMs -= 1000
            if (_blockadeRemainingMs <= 0) {
                _blockadeRemainingMs = 0
            }
        }
    }

    function startQuiz() {
        _currentQuestionIndex = 0
        _answered = false
        _allCorrect = true
        _blockadeRemainingMs = 0
    }

    function answerQuestion(answerIndex) {
        if (_answered) return
        _answered = true

        var currentQ = questions[_currentQuestionIndex]
        var selectedAnswer = currentQ.answers[answerIndex]
        var isCorrect = selectedAnswer.correct

        if (!isCorrect)
            _allCorrect = false

        // Show result briefly then move to next or finish
        resultText.text = isCorrect ? tr("correct") : tr("incorrect")
        resultText.color = isCorrect ? Config.get("color_primary") || "#2d5a2d" : Config.get("color_error") || "#c0392b"
        resultText.visible = true

        nextTimer.restart()
    }

    function nextOrFinish() {
        resultText.visible = false
        _answered = false

        if (_currentQuestionIndex + 1 < questions.length) {
            _currentQuestionIndex++
        } else {
            // Quiz finished
            if (_allCorrect) {
                finished(true)
            } else {
                // Wrong answer: block quiz for configured duration
                var duration = Projects.getBlockadeDurationMs()
                Projects.blockQuiz(projectId, duration)
                _blockadeRemainingMs = duration
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

    // Blockade countdown string mm:ss (binding, updates when _blockadeRemainingMs or blockadeRemainingMs changes)
    property string blockadeCountdown: formatCountdown(_blockadeRemainingMs > 0 ? _blockadeRemainingMs : blockadeRemainingMs)

    Connections {
        target: Projects
        function onProjectsChanged() {
            // Re-evaluate blockade state
            if (isBlocked && _blockadeRemainingMs <= 0) {
                _blockadeRemainingMs = blockadeRemainingMs
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
            text: _currentQuestionIndex < questions.length ? questions[_currentQuestionIndex].text : ""
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

        // Run Test button (only shown when not in a question)
        Button {
            id: runTestButton
            width: parent.width * 0.5
            height: 48
            anchors.horizontalCenter: parent.horizontalCenter
            visible: questions.length > 0 && _currentQuestionIndex === 0 && !_answered && !isBlocked && Projects.isQuizEnabled()
            enabled: !isBlocked
            text: tr("run_test")
            background: Rectangle {
                radius: 24
                color: runTestButton.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : Config.get("color_accent") || "#21a69b"
            }
            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.bold: true
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: startQuiz()
        }
    }

    // Populate answers when question changes
    Component.onCompleted: {
        if (questions.length > 0) {
            // Will be triggered by startQuiz
        }
    }

    // Watch for question index changes
    on_currentQuestionIndexChanged: {
        answersColumn.children.forEach(function(child) { child.destroy() })
        if (_currentQuestionIndex >= questions.length) return

        var q = questions[_currentQuestionIndex]
        for (var i = 0; i < q.answers.length; i++) {
            var ans = q.answers[i]
            var btn = Qt.createQmlObject('import QtQuick 2.15; import QtQuick.Controls 2.15; Button { width: parent.width; height: 56; text: "' + ans.text + '"; font.pixelSize: 16; contentItem: Text { text: parent.text; color: "#ffffff"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter } background: Rectangle { radius: 28; color: parent.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : Config.get("color_accent") || "#21a69b" } }', answersColumn)
            if (btn) {
                btn.clicked.connect(function() { answerQuestion(i) })
            }
        }
    }
}