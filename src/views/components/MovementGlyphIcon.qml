import QtQuick 2.15

Canvas {
    id: root
    property string glyph: "walk"    // "walk" | "turn" | "bend" | "shakeLeg" | "swing" | "moonwalk"
    property bool mirrored: false
    property color glyphColor: "#ffffff"

    implicitWidth: 100
    implicitHeight: 100
    antialiasing: true

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.clearRect(0, 0, width, height)
        ctx.strokeStyle = root.glyphColor
        ctx.fillStyle = root.glyphColor
        ctx.lineCap = "round"
        ctx.lineJoin = "round"
        ctx.lineWidth = width * 0.08

        ctx.save()
        ctx.translate(width / 2, height / 2)
        if (root.mirrored) ctx.scale(-1, 1)

        switch (root.glyph) {
        case "walk": drawWalk(ctx); break
        case "turn": drawTurn(ctx); break
        case "bend": drawBend(ctx); break
        case "shakeLeg": drawShakeLeg(ctx); break
        case "swing": drawSwing(ctx); break
        case "moonwalk": drawMoonwalk(ctx); break
        }

        ctx.restore()
    }

    function drawWalk(ctx) {
        var s = width * 0.15
        ctx.beginPath()
        ctx.ellipse(-s * 1.2, -s * 0.8, s, s * 1.5)
        ctx.fill()
        ctx.beginPath()
        ctx.ellipse(s * 0.2, s * 0.5, s, s * 1.5)
        ctx.fill()
    }

    function drawTurn(ctx) {
        var r = width * 0.25
        ctx.beginPath()
        ctx.arc(0, 0, r, 0, Math.PI * 1.5, false)
        ctx.stroke()
        var arrowX = r * Math.cos(-Math.PI / 4)
        var arrowY = r * Math.sin(-Math.PI / 4)
        ctx.beginPath()
        ctx.moveTo(arrowX, arrowY)
        ctx.lineTo(arrowX - 8, arrowY + 8)
        ctx.lineTo(arrowX + 5, arrowY + 3)
        ctx.closePath()
        ctx.fill()
    }

    function drawBend(ctx) {
        var headR = width * 0.12
        ctx.beginPath()
        ctx.arc(0, -width * 0.15, headR, 0, Math.PI * 2)
        ctx.fill()
        ctx.beginPath()
        ctx.moveTo(-width * 0.1, -width * 0.02)
        ctx.quadraticCurveTo(0, width * 0.15, width * 0.1, width * 0.25)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(-width * 0.2, width * 0.28)
        ctx.lineTo(width * 0.2, width * 0.28)
        ctx.stroke()
    }

    function drawShakeLeg(ctx) {
        var y = -width * 0.15
        ctx.beginPath()
        ctx.moveTo(-width * 0.15, y)
        ctx.lineTo(-width * 0.05, y + width * 0.1)
        ctx.lineTo(width * 0.05, y)
        ctx.lineTo(width * 0.15, y + width * 0.1)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(-width * 0.15, y + width * 0.15)
        ctx.lineTo(-width * 0.05, y + width * 0.25)
        ctx.lineTo(width * 0.05, y + width * 0.15)
        ctx.lineTo(width * 0.15, y + width * 0.25)
        ctx.stroke()
    }

    function drawSwing(ctx) {
        var pivotY = -width * 0.2
        ctx.beginPath()
        ctx.moveTo(0, pivotY)
        ctx.lineTo(0, pivotY + width * 0.3)
        ctx.stroke()
        var ballR = width * 0.08
        ctx.beginPath()
        ctx.arc(0, pivotY + width * 0.3, ballR, 0, Math.PI * 2)
        ctx.fill()
        var arcR = width * 0.25
        ctx.beginPath()
        ctx.arc(0, pivotY, arcR, -Math.PI / 6, Math.PI / 6, false)
        ctx.stroke()
    }

    function drawMoonwalk(ctx) {
        var s = width * 0.15
        ctx.beginPath()
        ctx.ellipse(-s * 1.2, -s * 0.8, s, s * 1.5)
        ctx.fill()
        ctx.beginPath()
        ctx.ellipse(s * 0.2, s * 0.5, s, s * 1.5)
        ctx.fill()
        for (var i = 0; i < 3; i++) {
            var offset = width * 0.08 * (i + 1)
            ctx.beginPath()
            ctx.moveTo(-offset, -width * 0.05)
            ctx.lineTo(-offset, width * 0.05)
            ctx.stroke()
        }
    }
}
