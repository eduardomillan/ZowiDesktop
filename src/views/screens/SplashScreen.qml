// SplashScreen: Initial screen shown on app launch.
// Displays the Zowi logo and provides Continue/Quit buttons along with the
// language selector (ES, CA, EN, FR, BG).
//
// Layout: a single ColumnLayout (connection notice, logo, title, buttons,
// language selector) instead of absolutely offset layers, so nothing overlaps
// at any window size. Sizes scale with the window (see `unit`) between sane
// limits. Everything is in logical pixels and the popup stays inside the
// window, so it behaves the same on X11 and Wayland (incl. fractional scaling).
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

FocusScope {
    id: splashScope

    signal splashFinished()
    signal quitRequested()

    // Read by main.qml for the window title.
    property string screenName: "SplashScreen"
    property bool _resetNoZowi: false

    focus: true

    // ── Palette ─────────────────────────────────────────────────────────
    readonly property color primary: Config.get("color_primary") || "#2d5a2d"
    readonly property color accent: Config.get("color_accent") || "#21a69b"
    readonly property color accentPressed: Config.get("color_accent_pressed") || "#17736c"

    // ── Scale: derived from the smaller window side ─────────────────────
    function clamp(value, low, high) { return Math.max(low, Math.min(high, value)) }
    readonly property real unit: Math.min(width, height)
    readonly property int pageMargin: Math.round(clamp(unit * 0.04, 12, 32))
    readonly property int gap: Math.round(clamp(unit * 0.025, 8, 24))
    readonly property real logoMax: clamp(unit * 0.30, 96, 380)
    readonly property int titleSize: Math.round(clamp(unit * 0.08, 28, 96))
    readonly property int subtitleSize: Math.round(clamp(unit * 0.03, 14, 30))
    readonly property int buttonHeight: Math.round(clamp(unit * 0.08, 44, 64))
    readonly property int buttonWidth: Math.round(clamp(width * 0.25, 160, 300))
    readonly property int buttonFont: Math.round(clamp(unit * 0.03, 16, 24))
    readonly property int smallFont: Math.round(clamp(unit * 0.022, 13, 18))
    // Continue and Quit stack in a column when they do not fit side by side.
    readonly property bool narrow: width < 2 * buttonWidth + gap + 2 * pageMargin

    function tr(source) { return Translator.translate("SplashScreen.qml", source) }

    Keys.onReturnPressed: splashScope.splashFinished()
    Keys.onEnterPressed: splashScope.splashFinished()

    Rectangle {
        anchors.fill: parent
        color: Config.get("color_bg_app") || "#f4f9f4"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: splashScope.pageMargin
        spacing: splashScope.gap

        // No connection notice: part of the flow, so it pushes the content down
        // instead of covering the logo.
        Rectangle {
            id: noBtBanner
            visible: !Robot.bluetoothAvailable && !Robot.usbAvailable
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.maximumWidth: 520
            Layout.preferredHeight: noBtText.implicitHeight + 24
            radius: 12
            color: "#fff4e5"
            border.color: Config.get("color_warning") || "#e67e22"
            border.width: 1

            Text {
                id: noBtText
                anchors.fill: parent
                anchors.margins: 12
                text: splashScope.tr("no_connection")
                color: "#a0522d"
                font.pixelSize: splashScope.smallFont
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Item { Layout.fillHeight: true }

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: splashScope.logoMax
            Layout.maximumHeight: splashScope.logoMax
            Layout.minimumHeight: 48
            source: Config.get("splash_image")
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignHCenter
            verticalAlignment: Image.AlignVCenter
            smooth: true
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: splashScope.tr("zowi")
            color: splashScope.primary
            font.pixelSize: splashScope.titleSize
            font.bold: true
            font.family: "monospace"
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: splashScope.tr("desktop")
            color: splashScope.primary
            font.pixelSize: splashScope.subtitleSize
            opacity: 0.7
        }

        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: splashScope.gap
            columns: splashScope.narrow ? 1 : 2
            columnSpacing: splashScope.gap
            rowSpacing: splashScope.gap

            Button {
                id: continueButton
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: splashScope.buttonWidth
                Layout.preferredHeight: splashScope.buttonHeight
                text: splashScope.tr("continue")

                contentItem: Text {
                    text: continueButton.text
                    font.pixelSize: splashScope.buttonFont
                    font.bold: true
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: height / 2
                    color: continueButton.pressed ? splashScope.accentPressed : splashScope.accent
                }

                onClicked: splashScope.splashFinished()
            }

            Button {
                id: quitButton
                visible: Config.get("button_quit_visible") === "true"
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: splashScope.buttonWidth
                Layout.preferredHeight: splashScope.buttonHeight
                text: splashScope.tr("quit")

                contentItem: Text {
                    text: quitButton.text
                    font.pixelSize: Math.round(splashScope.buttonFont * 0.8)
                    font.bold: true
                    color: splashScope.primary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: 0.8
                }

                background: Rectangle {
                    radius: height / 2
                    color: "transparent"
                    border.color: splashScope.primary
                    border.width: 2
                    opacity: 0.5
                }

                onClicked: splashScope.quitRequested()
            }
        }

        Item { Layout.fillHeight: true }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: splashScope.tr("select_language")
            color: splashScope.primary
            font.pixelSize: splashScope.smallFont
            opacity: 0.8
        }

        ComboBox {
            id: langCombo
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.round(splashScope.clamp(splashScope.width * 0.2, 160, 220))
            Layout.preferredHeight: Math.round(splashScope.clamp(splashScope.unit * 0.06, 36, 48))
            Layout.bottomMargin: 4

            model: ListModel {
                ListElement { text: "Español"; locale: "es_ES" }
                ListElement { text: "Valencià"; locale: "ca_ES" }
                ListElement { text: "English"; locale: "en_US" }
                ListElement { text: "Français"; locale: "fr_FR" }
                ListElement { text: "Български"; locale: "bg_BG" }
            }
            textRole: "text"

            font.pixelSize: Math.round(splashScope.clamp(splashScope.unit * 0.025, 14, 18))
            font.family: "monospace"

            Component.onCompleted: {
                var cur = Translator.currentLocale()
                for (var i = 0; i < model.count; ++i) {
                    if (model.get(i).locale === cur) {
                        currentIndex = i
                        return
                    }
                }
                currentIndex = 2 // fallback: English
            }

            onActivated: {
                var loc = model.get(currentIndex).locale
                if (Translator.currentLocale() !== loc) {
                    Translator.load(loc)
                    Session.saveString("locale", loc)
                }
            }

            contentItem: Text {
                text: langCombo.displayText
                color: splashScope.primary
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font: langCombo.font
            }

            background: Rectangle {
                radius: height / 2
                border.color: splashScope.primary
                border.width: 1
                opacity: 0.5
                color: "transparent"
            }

            delegate: ItemDelegate {
                width: langCombo.width
                height: langCombo.height

                contentItem: Text {
                    text: model.text
                    color: splashScope.primary
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    font: langCombo.font
                }

                background: Rectangle {
                    color: langCombo.highlightedIndex === index ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
                }
            }

            // In-window popup (the default), opened upwards on purpose: the
            // selector sits at the bottom, so it never runs off the window and
            // behaves the same on X11 and Wayland.
            popup: Popup {
                y: -implicitHeight - 2
                width: langCombo.width
                padding: 0

                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: langCombo.delegateModel
                    currentIndex: langCombo.highlightedIndex

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                }

                background: Rectangle {
                    color: "#ffffff"
                    radius: 8
                    border.color: splashScope.primary
                    border.width: 1
                }
            }
        }
    }

    // Development overlay: reset the registered Zowi.
    Item {
        anchors.fill: parent
        z: 10
        visible: Config.devMode && Config.devOverlayVisible

        MessageBar {
            id: msgBar
            duration: parseInt(Config.get("message_duration")) || 2000
        }

        Button {
            id: resetButton
            anchors {
                left: parent.left
                bottom: msgBar.top
                margins: 12
            }
            implicitWidth: 90
            height: 32
            text: splashScope.tr("reset")

            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.pixelSize: 12
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 16
                color: resetButton.pressed ? "#d35400" : Config.get("color_warning") || "#e67e22"
            }

            enabled: !msgBar.visible

            onClicked: {
                if (msgBar.visible) return
                var addr = Session.loadActiveZowiDeviceAddress()
                if (!addr) addr = Robot.deviceAddress
                splashScope._resetNoZowi = (addr === "")
                // Forget the registered Zowi. The controller also tries a
                // factory rename (zowi_default_name) if it can reach the robot.
                forgetter.forget(addr)
            }
        }
    }

    ForgetController {
        id: forgetter
        onForgetFinished: function(unpaired, message) {
            if (splashScope._resetNoZowi)
                msgBar.show(splashScope.tr("reset_no_zowi"), Config.get("color_error") || "#c0392b")
            else if (unpaired)
                msgBar.show(splashScope.tr("unpair_success"))
            else
                msgBar.show(splashScope.tr("unpair_app_only"))
        }
        onStatusMessage: function(text) { msgBar.show(text) }
    }

    Component.onCompleted: splashScope.forceActiveFocus()
}
