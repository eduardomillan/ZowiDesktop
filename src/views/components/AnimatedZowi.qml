import QtQuick 6.0

/**
 * AnimatedZowi — Componente animado para Zowi basado en los sprites de scratch.
 *
 * Alterna entre sprite1 y sprite2 con un efecto de rebote/respiración
 * y un ligero balanceo para dar sensación de vida.
 *
 * Uso:
 *   AnimatedZowi {
 *       width: 180
 *       height: 180
 *   }
 */
Item {
    id: root

    /* ---- Propiedades públicas ---- */

    /// Intervalo entre cambios de fotograma (ms)
    property int frameInterval: 800

    /// Intensidad del rebote vertical (0.0 = sin rebote)
    property real bounceStrength: 0.06

    /// Amplitud del balanceo en grados
    property real swayAmplitude: 3.0

    /// Duración del ciclo de balanceo (ms)
    property int swayDuration: 2000

    /// Fotograma actual (0 = sprite1, 1 = sprite2)
    property int currentFrame: 0

    /* ---- Estado interno ---- */

    // Factor de escala animado para el rebote (1.0 = normal)
    property real bounceScale: 1.0

    // Factor de escala horizontal para compensar el rebote
    property real squashScale: 1.0

    // Ángulo de balanceo animado
    property real swayAngle: 0.0

    // Ratio de progreso del rebote [0..1]
    property real bounceProgress: 0.0

    // Fase del rebote: 0=expandir, 1=mantener, 2=contraer
    property int bouncePhase: 0

    /* ---- Sprites ---- */

    Image {
        id: sprite1
        anchors.centerIn: parent
        source: "qrc:/images/scratch/sprite1.png"
        sourceSize.width: root.width
        sourceSize.height: root.height
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        visible: root.currentFrame === 0
        opacity: root.currentFrame === 0 ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 180; easing.type: Easing.InOutQuad }
        }
    }

    Image {
        id: sprite2
        anchors.centerIn: parent
        source: "qrc:/images/scratch/sprite2.png"
        sourceSize.width: root.width
        sourceSize.height: root.height
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        visible: root.currentFrame === 1
        opacity: root.currentFrame === 1 ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 180; easing.type: Easing.InOutQuad }
        }
    }

    /* ---- Animaciones ---- */

    // Balanceo continuo (sway)
    SequentialAnimation on swayAngle {
        loops: Animation.Infinite

        NumberAnimation {
            from: -root.swayAmplitude
            to: root.swayAmplitude
            duration: root.swayDuration / 2
            easing.type: Easing.SineCurve
        }
        NumberAnimation {
            from: root.swayAmplitude
            to: -root.swayAmplitude
            duration: root.swayDuration / 2
            easing.type: Easing.SineCurve
        }
    }

    // Temporizador que cambia los fotogramas e inicia el rebote
    Timer {
        id: frameTimer
        interval: root.frameInterval
        running: true
        repeat: true

        onTriggered: {
            // Cambiar sprite
            root.currentFrame = (root.currentFrame + 1) % 2

            // Iniciar ciclo de rebote
            root.bouncePhase = 0;
            root.bounceProgress = 0.0;
            bounceAnimTimer.running = true;
        }
    }

    // Temporizador para interpolar el rebote (~60 fps)
    Timer {
        id: bounceAnimTimer
        interval: 16
        repeat: true

        property int steps: 20

        onTriggered: {
            bounceProgress += 1.0 / steps;

            if (bounceProgress >= 1.0) {
                if (bouncePhase === 0) {
                    // Expandir completado → mantener
                    bouncePhase = 1;
                    bounceProgress = 0.0;
                } else if (bouncePhase === 1) {
                    // Mantener completado → contraer
                    bouncePhase = 2;
                    bounceProgress = 0.0;
                } else {
                    // Ciclo completado
                    bounceAnimTimer.running = false;
                    bounceScale = 1.0;
                    squashScale = 1.0;
                    bounceProgress = 0.0;
                    return;
                }
            }

            // easeInOutQuad personalizado
            var t = bounceProgress;
            var eased;
            if (bouncePhase === 0) {
                // Expandir: easeOutQuad
                eased = 1 - (1 - t) * (1 - t);
                bounceScale = 1.0 + eased * bounceStrength;
                squashScale = 1.0 - eased * bounceStrength * 0.5;
            } else if (bouncePhase === 1) {
                // Mantener en punto máximo
                bounceScale = 1.0 + bounceStrength;
                squashScale = 1.0 - bounceStrength * 0.5;
            } else {
                // Contraer: easeInQuad
                eased = t * t;
                bounceScale = 1.0 + (1.0 - eased) * bounceStrength;
                squashScale = 1.0 - (1.0 - eased) * bounceStrength * 0.5;
            }
        }
    }

    /* ---- Transformaciones aplicadas en orden ---- */

    transform: [
        // 1. Balanceo (rotación alrededor de la base)
        Rotation {
            origin.x: root.width / 2
            origin.y: root.height
            angle: root.swayAngle
        },
        // 2. Rebote (escalado centrado)
        Scale {
            origin.x: root.width / 2
            origin.y: root.height / 2
            xScale: root.squashScale
            yScale: root.bounceScale
        }
    ]

    /* ---- API pública ---- */

    function startAnimation() {
        frameTimer.running = true;
    }

    function stopAnimation() {
        frameTimer.running = false;
        bounceAnimTimer.running = false;
        currentFrame = 0;
        bounceScale = 1.0;
        squashScale = 1.0;
    }
}