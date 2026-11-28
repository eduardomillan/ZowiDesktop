// Small rounded flag drawn in code (no image files): one per supported locale.
// Simplified on purpose; at ~24 px only the colours and the layout matter.
import QtQuick 2.15

Canvas {
    id: flag
    property string locale: "en_US"
    property color border: "#00000033"

    width: 24
    height: 16
    onLocaleChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        var w = width, h = height, r = 3

        function rrect() {
            ctx.beginPath()
            ctx.moveTo(r, 0); ctx.lineTo(w - r, 0); ctx.quadraticCurveTo(w, 0, w, r)
            ctx.lineTo(w, h - r); ctx.quadraticCurveTo(w, h, w - r, h)
            ctx.lineTo(r, h); ctx.quadraticCurveTo(0, h, 0, h - r)
            ctx.lineTo(0, r); ctx.quadraticCurveTo(0, 0, r, 0)
            ctx.closePath()
        }
        function rect(c, x, y, ww, hh) { ctx.fillStyle = c; ctx.fillRect(x, y, ww, hh) }

        ctx.save()
        rrect(); ctx.clip()

        if (locale === "es_ES") {                      // red / yellow / red (1:2:1)
            rect("#c60b1e", 0, 0, w, h)
            rect("#ffc400", 0, h / 4, w, h / 2)
        } else if (locale === "ca_ES") {               // Valencian senyera: 9 stripes + blue band
            var sh = h / 9
            for (var i = 0; i < 9; ++i)
                rect(i % 2 === 0 ? "#fcdd09" : "#da121a", 0, i * sh, w, sh + 0.5)
            rect("#0b4ea2", 0, 0, w * 0.27, h)
        } else if (locale === "fr_FR") {               // blue / white / red, vertical
            rect("#0055a4", 0, 0, w / 3, h)
            rect("#ffffff", w / 3, 0, w / 3, h)
            rect("#ef4135", 2 * w / 3, 0, w / 3, h)
        } else if (locale === "bg_BG") {               // white / green / red, horizontal
            rect("#ffffff", 0, 0, w, h / 3)
            rect("#00966e", 0, h / 3, w, h / 3)
            rect("#d62612", 0, 2 * h / 3, w, h / 3)
        } else {                                       // English: Union Jack
            rect("#012169", 0, 0, w, h)
            ctx.lineCap = "butt"
            function diag(color, lw) {
                ctx.strokeStyle = color; ctx.lineWidth = lw
                ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(w, h)
                ctx.moveTo(w, 0); ctx.lineTo(0, h); ctx.stroke()
            }
            diag("#ffffff", h * 0.28)
            diag("#c8102e", h * 0.10)
            rect("#ffffff", 0, h * 0.35, w, h * 0.30)
            rect("#ffffff", w * 0.41, 0, w * 0.18, h)
            rect("#c8102e", 0, h * 0.42, w, h * 0.16)
            rect("#c8102e", w * 0.45, 0, w * 0.10, h)
        }
        ctx.restore()

        // thin outline so white areas (FR, BG, EN) do not melt into the background
        ctx.strokeStyle = border
        ctx.lineWidth = 1
        ctx.translate(0.5, 0.5)
        w -= 1; h -= 1
        rrect(); ctx.stroke()
    }
}
