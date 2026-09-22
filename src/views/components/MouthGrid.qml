// MouthGrid: shared 6×5 LED grid used by the Mouth Editor and the "Pintabocas"
// (Draw the mouths) game. One MouseArea over the whole grid: press toggles a
// cell, drag paints/erases according to the last press state (mirroring the
// Android MouthGridLayout.handleTouch). Consumers react on `patternChanged`
// and read `patternBits` / `matrix` (live-send in the editor, compare in the
// game). `touchEnabled` lets the game forbid drawing outside a round.
import QtQuick 2.15

Item {
    id: root

    readonly property int columns: 6
    readonly property int rows: 5
    // Tamaño de cada píxel de la boca (px). Cambia aquí para hacer la rejilla
    // más grande o más pequeña. Default 50 ≈ 20% mayor que el original (42px).
    property real cellSize: 35
    readonly property int cellSpacing: 6
    // Fondo de la rejilla: tono verde claro distinto del fondo de la app para
    // que se aprecien los píxeles antes de marcarlos.
    property color gridBackground: "#cdeccd"
    property color cellBorderColor: "#000000"
    property int cellBorderWidth: 1
    // Fracción de cada píxel que ocupa el círculo del LED (coincide con el
    // tamaño del píxel blanco en mouths_led_*.png).
    property real ledSizeRatio: 0.75
    // Game only: drawing is allowed while a round is active.
    property bool touchEnabled: true

    // Current draw, kept in sync by recompute() on every change. Cell 0 is the
    // leftmost LED: bit (29 - i) of the 32-bit pattern (the two top bits stay
    // zero, the matrix only has 30 LEDs).
    property string patternBits: ""
    property int matrix: 0

    // Emitted on every change (toggle / clear / select). Consumers rebuild
    // their own state from `patternBits` / `matrix`.
    signal patternChanged()

    // Implicit size: whole grid = cells + spacing, so anchors/centering work
    // without consumers hard-coding the geometry.
    implicitWidth: root.columns * root.cellSize + (root.columns - 1) * root.cellSpacing
    implicitHeight: root.rows * root.cellSize + (root.rows - 1) * root.cellSpacing

    ListModel {
        id: gridModel
    }

    Component.onCompleted: {
        for (var i = 0; i < root.columns * root.rows; i++)
            gridModel.append({ on: false })
        recompute()
    }

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
        recompute()
    }

    // Replica de MouthGridLayout.handleTouch (ZowiAppReborn): al pulsar se
    // alterna el estado; al arrastrar se pinta/borra según el estado previo.
    property var lastItem: null
    property bool lastWasOn: false

    function handleCellPress(index) {
        var nextOn = !gridModel.get(index).on
        setCell(index, nextOn)
        lastItem = index
        lastWasOn = nextOn
    }

    function handleCellDrag(index) {
        if (index < 0 || lastItem === null || index === lastItem)
            return
        if (lastWasOn && !gridModel.get(index).on) {
            setCell(index, true)
        } else if (!lastWasOn && gridModel.get(index).on) {
            setCell(index, false)
        }
        lastItem = index
    }

    function clearAll() {
        for (var i = 0; i < gridModel.count; i++)
            gridModel.setProperty(i, "on", false)
        lastItem = null
        lastWasOn = false
        recompute()
    }

    function selectAll() {
        for (var i = 0; i < gridModel.count; i++)
            gridModel.setProperty(i, "on", true)
        lastItem = null
        lastWasOn = false
        recompute()
    }

    // Recomputes patternBits/matrix and notifies. Called on every model change
    // (setProperty may fire multiple times per gesture, so keep it cheap).
    function recompute() {
        var bits = ""
        var m = 0
        for (var i = 0; i < gridModel.count; i++) {
            if (gridModel.get(i).on) {
                bits += "1"
                m |= (1 << (29 - i)) // celda i → bit (29-i) del patrón de 32
            } else {
                bits += "0"
            }
        }
        patternBits = bits
        matrix = m
        patternChanged()
    }

    Rectangle {
        id: gridRect
        anchors.fill: parent
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

        // Mouse area global sobre la rejilla: dibujo por arrastre. Desactivado
        // por el juego fuera de una ronda (touchEnabled).
        MouseArea {
            id: gridMouseArea
            anchors.fill: parent
            preventStealing: true
            enabled: root.touchEnabled
            onPressed: {
                var idx = cellAt(gridMouseArea.mouseX, gridMouseArea.mouseY)
                if (idx >= 0)
                    handleCellPress(idx)
            }
            onPositionChanged: {
                if (!pressed)
                    return
                var idx = cellAt(gridMouseArea.mouseX, gridMouseArea.mouseY)
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
}