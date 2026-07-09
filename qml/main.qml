import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

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

                var welcome = stack.replace("qrc:/qml/WelcomeScreen.qml")
                welcome.startWizard.connect(function() {
                    var scan = stack.push("qrc:/qml/ScanScreen.qml")
                    setupScanScreen(scan)
                })
                welcome.enterDemoMode.connect(function() {
                    paired = true
                })
            }
        }
    }
}
