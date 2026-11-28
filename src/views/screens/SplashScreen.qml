// SplashScreen: initial screen shown on app launch (also reached from
// Settings -> Language). Same look and structure as WelcomeScreen: one compact
// group centred in the window (Zowi image, title, "Desktop", Continue / Quit
// pills, a row of language pills with their flag, and the "no connection"
// notice), with the image as the only elastic piece. Sizes scale with the
// window (see `unit`) between sane limits. Everything is in logical pixels and
// there is no popup, so it behaves the same on X11 and Wayland (incl.
// fractional scaling).
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

FocusScope {
    id: splash

    signal splashFinished()
    signal quitRequested()

    // Read by main.qml for the window title.
    property string screenName: "SplashScreen"
    property bool _resetNoZowi: false
    focus: true

    readonly property color primary: Config.get("color_primary") || "#2d5a2d"
    readonly property color accent: Config.get("color_accent") || "#21a69b"
    readonly property color accentPressed: Config.get("color_accent_pressed") || "#17736c"
    readonly property color hover: Config.get("color_bg_hover") || "#e0f0e0"

    function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)) }
    readonly property real unit: Math.min(width, height)
    readonly property int margin: Math.round(clamp(unit * 0.03, 10, 28))
    readonly property int gap: Math.round(clamp(unit * 0.025, 8, 20))      // 15 at 600
    readonly property int imageSize: Math.round(clamp(unit * 0.44, 110, 420)) // 264 at 600 (max; shrinks if short)
    readonly property int titleSize: Math.round(clamp(unit * 0.06, 26, 64))  // 36 at 600
    readonly property int taglineSize: Math.round(clamp(unit * 0.027, 13, 24)) // 16 at 600
    readonly property int pillHeight: Math.round(clamp(unit * 0.075, 38, 52)) // 45 at 600
    readonly property int pillWidth: Math.round(clamp(width * 0.15, 130, 200)) // 154 at 1024
    readonly property int chipHeight: Math.round(clamp(unit * 0.058, 30, 40))
    readonly property int chipFont: Math.round(clamp(unit * 0.023, 12, 17))
    readonly property bool stackButtons: width < 2 * pillWidth + gap + 2 * margin
    readonly property bool wideChips: width - 2 * margin >= 640

    readonly property var languages: [
        { text: "Español", locale: "es_ES" },
        { text: "Valencià", locale: "ca_ES" },
        { text: "English", locale: "en_US" },
        { text: "Français", locale: "fr_FR" },
        { text: "Български", locale: "bg_BG" }
    ]
    property string currentLocale: Translator.currentLocale()
    // No Bluetooth and no USB: show the notice. (A property so previews can force it.)
    property bool noticeVisible: !Robot.bluetoothAvailable && !Robot.usbAvailable

    // Reading `_rev` makes every text binding that calls tr() re-evaluate when
    // the language changes, so the screen is translated in place (no reload).
    property int _rev: 0
    function tr(source) { var r = _rev; return Translator.translate("SplashScreen.qml", source) }

    Keys.onReturnPressed: splash.splashFinished()
    Keys.onEnterPressed: splash.splashFinished()

    Rectangle {
        anchors.fill: parent
        color: Config.get("color_bg_app") || "#f4f9f4"
    }

    // One group, centred: equal flexible gaps above and below. When the window
    // is short the image (the only elastic piece) gives way first.
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: splash.margin
        spacing: splash.gap

        Item { Layout.fillHeight: true }

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: splash.imageSize
            Layout.maximumHeight: splash.imageSize
            Layout.minimumHeight: 40
            source: Config.get("splash_image")
            sourceSize.width: 960   // keep it sharp (hi-dpi / fractional scaling)
            sourceSize.height: 960
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignHCenter
            verticalAlignment: Image.AlignVCenter
            smooth: true
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: splash.tr("zowi")
            color: splash.primary
            font.pixelSize: splash.titleSize
            font.bold: true
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: splash.tr("desktop")
            color: splash.primary
            font.pixelSize: splash.taglineSize
            opacity: 0.8
        }

        // Primary / secondary pills, exactly the WelcomeScreen ones.
        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Math.round(splash.gap / 2)
            columns: splash.stackButtons ? 1 : 2
            columnSpacing: splash.gap
            rowSpacing: splash.gap

            Button {
                id: continueButton
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: splash.pillWidth
                Layout.preferredHeight: splash.pillHeight
                text: splash.tr("continue")

                contentItem: Text {
                    text: continueButton.text
                    font.pixelSize: Math.round(splash.pillHeight * 0.36)
                    font.bold: true
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: height / 2
                    color: continueButton.pressed ? splash.accentPressed : splash.accent
                }
                onClicked: splash.splashFinished()
            }

            Button {
                id: quitButton
                visible: Config.get("button_quit_visible") === "true"
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: splash.pillWidth
                Layout.preferredHeight: splash.pillHeight
                text: splash.tr("quit")

                contentItem: Text {
                    text: quitButton.text
                    font.pixelSize: Math.round(splash.pillHeight * 0.30)
                    font.bold: true
                    color: splash.primary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    opacity: 0.8
                }
                background: Rectangle {
                    radius: height / 2
                    color: "transparent"
                    border.color: splash.primary
                    border.width: 2
                    opacity: 0.5
                }
                onClicked: splash.quitRequested()
            }
        }

        // Language pills: the selected one is highlighted.
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Math.round(splash.gap / 2)

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: splash.tr("select_language")
                color: splash.primary
                font.pixelSize: splash.chipFont
                opacity: 0.7
            }

            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                columns: splash.wideChips ? 5 : 3
                columnSpacing: 8
                rowSpacing: 8

                Repeater {
                    model: splash.languages
                    delegate: Button {
                        id: chip
                        readonly property bool selected: splash.currentLocale === modelData.locale
                        Layout.preferredHeight: splash.chipHeight
                        Layout.preferredWidth: chipContent.implicitWidth + 2 * Math.round(splash.chipHeight * 0.5)
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.text

                        // Flag first, then the language name.
                        contentItem: Item {
                            implicitWidth: chipContent.implicitWidth
                            Row {
                                id: chipContent
                                anchors.centerIn: parent
                                spacing: Math.round(splash.chipHeight * 0.25)

                                FlagIcon {
                                    anchors.verticalCenter: parent.verticalCenter
                                    locale: modelData.locale
                                    width: Math.round(splash.chipHeight * 0.72)
                                    height: Math.round(width * 2 / 3)
                                    opacity: chip.selected ? 1.0 : 0.85
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: chip.text
                                    font.pixelSize: splash.chipFont
                                    font.bold: chip.selected
                                    color: splash.primary
                                    opacity: chip.selected ? 1.0 : 0.8
                                }
                            }
                        }
                        background: Rectangle {
                            radius: height / 2
                            color: chip.selected ? splash.hover : (chip.hovered ? Qt.lighter(splash.hover, 1.15) : "transparent")
                            border.color: splash.primary
                            border.width: chip.selected ? 2 : 1
                            opacity: chip.selected ? 0.9 : 0.5
                        }
                        onClicked: {
                            if (Translator.currentLocale() !== modelData.locale) {
                                Translator.load(modelData.locale)
                                Session.saveString("locale", modelData.locale)
                            }
                            splash.currentLocale = Translator.currentLocale()
                        }
                    }
                }
            }
        }

        // No-connection notice: inside the group (it never covers anything and
        // the group stays centred), same style as before.
        Rectangle {
            visible: splash.noticeVisible
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(520, splash.width - 2 * splash.margin)
            Layout.preferredHeight: noConnText.implicitHeight + 14
            radius: 12
            color: "#fff4e5"
            border.color: Config.get("color_warning") || "#e67e22"
            border.width: 1

            Text {
                id: noConnText
                anchors.fill: parent
                anchors.margins: 7
                text: splash.tr("no_connection")
                color: "#a0522d"
                font.pixelSize: splash.chipFont
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: Translator
        function onLanguageChanged() {
            splash.currentLocale = Translator.currentLocale()
            splash._rev++
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
            text: splash.tr("reset")

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
                splash._resetNoZowi = (addr === "")
                // Forget the registered Zowi. The controller also tries a
                // factory rename (zowi_default_name) if it can reach the robot.
                forgetter.forget(addr)
            }
        }
    }

    ForgetController {
        id: forgetter
        onForgetFinished: function(unpaired, message) {
            if (splash._resetNoZowi)
                msgBar.show(splash.tr("reset_no_zowi"), Config.get("color_error") || "#c0392b")
            else if (unpaired)
                msgBar.show(splash.tr("unpair_success"))
            else
                msgBar.show(splash.tr("unpair_app_only"))
        }
        onStatusMessage: function(text) { msgBar.show(text) }
    }

    Component.onCompleted: splash.forceActiveFocus()
}
