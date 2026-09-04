// MouthEditorScreen: pintabocas. Draw Zowi's mouth dot-by-dot on a 6x5 LED
// grid; every change is sent live to the robot as an L command (same wire
// semantics as ZowiAppReborn's MouthsEditor: 30 grid cells become the 32-bit
// binary pattern "L 00<30bits>\r"). Footer offers "clear all" / "select all".
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"

ScreenTemplate {
    id: root
    screenName: "MouthEditorScreen"
    title: tr("title")
    subtitle: tr("subtitle")
    showBackButton: true
    footerHeight: 72

    // The identity poll (E/I/B burst) drains the robot's command queue and can
    // delay/interrupt the live mouth updates — pause it while editing (handled
    // in the root Component.onCompleted below, alongside grid initialization).

    function tr(source) { return Translator.translate("MouthEditorScreen.qml", source) }

    readonly property int columns: 6
    readonly property int rows: 5
    // Tamaño de cada píxel de la boca (px). Cambia aquí para hacer la rejilla
    // más grande o más pequeña. Default 50 ≈ 20% mayor que el original (42px).
    property real cellSize: 50
    readonly property int cellSpacing: 8
    // Fondo de la rejilla: tono verde claro distinto del fondo de la app para
    // que se aprecien los píxeles antes de marcarlos.
    property color gridBackground: "#cdeccd"
    property color cellBorderColor: "#000000"
    property int cellBorderWidth: 1
    // Fracción de cada píxel que ocupa el círculo del LED (coincide con el
    // tamaño del píxel blanco en mouths_led_*.png).
    property real ledSizeRatio: 0.75

    property var lastItem: null
    property bool lastWasOn: false

    ListModel {
        id: gridModel
    }

    Component.onCompleted: {
        Robot.setDataPollingEnabled(false)
        for (var i = 0; i < root.columns * root.rows; i++)
            gridModel.append({ on: false })
    }
    Component.onDestruction: Robot.setDataPollingEnabled(true)

    function gridIndex(column, row) { return row * root.columns + column }

    function cellAt(x, y) {
        var col = Math.floor(x / (root.cellSize + root.cellSpacing))
        var row = Math.floor(y / (root.cellSize + root.cellSpacing))
        if (col < 0 || col >= root.columns || row < 0 || row >= root.rows)
            return -1
        return gridIndex(col, row)
    }

    function setCell(index, on) {
        if (index < 0 || index >= gridModel.count)
            return
        gridModel.setProperty(index, "on", on)
    }

    // Replica de MouthGridLayout.handleTouch (ZowiAppReborn): al pulsar se
    // alterna el estado; al arrastrar se pinta/borra según el estado previo.
    function handleCellPress(index) {
        var nextOn = !gridModel.get(index).on
        setCell(index, nextOn)
        lastItem = index
        lastWasOn = nextOn
        sendGrid()
    }

    function handleCellDrag(index) {
        if (index < 0 || lastItem === null || index === lastItem)
            return
        var changed = false
        if (lastWasOn && !gridModel.get(index).on) {
            setCell(index, true)
            changed = true
        } else if (!lastWasOn && gridModel.get(index).on) {
            setCell(index, false)
            changed = true
        }
        lastItem = index
        if (changed)
            sendGrid()
    }

    function sendGrid() {
        var matrix = 0
        for (var i = 0; i < gridModel.count; i++) {
            if (gridModel.get(i).on)
                matrix |= (1 << (29 - i)) // celda i → bit (29-i) del patrón de 32
        }
        if (Robot.connected)
            Robot.sendData(Commands.mouth(matrix))
    }

    function clearAll() {
        for (var i = 0; i < gridModel.count; i++)
            gridModel.setProperty(i, "on", false)
        lastItem = null
        lastWasOn = false
        sendGrid()
    }

    function selectAll() {
        for (var i = 0; i < gridModel.count; i++)
            gridModel.setProperty(i, "on", true)
        lastItem = null
        lastWasOn = false
        sendGrid()
    }

    Rectangle {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 24
        }
        width: root.columns * root.cellSize + (root.columns - 1) * root.cellSpacing
        height: root.rows * root.cellSize + (root.rows - 1) * root.cellSpacing
        color: root.gridBackground
        radius: 12
        border.color: Config.get("color_primary") || "#2d5a2d"
        border.width: 2

        Grid {
            anchors.fill: parent
            rows: root.rows
            columns: root.columns
            rowSpacing: root.cellSpacing
            columnSpacing: root.cellSpacing

            Repeater {
                model: gridModel

                delegate: Item {
                    width: root.cellSize
                    height: root.cellSize

                    // Borde circular que coincide con el tamaño del píxel
                    // blanco del LED (el círculo del PNG es ~75% del píxel).
                    Rectangle {
                        anchors.centerIn: parent
                        width: root.cellSize * root.ledSizeRatio
                        height: root.cellSize * root.ledSizeRatio
                        radius: width / 2
                        color: "transparent"
                        border.color: root.cellBorderColor
                        border.width: root.cellBorderWidth

                        Image {
                            anchors.fill: parent
                            source: model.on
                                    ? "qrc:/images/android/mouths_led_on.png"
                                    : "qrc:/images/android/mouths_led_off.png"
                            sourceSize: Qt.size(root.cellSize * 2, root.cellSize * 2)
                            fillMode: Image.PreserveAspectFit
                        }
                    }
                }
            }
        }

        // Mouse area global sobre la rejilla: dibujo por arrastre.
        MouseArea {
            anchors.fill: parent
            preventStealing: true
            onPressed: {
                var idx = cellAt(mouse.x, mouse.y)
                if (idx >= 0)
                    handleCellPress(idx)
            }
            onPositionChanged: {
                if (!pressed)
                    return
                var idx = cellAt(mouse.x, mouse.y)
                if (idx < 0) {
                    // Fuera de la rejilla: se reinicia el arrastre, como en Android.
                    lastItem = null
                    lastWasOn = false
                    return
                }
                handleCellDrag(idx)
            }
            onReleased: {
                lastItem = null
                lastWasOn = false
            }
        }
    }

    footer: Row {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 14
        }
        spacing: 20

        Button {
            id: clearBtn
            implicitWidth: 170
            height: 44
            anchors.verticalCenter: parent.verticalCenter
            text: root.tr("clear_all")

            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.bold: true
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 22
                color: clearBtn.pressed ? Config.get("color_error") || "#c0392b" : Config.get("color_danger") || "#e74c3c"
            }

            onClicked: {
                console.log("[MouthEditorScreen] clear all")
                root.clearAll()
            }
        }

        Button {
            id: selectAllBtn
            implicitWidth: 170
            height: 44
            anchors.verticalCenter: parent.verticalCenter
            text: root.tr("select_all")

            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.bold: true
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 22
                color: selectAllBtn.pressed ? Config.get("color_primary_pressed") || "#1f4a1f" : Config.get("color_primary") || "#2d5a2d"
            }

            onClicked: {
                console.log("[MouthEditorScreen] select all")
                root.selectAll()
            }
        }
    }
}