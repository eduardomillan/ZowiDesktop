// HomeScreen: Main dashboard with two swipeable pages:
// Page 0 - Zowi Apps (games/modes), Page 1 - Projects.
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

Rectangle {
    id: home
    color: "#f4f9f4"

    signal settingsClicked()
    signal achievementsClicked()
    signal goSplash()
    signal goWelcome()

    property string screenName: "HomeScreen"

    function tr(source) { return Translator.translate("HomeScreen.qml", source) }

    // Top bar with Settings and Achievements
    Row {
        id: topBar
        anchors {
            top: statusIndicator.bottom
            left: parent.left
            right: parent.right
            margins: 15
        }
        height: 50

        Button {
            id: settingsBtn
            width: 44
            height: 44

            contentItem: Image {
                source: "qrc:/images/android/settings_button.png"
                sourceSize.width: 28
                sourceSize.height: 28
                fillMode: Image.PreserveAspectFit
            }

            background: Rectangle {
                radius: 22
                color: settingsBtn.pressed ? "#e0f0e0" : "transparent"
            }

            onClicked: home.settingsClicked()
        }

        Item { width: parent.width - 132; height: 1 }

        Button {
            id: achievementsBtn
            width: 44
            height: 44

            contentItem: Image {
                source: "qrc:/images/android/achievements_button.png"
                sourceSize.width: 28
                sourceSize.height: 28
                fillMode: Image.PreserveAspectFit
            }

            background: Rectangle {
                radius: 22
                color: achievementsBtn.pressed ? "#e0f0e0" : "transparent"
            }

            onClicked: home.achievementsClicked()
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
                text: tr("Zowi Apps")
                color: swipeView.currentIndex === 0 ? "#21a69b" : "#2d5a2d"
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
                color: "#2d5a2d"
                font.pixelSize: 18
                opacity: 0.3
            }

            Text {
                text: tr("Projects")
                color: swipeView.currentIndex === 1 ? "#21a69b" : "#2d5a2d"
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
        GridView {
            id: appsGrid
            cellWidth: Math.floor(width / 3)
            cellHeight: Math.floor(Math.min(cellWidth,
                (swipeView.height - 40) / Math.max(Math.ceil(count / 3), 1)))
            bottomMargin: 40
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            model: ListModel {
                id: appsModel
            }

            delegate: Item {
                width: appsGrid.cellWidth
                height: appsGrid.cellHeight
                readonly property real cellBox: Math.min(appsGrid.cellWidth * 0.55, 80)

                Column {
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: cellBox
                        height: cellBox
                        radius: Math.min(cellBox * 0.2, 16)
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: appMouse.containsMouse ? "#e0f0e0" : "#ffffff"
                        border.color: "#21a69b"
                        border.width: 1

                        Image {
                            anchors.centerIn: parent
                            source: icon
                            sourceSize.width: Math.min(cellBox * 0.65, 50)
                            sourceSize.height: Math.min(cellBox * 0.65, 50)
                            fillMode: Image.PreserveAspectFit
                        }

                        MouseArea {
                            id: appMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: console.log("Home: tapped", name)
                        }
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: name
                        color: "#2d5a2d"
                        font.pixelSize: Math.min(appsGrid.cellWidth * 0.1, 12)
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        // Page 1: Projects
        GridView {
            id: projectsGrid
            cellWidth: Math.floor(width / 3)
            cellHeight: Math.floor(Math.min(cellWidth,
                (swipeView.height - 40) / Math.max(Math.ceil(count / 3), 1)))
            bottomMargin: 40
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            model: ListModel {
                id: projectsModel
            }

            delegate: Item {
                width: projectsGrid.cellWidth
                height: projectsGrid.cellHeight
                readonly property real cellBox: Math.min(projectsGrid.cellWidth * 0.55, 80)

                Column {
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: cellBox
                        height: cellBox
                        radius: Math.min(cellBox * 0.2, 16)
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: projMouse.containsMouse ? "#e0f0e0" : "#ffffff"
                        border.color: "#21a69b"
                        border.width: 1

                        Image {
                            anchors.centerIn: parent
                            source: icon
                            sourceSize.width: Math.min(cellBox * 0.65, 50)
                            sourceSize.height: Math.min(cellBox * 0.65, 50)
                            fillMode: Image.PreserveAspectFit
                        }

                        MouseArea {
                            id: projMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: console.log("Home: tapped project", name)
                        }
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: name
                        color: "#2d5a2d"
                        font.pixelSize: Math.min(projectsGrid.cellWidth * 0.1, 12)
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }

    Row {
        id: devNav
        visible: Config.devMode
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
                color: "#e67e22"
            }

            onClicked: home.goSplash()
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
                color: "#e67e22"
            }

            onClicked: home.goWelcome()
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
                color: swipeView.currentIndex === index ? "#21a69b" : "#cccccc"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: swipeView.currentIndex = index
                }
            }
        }
    }

    Component.onCompleted: {
        var apps = [
            { name: tr("Gamepad"), icon: "qrc:/images/android/pad_button.png" },
            { name: tr("Timeline"), icon: "qrc:/images/android/timeline_button.png" },
            { name: tr("Zowi Says"), icon: "qrc:/images/android/simon_game_button.png" },
            { name: tr("Mouths"), icon: "qrc:/images/android/mouths_game_button.png" },
            { name: tr("Mouths Editor"), icon: "qrc:/images/android/mouths_editor_game_button.png" }
        ]
        for (var i = 0; i < apps.length; i++)
            appsModel.append(apps[i])

        var projects = [
            { name: tr("Move"), icon: "qrc:/images/android/project_move_image.png" },
            { name: tr("Choreography"), icon: "qrc:/images/android/project_choreography_image.png" },
            { name: tr("Form"), icon: "qrc:/images/android/project_form_image.png" },
            { name: tr("Paint"), icon: "qrc:/images/android/project_bio3_image.png" },
            { name: tr("Guess"), icon: "qrc:/images/android/project_adivinawi_image.png" },
            { name: tr("Gravity"), icon: "qrc:/images/android/project_gravity_image.png" },
            { name: tr("Hello World"), icon: "qrc:/images/android/project_helloworld_image.png" },
            { name: tr("Bitbloq"), icon: "qrc:/images/android/project_bitbloq2_image.png" },
            { name: tr("Alarm"), icon: "qrc:/images/android/project_alarm_image.png" }
        ]
        for (var i = 0; i < projects.length; i++)
            projectsModel.append(projects[i])
    }
}
