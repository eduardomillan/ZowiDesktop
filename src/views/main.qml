import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "screens"
import "components"

Window {
    visible: true
    // Window size ratio: change 0.6 to desired fraction (e.g., 0.8 for 80%)
    width: Screen.desktopAvailableWidth * 0.6
    height: Screen.desktopAvailableHeight * 0.65
    title: "ZowiDesktop - " + AppVersion + (stack.currentItem && stack.currentItem.screenName ? " - " + stack.currentItem.screenName : "")
    color: Config.get("color_bg_app") || "#f4f9f4"

    property bool paired: false

    function tr(source) { return Translator.translate("main.qml", source) }

    // Push the Gamepad (PadScreen) with its standard wiring. Shared by the
    // Home gamepad tile and the project action button ("gamepad" target).
    function pushGamepad() {
        var pad = stack.push("qrc:/src/views/screens/PadScreen.qml")
        pad.backClicked.connect(function() { stack.pop() })
        pad.mouthScreenRequested.connect(function() {
            var mouth = stack.push("qrc:/src/views/screens/MouthScreen.qml")
            mouth.backClicked.connect(function() { stack.pop() })
        })
        pad.gestureScreenRequested.connect(function() {
            var gesture = stack.push("qrc:/src/views/screens/GestureScreen.qml")
            gesture.backClicked.connect(function() { stack.pop() })
        })
    }

    function connectHome(home) {
        home.settingsClicked.connect(function() {
            var settings = stack.push("qrc:/src/views/screens/SettingsScreen.qml")
            connectSettings(settings)
        })
        home.achievementsClicked.connect(function() {
            console.log("Home: achievements")
        })
        home.gamepadClicked.connect(function() { pushGamepad() })
        home.mouthEditorClicked.connect(function() {
            var editor = stack.push("qrc:/src/views/screens/MouthEditorScreen.qml")
            editor.backClicked.connect(function() { stack.pop() })
        })
        home.projectRequested.connect(function(projectId) {
            var project = stack.push("qrc:/src/views/screens/ProjectScreen.qml",
                                     { projectId: projectId })
            project.backClicked.connect(function() { stack.pop() })
            project.actionRequested.connect(function(target) {
                // Third project button: pop back to Home, then open the
                // destination screen (back from it returns to Home).
                stack.pop()
                if (target === "gamepad") {
                    pushGamepad()
                } else if (target === "calibration") {
                    var calib = stack.push("qrc:/src/views/screens/CalibrationScreen.qml")
                    calib.backClicked.connect(function() { stack.pop() })
                } else if (target === "mouth_editor") {
                    var editor = stack.push("qrc:/src/views/screens/MouthEditorScreen.qml")
                    editor.backClicked.connect(function() { stack.pop() })
                } else {
                    console.warn("main.qml: unknown project action target:", target)
                }
            })
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
        settings.calibrationRequested.connect(function() {
            var calib = stack.push("qrc:/src/views/screens/CalibrationScreen.qml")
            calib.backClicked.connect(function() { stack.pop() })
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
            console.log("main.qml: splashFinished, hasDismissedWizard:", Session.hasDismissedWizard(), "deviceAddress:", Session.loadActiveZowiDeviceAddress())
            if (Session.hasDismissedWizard() || Session.loadActiveZowiDeviceAddress() !== "") {
                console.log("main.qml: going to HomeScreen")
                var home = stack.replace("qrc:/src/views/screens/HomeScreen.qml")
                connectHome(home)
                return
            }
            console.log("main.qml: going to WelcomeScreen")
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
                    usbFound.pairingAttemptChanged.connect(function() {
                        wizardBusy = usbFound.pairingAttempt
                    })
                    usbFound.backClicked.connect(function() { wizardBusy = false; stack.pop() })
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
                    found.pairingAttemptChanged.connect(function() {
                        wizardBusy = found.pairingAttempt
                    })
                    found.backClicked.connect(function() { wizardBusy = false; stack.pop() })
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
        var addr = Robot.deviceAddress
        if (addr && addr !== "")
            Session.saveActiveZowiDeviceAddress(addr)
        Session.saveActiveZowiTransport(Robot.activeTransport === Robot.TransportUsb ? "usb" : "bt")
        Session.saveWizardDismissed(true)
        // Recompute the situation so SettingsScreen reflects a live connection.
        Robot.refreshTransports()

        if (!Robot.deviceName) {
            _waitForName(function() { _routeAfterNameCheck() })
            return
        }
        _routeAfterNameCheck()
    }

    function _routeAfterNameCheck() {
        var defaultName = Config.get("zowi_default_name") || "Zowi"
        if (Robot.deviceName && Robot.deviceName.toLowerCase() !== defaultName.toLowerCase()) {
            Session.saveActiveZowiName(Robot.deviceName)
            rootNotice.show(tr("already_named").arg(Robot.deviceName))
            homeTransitionTimer.start()
            return
        }
        var rename = stack.push("qrc:/src/views/screens/WizardRenameScreen.qml")
        rename.usbMode = Robot.usbAvailable && !Robot.bluetoothAvailable
        rename.backClicked.connect(function() { stack.pop() })
        rename.renamed.connect(function(name) {
            Session.saveActiveZowiName(name)
            var home = stack.replace("qrc:/src/views/screens/HomeScreen.qml")
            connectHome(home)
        })
    }

    function _waitForName(callback) {
        var done = false
        var handler = function() {
            if (!done && Robot.deviceName) {
                done = true
                Robot.deviceChanged.disconnect(handler)
                callback()
            }
        }
        Robot.deviceChanged.connect(handler)
        var safetyTimer = Qt.createQmlObject(
            'import QtQuick 2.15; Timer { interval: 5000; repeat: false }', main)
        safetyTimer.triggered.connect(function() {
            if (!done) {
                done = true
                Robot.deviceChanged.disconnect(handler)
                safetyTimer.destroy()
                callback()
            }
        })
        safetyTimer.start()
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: SplashScreen {
            Component.onCompleted: connectSplash(this)
        }
    }

    Timer {
        id: homeTransitionTimer
        interval: 800
        onTriggered: {
            var home = stack.replace("qrc:/src/views/screens/HomeScreen.qml")
            connectHome(home)
        }
    }

    // Wait-cursor state. True while the app cannot respond: a connection
    // attempt (incl. the blocking USB probe), a firmware restore, the pairing
    // attempt of the wizard, or the short home transition.
    property bool wizardBusy: false
    property bool busy: Robot.connecting || Robot.restoring
                        || homeTransitionTimer.running
                        || wizardBusy

    DevOverlay {
    }

    // Toggle the DEV overlay from every screen (Ctrl+D). Captured at window
    // scope so it works regardless of which screen has active focus, without
    // colliding with typing the letter "d" (e.g. in the rename field).
    Shortcut {
        sequence: "Ctrl+D"
        context: Qt.WindowShortcut
        onActivated: Config.devOverlayVisible = !Config.devOverlayVisible
    }

    MessageBar {
        id: rootNotice
    }

    // Hourglass overlay: while `busy` is true the wait cursor is shown and
    // every input event is swallowed, so the user gets feedback instead of a
    // seemingly frozen window.
    MouseArea {
        anchors.fill: parent
        z: 10000
        visible: busy
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        cursorShape: Qt.WaitCursor
    }
}
