import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Window 6.0
import "screens"

Window {
    visible: true
    width: 1024
    height: 600
    title: "Zowi Desktop - " + (stack.currentItem && stack.currentItem.screenName ? stack.currentItem.screenName : "")
    color: "#f4f9f4"

    property bool paired: false

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: SplashScreen {
            onSplashFinished: {
                var start = stack.replace("qrc:/src/views/screens/StartScreen.qml")
                start.startWizard.connect(function() {
                    var welcome = stack.push("qrc:/src/views/screens/WelcomeScreen.qml")
                    welcome.startClicked.connect(function() {
                        var scan = stack.push("qrc:/src/views/screens/ScanScreen.qml")
                        scan.back.connect(function() {
                            stack.pop()
                        })
                    })
                    welcome.goBack.connect(function() {
                        stack.pop()
                    })
                })
                start.knowMoreClicked.connect(function() {
                    Qt.openUrlExternally(Config.knowMoreUrl)
                })
            }
            onQuitRequested: Qt.quit()
        }
    }
}
