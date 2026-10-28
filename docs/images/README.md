# Reference images (not shipped)

Images kept for reference that the app does **not** use. They were moved here
from `images/` because every PNG/JPG under `images/<dir>/` is embedded in the
GUI executable through the auto-generated `images.qrc` (see
`src/gui/CMakeLists.txt` and `cmake/GenerateQrc.cmake`); keeping unused files
there made the binary ~17 MB larger for nothing.

The folder structure mirrors `images/`, so an image keeps the same relative
path in both places.

| Folder | Content |
|---|---|
| `android/` | Assets from the original Android app not (yet) used by any screen: buttons, icons and badges for movements, modes, menus, etc., plus the GIMP source `repeat_X_button.xcf` of the timeline repeat buttons. |
| `android/androidout.com/` | Store listing screenshots and icon of the original Android app. |
| `legacyapp/` | Screenshots of the original Android app, used as a visual reference when rebuilding screens. |
| `components/` | Photos and diagrams of the robot hardware (board, sensors, exploded view). |
| `robot/` | Photos of Zowi doing things (dancing, walking, alarm…) and the original product picture. |
| `misc/` | Other photos of Zowi. |
| `modes/` | Mode A / B / AB illustrations. |
| `projects/` | Project pictures not referenced by any `projects/<id>/project.json`. |
| `sounds/` | Sound button variant not used by the gamepad. |

## Using one of these images in the app

Move it back with `git mv docs/images/<path> images/<path>`, reference it as
`qrc:/images/<path>` from QML, and rebuild (`./build.sh --gui`): the
`images.qrc` file is regenerated automatically.

## Adding images

Put an image in `images/` only when the code references it. Reference
material (screenshots, photos, unused assets) goes here.
