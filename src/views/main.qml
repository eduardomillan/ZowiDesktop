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

    function connectHome(home) {
        home.settingsClicked.connect(function() {
            console.log("Home: settings")
        })
        home.achievementsClicked.connect(function() {
            console.log("Home: achievements")
        })
        // DEV: temporary navigation
        home.goSplash.connect(function() {
            Session.saveWizardDismissed(false)
            stack.replace("qrc:/src/views/screens/SplashScreen.qml")
            var splash = stack.currentItem
            connectSplash(splash)
        })
        home.goWelcome.connect(function() {
            Session.saveWizardDismissed(false)
            stack.replace("qrc:/src/views/screens/WelcomeScreen.qml")
            var welcome = stack.currentItem
            connectWelcome(welcome)
        })
    }

    function connectSplash(splash) {
        splash.splashFinished.connect(function() {
            if (Session.hasDismissedWizard() || Session.loadActiveZowiDeviceAddress() !== "") {
                var home = stack.replace("qrc:/src/views/screens/HomeScreen.qml")
                connectHome(home)
                return
            }
            var welcome = stack.replace("qrc:/src/views/screens/WelcomeScreen.qml")
            connectWelcome(welcome)
        })
        splash.quitRequested.connect(function() { Qt.quit() })
    }

    function connectWelcome(welcome) {
        welcome.startWizard.connect(function() {
            var wizard = stack.push("qrc:/src/views/screens/WizardScreen.qml")
            wizard.startClicked.connect(function() {
                var scan = stack.push("qrc:/src/views/screens/ScanScreen.qml")
                scan.back.connect(function() { stack.pop() })
                scan.deviceSelected.connect(function() {
                    var home = stack.replace("qrc:/src/views/screens/HomeScreen.qml")
                    connectHome(home)
                })
            })
            wizard.dismissed.connect(function() {
                Session.saveWizardDismissed(true)
                var home = stack.replace("qrc:/src/views/screens/HomeScreen.qml")
                connectHome(home)
            })
        })
        welcome.knowMoreClicked.connect(function() {
            var locale = Translator.currentLocale().substring(0, 2)
            Qt.openUrlExternally(Config.get("know_more") + "/" + locale)
        })
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: SplashScreen {
            Component.onCompleted: connectSplash(this)
        }
    }
}
