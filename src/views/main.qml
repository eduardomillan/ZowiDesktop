import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "screens"
import "components"

Window {
    visible: true
    width: 1024
    height: 600
    title: "Zowi Desktop - " + (stack.currentItem && stack.currentItem.screenName ? stack.currentItem.screenName : "")
    color: "#f4f9f4"

    property bool paired: false

    function connectHome(home) {
        home.settingsClicked.connect(function() {
            var settings = stack.push("qrc:/src/views/screens/SettingsScreen.qml")
            connectSettings(settings)
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

    function connectSettings(settings) {
        settings.backClicked.connect(function() {
            stack.pop()
        })
        settings.renameRequested.connect(function() {
            var rename = stack.push("qrc:/src/views/screens/WizardRenameScreen.qml")
            rename.backClicked.connect(function() { stack.pop() })
            rename.renamed.connect(function(name) {
                Session.saveActiveZowiName(name)
                stack.pop()
            })
        })
        settings.forgetCompleted.connect(function() {
            var welcome = stack.replace("qrc:/src/views/screens/WelcomeScreen.qml")
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
            // Starting (or restarting) the pairing wizard from scratch: fall back
            // to Automatic transport so the wizard can drive the connection.
            Bluetooth.setTransportPreference(Bluetooth.TransportAuto)
            var wizard = stack.push("qrc:/src/views/screens/WizardScreen.qml")
            wizard.startClicked.connect(function() {
                var scan = stack.push("qrc:/src/views/screens/ScanScreen.qml")
                scan.back.connect(function() { stack.pop() })
                scan.deviceSelected.connect(function() {
                    var found = stack.push("qrc:/src/views/screens/WizardFoundScreen.qml")
                    found.backClicked.connect(function() { stack.pop() })
                    found.paired.connect(function() {
                        var rename = stack.push("qrc:/src/views/screens/WizardRenameScreen.qml")
                        rename.backClicked.connect(function() { stack.pop() })
                        rename.renamed.connect(function(name) {
                            Session.saveActiveZowiName(name)
                            var home = stack.replace("qrc:/src/views/screens/HomeScreen.qml")
                            connectHome(home)
                        })
                    })
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

    DevOverlay {
    }
}
