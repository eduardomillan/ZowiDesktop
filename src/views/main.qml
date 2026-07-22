import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "screens"
import "components"

Window {
    visible: true
    width: 1024
    height: 600
    title: "ZowiDesktop - " + AppVersion + (stack.currentItem && stack.currentItem.screenName ? " - " + stack.currentItem.screenName : "")
    color: "#f4f9f4"

    property bool paired: false

    function tr(source) { return Translator.translate("main.qml", source) }

    function connectHome(home) {
        home.settingsClicked.connect(function() {
            var settings = stack.push("qrc:/src/views/screens/SettingsScreen.qml")
            connectSettings(settings)
        })
        home.achievementsClicked.connect(function() {
            console.log("Home: achievements")
        })
        home.gamepadClicked.connect(function() {
            var pad = stack.push("qrc:/src/views/screens/PadScreen.qml")
            pad.backClicked.connect(function() { stack.pop() })
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
            Robot.setTransportPreference(Robot.TransportAuto)
            var wizard = stack.push("qrc:/src/views/screens/WizardScreen.qml")
            wizard.startClicked.connect(function() {
                // Only USB available (no Bluetooth): skip the Bluetooth scan and
                // go straight to a USB connection (no pairing concept).
                if (Robot.usbAvailable && !Robot.bluetoothAvailable) {
                    var usbFound = stack.push("qrc:/src/views/screens/WizardFoundScreen.qml")
                    usbFound.usbMode = true
                    usbFound.backClicked.connect(function() { stack.pop() })
                    usbFound.paired.connect(function() {
                        finishRegistration()
                    })
                    return
                }
                // Bluetooth available (alone or with USB): Bluetooth is the
                // priority transport, drive the scan/pairing flow.
                var scan = stack.push("qrc:/src/views/screens/ScanScreen.qml")
                scan.back.connect(function() { stack.pop() })
                scan.deviceSelected.connect(function() {
                    var found = stack.push("qrc:/src/views/screens/WizardFoundScreen.qml")
                    found.backClicked.connect(function() { stack.pop() })
                    found.paired.connect(function() {
                        finishRegistration()
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

    // Shared end-of-registration step: after a Zowi is paired/connected (Bt or
    // USB), skip the rename wizard if it already has a non-default name and go
    // straight to Home; otherwise show the rename wizard.
    function finishRegistration() {
        // Persist the registration so the situation state machine reports
        // Connected (not Unregistered) and SettingsScreen shows the live link.
        console.log("[finishRegistration] deviceAddress=", Robot.deviceAddress,
                    "activeTransport=", Robot.activeTransport,
                    "connected=", Robot.connected)
        Session.saveActiveZowiDeviceAddress(Robot.deviceAddress)
        Session.saveActiveZowiTransport(Robot.activeTransport === Robot.TransportUsb ? "usb" : "bt")
        Session.saveWizardDismissed(true)
        // Recompute the situation so SettingsScreen reflects a live connection.
        Robot.refreshTransports()

        var defaultName = Config.get("zowi_default_name") || "Zowi"
        if (Robot.deviceName && Robot.deviceName.toLowerCase() !== defaultName.toLowerCase()) {
            Session.saveActiveZowiName(Robot.deviceName)
            rootNotice.show(tr("already_named").arg(Robot.deviceName))
            var home = stack.replace("qrc:/src/views/screens/HomeScreen.qml")
            connectHome(home)
            return
        }
        var rename = stack.push("qrc:/src/views/screens/WizardRenameScreen.qml")
        // Flag USB-only connections so the rename is best-effort (the robot may
        // not ACK the rename over USB).
        rename.usbMode = Robot.usbAvailable && !Robot.bluetoothAvailable
        rename.backClicked.connect(function() { stack.pop() })
        rename.renamed.connect(function(name) {
            Session.saveActiveZowiName(name)
            var home = stack.replace("qrc:/src/views/screens/HomeScreen.qml")
            connectHome(home)
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

    MessageBar {
        id: rootNotice
    }
}
