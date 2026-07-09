import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

Window {
    visible: true
    width: 1024
    height: 600
    title: Translator.translate("main.qml", "Zowi Desktop")
    color: "#702076"

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: SplashScreen {
            onSplashFinished: {
                var wiz = Session.hasDismissedWizard()
                var addr = Session.loadActiveZowiDeviceAddress()
                if (wiz && addr !== "") {
                    // Already paired — go directly to main mode
                    stack.replace("qrc:/qml/WelcomeScreen.qml",
                                  { skipToDemo: true })
                } else {
                    stack.replace("qrc:/qml/WelcomeScreen.qml")
                }
            }
        }
    }
}
