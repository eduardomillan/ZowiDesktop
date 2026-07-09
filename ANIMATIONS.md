# Animation formats supported in QML

| Type | Format | How to use |
|------|--------|------------|
| **Animated GIF** | `.gif` | `AnimatedImage { source: "file.gif" }` — plays automatically |
| **Sprite sheet** | `.png`, `.jpg` (single strip with frames) | `SpriteSequence` / `AnimatedSprite` — ideal for character animations |
| **Individual frames** | `.png`, `.jpg` | `Timer` + switch `Image.source` (what `AnimatedZowi.qml` already does) |
| **SVG** | `.svg` | `Image { source: "file.svg" }` — scalable, no CSS animation |
| **Animated WEBP** | `.webp` | Limited support in Qt 5.15, better in Qt 6 |
| **Lottie / JSON** | `.json` | Requires external library (lottie-qml) |

## Recommended approaches for SplashScreen

- **Sprite sheet**: export your animation as a single PNG strip and use `AnimatedSprite`. Most efficient and clean.
- **Animated GIF**: simplest — just switch to `AnimatedImage` instead of `Image`, but limited quality.
- **Keep using `AnimatedZowi.qml`** (frame switching with `Timer`): works well and is already in place.
