// SplashScreen: Initial screen shown on app launch.
// Displays the Zowi logo and provides Continue/Quit buttons
// along with language selector flags (ES, CA, EN).
import QtQuick 6.0
import QtQuick.Controls 6.0
import "../components"

Rectangle {
    id: splash
    color: "#f4f9f4"

    signal splashFinished()
    signal quitRequested()

    property string screenName: "SplashScreen"

    function tr(source) { return Translator.translate("SplashScreen.qml", source) }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -60
        spacing: 20

        AnimatedZowi {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 180
            height: 180
            frameInterval: 1200
            bounceStrength: 0.07
            swayAmplitude: 4
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("ZOWI")
            color: "#2d5a2d"
            font.pixelSize: 48
            font.bold: true
            font.family: "monospace"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: tr("Desktop")
            color: "#2d5a2d"
            font.pixelSize: 18
            opacity: 0.7
        }
    }

    Row {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 150
        spacing: 20

        Button {
            id: continueButton
            implicitWidth: 200
            height: 50
            text: tr("Continuar")

            contentItem: Text {
                text: parent.text
                font.pixelSize: 18
                font.bold: true
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 25
                color: continueButton.pressed ? "#17736c" : "#21a69b"
            }

            onClicked: splash.splashFinished()
        }

        Button {
            id: quitButton
            implicitWidth: 200
            height: 50
            text: tr("Salir")

            contentItem: Text {
                text: parent.text
                font.pixelSize: 14
                font.bold: true
                color: "#2d5a2d"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                opacity: 0.8
            }

            background: Rectangle {
                radius: 25
                color: "transparent"
                border.color: "#2d5a2d"
                border.width: 2
                opacity: 0.5
            }

            onClicked: splash.quitRequested()
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: flagsRow.top
        anchors.bottomMargin: 10
        text: tr("Selecciona idioma")
        color: "#2d5a2d"
        font.pixelSize: 13
        opacity: 0.6
    }

    Row {
        id: flagsRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        spacing: 20

        Repeater {
            model: [
                { flag: "qrc:/docs/images/es.svg", locale: "es_ES" },
                { flag: "qrc:/docs/images/vl.svg", locale: "ca_ES" },
                { flag: "qrc:/docs/images/gb.svg", locale: "en_US" }
            ]

            delegate: Item {
                width: 40
                height: 40

                Image {
                    anchors.centerIn: parent
                    source: modelData.flag
                    sourceSize.width: 28
                    sourceSize.height: 28
                    fillMode: Image.PreserveAspectFit
                    opacity: Translator.currentLocale() === modelData.locale ? 1.0 : 0.4
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (Translator.currentLocale() !== modelData.locale) {
                            Translator.load(modelData.locale)
                        }
                    }
                }
            }
        }
    }
}
