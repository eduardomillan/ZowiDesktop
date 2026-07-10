import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Window 6.0

Window {
    visible: true
    width: 1024
    height: 600
    title: Translator.translate("main.qml", "Zowi Desktop")
    color: "#f4f9f4"

    property bool paired: false

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: SplashScreen {
            onSplashFinished: {
                function setupScanScreen(page) {
                    page.deviceSelected.connect(function(name, addr) {
                        Session.saveWizardDismissed(true)
                        Session.saveActiveZowiDeviceAddress(addr)
                        Session.saveActiveZowiName(name)
                        // TODO: navigate to mode selection screen
                    })
                    page.back.connect(function() {
                        stack.pop()
                    })
                }

                var welcome = stack.replace("qrc:/src/views/screens/WelcomeScreen.qml")
                welcome.startWizard.connect(function() {
                    var scan = stack.push("qrc:/src/views/screens/ScanScreen.qml")
                    setupScanScreen(scan)
                })
                welcome.enterDemoMode.connect(function() {
                    paired = true
                })
            }
        }
    }
}
