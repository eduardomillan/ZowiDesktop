// HomeScreen: Main dashboard with two swipeable pages:
// Page 0 - Zowi Apps (games/modes), Page 1 - Projects.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

FocusScope {
    id: homeScope

    signal settingsClicked()
    signal achievementsClicked()
    signal gamepadClicked()
    signal mouthEditorClicked()
    signal goSplash()
    signal goWelcome()

    property string screenName: "HomeScreen"
    property real cellSpacing: 60
    property real iconSize: Math.min(home.width * 0.55, 90)

    function tr(source) { return Translator.translate("HomeScreen.qml", source) }

    Rectangle {
        id: home
        anchors.fill: parent
        property bool robotReady: Robot.connected && Robot.appId !== "" && Robot.battery >= 0

        color: Config.get("color_bg_app") || "#f4f9f4"

        property var projectsData: [
            { name: tr("move_objects"),    icon: "qrc:/images/android/move_button.png",        enabled: false },
            { name: tr("choreography"),    icon: "qrc:/images/android/choreography_button.png", enabled: false },
            { name: tr("robot_form"),      icon: "qrc:/images/android/robot_form_button.png",   enabled: false },
            { name: tr("robot_eyes"),      icon: "qrc:/images/android/eyes_button.png",         enabled: false },
            { name: tr("robot_feet"),      icon: "qrc:/images/android/feet_button.png",         enabled: false },
            { name: tr("robot_alarm"),     icon: "qrc:/images/android/alarm_button.png",        enabled: false },
            { name: tr("adivinawi"),       icon: "qrc:/images/android/adivinawi_button.png",    enabled: false },
            { name: tr("gravity"),         icon: "qrc:/images/android/gravity_button.png",      enabled: false },
            { name: tr("hello_world"),     icon: "qrc:/images/android/bitbloq_button.png",      enabled: false },
            { name: tr("bitbloq_sensors"), icon: "qrc:/images/android/bitbloq2_button.png",     enabled: false }
        ]

    // Top bar with Settings and Achievements
    Row {
        id: topBar
        anchors {
            top: statusIndicator.bottom
            left: parent.left
            right: parent.right
            margins: 15
        }
        height: 88

        Button {
            id: settingsBtn
            width: 88
            height: 88

            contentItem: Image {
                source: "qrc:/images/android/settings_button.png"
                sourceSize.width: 56
                sourceSize.height: 56
                fillMode: Image.PreserveAspectFit
            }

            background: Rectangle {
                radius: 44
                color: settingsBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
            }

            onClicked: homeScope.settingsClicked()
        }

        Item { width: parent.width - 220; height: 1 }

        Button {
            id: achievementsBtn
            width: 88
            height: 88

            contentItem: Image {
                source: "qrc:/images/android/achievements_button.png"
                sourceSize.width: 56
                sourceSize.height: 56
                fillMode: Image.PreserveAspectFit
            }

            background: Rectangle {
                radius: 44
                color: achievementsBtn.pressed ? Config.get("color_bg_hover") || "#e0f0e0" : "transparent"
            }

            onClicked: homeScope.achievementsClicked()
        }
    }

    // Connection status bar (top of the screen)
    StatusBar {
        id: statusIndicator
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 5
        }
    }

    // Page title and indicator
    Column {
        id: titleArea
        anchors {
            top: statusIndicator.bottom
            left: parent.left
            right: parent.right
            topMargin: 10
        }
        height: 50

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12

            Text {
                text: tr("zowi_apps")
                color: swipeView.currentIndex === 0 ? Config.get("color_accent") || "#21a69b" : Config.get("color_primary") || "#2d5a2d"
                font.pixelSize: 18
                font.bold: swipeView.currentIndex === 0
                opacity: swipeView.currentIndex === 0 ? 1.0 : 0.5

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: swipeView.currentIndex = 0
                }
            }

            Text {
                text: "|"
                color: Config.get("color_primary") || "#2d5a2d"
                font.pixelSize: 18
                opacity: 0.3
            }

            Text {
                text: tr("projects")
                color: swipeView.currentIndex === 1 ? Config.get("color_accent") || "#21a69b" : Config.get("color_primary") || "#2d5a2d"
                font.pixelSize: 18
                font.bold: swipeView.currentIndex === 1
                opacity: swipeView.currentIndex === 1 ? 1.0 : 0.5

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: swipeView.currentIndex = 1
                }
            }
        }
    }

    // SwipeView with 2 pages
    SwipeView {
        id: swipeView
        anchors {
            top: titleArea.bottom
            left: parent.left
            right: parent.right
            bottom: pageIndicator.top
            topMargin: 5
        }
        currentIndex: 0
        interactive: true

        // Page 0: Zowi Apps
        Item {
            ListModel { id: appsModel }

            Row {
                id: appsRow
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin: spacing * 2
                    rightMargin: spacing * 2
                }
                spacing: 15

                Repeater {
                    model: appsModel

                    delegate: Column {
                        spacing: 6
                        width: (appsRow.width - appsRow.spacing * (appsModel.count - 1)) / appsModel.count

                        readonly property real btnSize: Math.min(width * 0.55, 90)
                        // Grey-shade buttons that are not yet implemented.
                        readonly property bool isDisabled: !model.enabled

                        Rectangle {
                            width: btnSize
                            height: btnSize
                            radius: Math.min(btnSize * 0.2, 16)
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: isDisabled
                                   ? (Config.get("color_bg_disabled") || "#e6e6e6")
                                   : (appMouse.containsMouse && home.robotReady ? Config.get("color_bg_hover") || "#e0f0e0" : "#ffffff")
                            border.color: isDisabled
                                          ? (Config.get("color_border_disabled") || "#c8c8c8")
                                          : (Config.get("color_accent") || "#21a69b")
                            border.width: 1
                            opacity: (!home.robotReady || isDisabled) ? 0.4 : 1.0

                            Image {
                                anchors.centerIn: parent
                                source: icon
                                sourceSize.width: Math.min(btnSize * 0.6, 50)
                                sourceSize.height: Math.min(btnSize * 0.6, 50)
                                fillMode: Image.PreserveAspectFit
                                opacity: isDisabled ? 0.5 : 1.0
                            }

                            MouseArea {
                                id: appMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: (!home.robotReady || isDisabled) ? Qt.ForbiddenCursor : Qt.PointingHandCursor
                                enabled: home.robotReady && !isDisabled
                                onClicked: {
                                    if (name === tr("gamepad")) {
                                        homeScope.gamepadClicked()
                                    } else if (name === tr("mouths_editor")) {
                                        homeScope.mouthEditorClicked()
                                    } else {
                                        console.log("Home: tapped", name)
                                    }
                                }
                            }
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: name
                            color: isDisabled
                                   ? (Config.get("color_fg_disabled") || "#9e9e9e")
                                   : (Config.get("color_primary") || "#2d5a2d")
                            font.pixelSize: Math.min(parent.width * 0.1, 12)
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            opacity: !home.robotReady ? 0.6 : 1.0
                        }
                    }
                }
            }
        }

        // Page 1: Projects
        Item {

            Item {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                width: parent.width * 0.8
                height: parent.height
                clip: true

                Flow {
                    anchors.centerIn: parent
                    width: parent.width
                    spacing: homeScope.cellSpacing

                    Repeater {
                        model: home.projectsData

                        Rectangle {
                            width: homeScope.iconSize + 8
                            height: homeScope.iconSize + 24
                            color: "transparent"

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.ForbiddenCursor
                            }

                            Column {
                                anchors.centerIn: parent
                                spacing: 4

                                Rectangle {
                                    width: homeScope.iconSize
                                    height: homeScope.iconSize
                                    radius: Math.min(homeScope.iconSize * 0.2, 16)
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    color: Config.get("color_bg_disabled") || "#e6e6e6"
                                    border.color: Config.get("color_border_disabled") || "#c8c8c8"
                                    border.width: 1
                                    opacity: 0.4

                                    Image {
                                        anchors.centerIn: parent
                                        width: homeScope.iconSize * 0.65
                                        height: homeScope.iconSize * 0.65
                                        source: modelData.icon
                                        sourceSize: Qt.size(homeScope.iconSize * 2, homeScope.iconSize * 2)
                                        fillMode: Image.PreserveAspectFit
                                        opacity: 0.5
                                    }
                                }

                                Text {
                                    text: modelData.name
                                    color: Config.get("color_fg_disabled") || "#9e9e9e"
                                    font.pixelSize: Math.max(9, homeScope.iconSize * 0.16)
                                    horizontalAlignment: Text.AlignHCenter
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Row {
        id: devNav
        visible: Config.devMode && Config.devOverlayVisible
        anchors.bottom: pageIndicator.top
        anchors.bottomMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10

        Button {
            implicitWidth: 120
            height: 28
            text: "Splash"

            contentItem: Text {
                text: parent.text
                font.pixelSize: 10
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 14
                color: Config.get("color_warning") || "#e67e22"
            }

            onClicked: homeScope.goSplash()
        }

        Button {
            implicitWidth: 120
            height: 28
            text: "Welcome"

            contentItem: Text {
                text: parent.text
                font.pixelSize: 10
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 14
                color: Config.get("color_warning") || "#e67e22"
            }

            onClicked: homeScope.goWelcome()
        }
    }

    // Page indicator dots
    Row {
        id: pageIndicator
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10

        Repeater {
            model: 2

            Rectangle {
                width: swipeView.currentIndex === index ? 12 : 8
                height: 8
                radius: 4
                color: swipeView.currentIndex === index ? Config.get("color_accent") || "#21a69b" : "#cccccc"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: swipeView.currentIndex = index
                }
            }
        }
    }

    } // Rectangle

    Component.onCompleted: {
        // Auto-connect on launch (mirrors ZowiAppReborn's onResume ->
        // manageConnection). Prefer USB when the auto-detector selected it and
        // a robot is present on a port; otherwise reconnect to the saved
        // Bluetooth device. The BT backend auto-reconnects every 3s.
        if (!Robot.connected && Robot.situation !== Robot.SituationTransportLost) {
            if (Robot.activeTransport === Robot.TransportUsb && Robot.usbAvailable)
                Robot.connectUsb()
            else if (Session.loadActiveZowiDeviceAddress() !== "")
                Robot.connectToDevice(Session.loadActiveZowiDeviceAddress())
        }

        var apps = [
            { name: tr("gamepad"), icon: "qrc:/images/android/pad_button.png", enabled: true },
            { name: tr("timeline"), icon: "qrc:/images/android/timeline_button.png", enabled: false },
            { name: tr("zowi_says"), icon: "qrc:/images/android/simon_game_button.png", enabled: false },
            { name: tr("mouths"), icon: "qrc:/images/android/mouths_game_button.png", enabled: false },
            { name: tr("mouths_editor"), icon: "qrc:/images/android/mouths_editor_game_button.png", enabled: true }
        ]
        for (var i = 0; i < apps.length; i++)
            appsModel.append(apps[i])

        homeScope.forceActiveFocus()
    }
}
