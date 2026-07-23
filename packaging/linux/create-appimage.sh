#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
APPDIR="$PROJECT_ROOT/build/AppDir"
TOOLS_DIR="$PROJECT_ROOT/build/.tools"
VERSION=$(grep -oP 'project\(ZowiDesktop\s+VERSION\s+\K\S+(?=\s+LANGUAGES)' "$PROJECT_ROOT/CMakeLists.txt")

# Qt6 location — override with: export QT_ROOT_DIR=/path/to/qt6
QT_ROOT_DIR="${QT_ROOT_DIR:-/home/eduardo/Qt/6.5.2/gcc_64}"

# Packaged builds ship with dev mode OFF. It can be re-enabled at runtime
# via the ZOWI_DEV environment variable.
sed -i -E 's/("dev_mode"\s*:\s*")[^"]*(")/\1false\2/' "$PROJECT_ROOT/src/config.json"

echo "=== Step 1: Build project ==="
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_PREFIX_PATH="$QT_ROOT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$BUILD_DIR"

echo ""
echo "=== Step 2: Install to AppDir ==="
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR"

echo ""
echo "=== Step 3: Prepare AppDir resources ==="
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

cp "$PROJECT_ROOT/packaging/linux/zowi-desktop.desktop" "$APPDIR/usr/share/applications/"
cp "$PROJECT_ROOT/images/app_icon.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/zowi-desktop.png"
cp "$PROJECT_ROOT/images/app_icon.png" "$APPDIR/zowi-desktop.png"

echo ""
echo "=== Step 4: Download linuxdeploy ==="
mkdir -p "$TOOLS_DIR"
if [ ! -f "$TOOLS_DIR/linuxdeploy-x86_64.AppImage" ]; then
    wget -q -O "$TOOLS_DIR/linuxdeploy-x86_64.AppImage" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
fi
if [ ! -f "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    wget -q -O "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
fi

echo ""
echo "=== Step 5: Bundle Qt platform plugins (xcb + wayland) ==="
# Ensure both the X11 (xcb) and Wayland platform plugins plus their
# dependencies are present in the AppDir BEFORE linuxdeploy builds the
# AppImage, so it runs on X11 and on native Wayland sessions alike.
QT_PLATFORMS="$APPDIR/usr/plugins/platforms"
mkdir -p "$QT_PLATFORMS" "$APPDIR/usr/lib"

copy_lib() {
    local name="$1"
    local dest="$APPDIR/usr/lib"
    [ -e "$dest/$name" ] && return
    local f
    for f in $(find "$QT_ROOT_DIR/lib" /lib /usr/lib /usr/lib/x86_64-linux-gnu \
                  -name "${name}*" 2>/dev/null); do
        cp -a "$f" "$dest/" 2>/dev/null || true
    done
}

# X11 (xcb) — only if linuxdeploy has not already provided it
if [ ! -f "$QT_PLATFORMS/libqxcb.so" ]; then
    echo "   libqxcb.so not found, copying from Qt6..."
    cp "$QT_ROOT_DIR/plugins/platforms/libqxcb.so" "$QT_PLATFORMS/"
    mkdir -p "$APPDIR/usr/plugins/xcbglintegrations"
    cp "$QT_ROOT_DIR/plugins/xcbglintegrations/"*.so "$APPDIR/usr/plugins/xcbglintegrations/" 2>/dev/null || true
    mkdir -p "$APPDIR/usr/plugins/imageformats"
    cp "$QT_ROOT_DIR/plugins/imageformats/"*.so "$APPDIR/usr/plugins/imageformats/" 2>/dev/null || true
    if [ -d "$QT_ROOT_DIR/plugins/platformsupport" ]; then
        mkdir -p "$APPDIR/usr/plugins/platformsupport"
        cp "$QT_ROOT_DIR/plugins/platformsupport/"*.so "$APPDIR/usr/plugins/platformsupport/" 2>/dev/null || true
    fi
    if [ -d "$QT_ROOT_DIR/plugins/tls" ]; then
        mkdir -p "$APPDIR/usr/plugins/tls"
        cp "$QT_ROOT_DIR/plugins/tls/"*.so "$APPDIR/usr/plugins/tls/" 2>/dev/null || true
    fi
    for lib in libxcb-cursor.so.0 libxcb-icccm.so.4 libxcb-image.so.0 libxcb-keysyms.so.1 libxcb-render-util.so.0 libxcb-shm.so.0 libxcb-util.so.1 libxcb-xinerama.so.0 libxcb-xkb.so.1; do
        copy_lib "$lib"
    done
fi

# Wayland
echo "   Ensuring Wayland platform plugin is bundled..."
for wp in libqwayland-generic.so libqwayland-egl.so; do
    if [ ! -f "$QT_PLATFORMS/$wp" ]; then
        cp "$QT_ROOT_DIR/plugins/platforms/$wp" "$QT_PLATFORMS/" 2>/dev/null || true
    fi
done
for wl in libQt6WaylandClient.so.6 libQt6WaylandEglClientHwIntegration.so.6; do
    copy_lib "$wl"
done
for wl in libwayland-client.so.0 libwayland-cursor.so.0 libwayland-egl.so.1 libffi.so.8 libxkbcommon.so.0; do
    copy_lib "$wl"
done

echo ""
echo "=== Step 5b: Bundle essential QtQml modules ==="
# linuxdeploy-plugin-qt bundles QML modules by scanning imports, but it can
# miss modules that are only pulled in *transitively* (e.g. QtQml.WorkerScript,
# required by ListModel/ListView). If they are absent, the QML engine aborts
# with `module "QtQml.WorkerScript" is not installed` and the app never starts
# on machines that have no system-wide Qt QML modules to fall back on.
QML_DEST="$APPDIR/usr/qml"
mkdir -p "$QML_DEST/QtQml"

# Locate the Qt QML import directory. On apt-based Qt (used in CI) the modules
# live under $QT_ROOT_DIR/qml, but the exact prefix can vary, so fall back to a
# recursive search for the QtQml/qmldir marker if the expected path is missing.
QML_SRC_ROOT="$QT_ROOT_DIR/qml"
if [ ! -f "$QML_SRC_ROOT/QtQml/qmldir" ]; then
    found_qmldir="$(find "$QT_ROOT_DIR" /usr/lib -maxdepth 6 -path '*/qml/QtQml/qmldir' 2>/dev/null | head -n1)"
    if [ -n "$found_qmldir" ]; then
        QML_SRC_ROOT="$(dirname "$(dirname "$found_qmldir")")"
    fi
fi
echo "   Using QML import root: $QML_SRC_ROOT"

# QtQml top-level qmldir (declares QtQml.Base) must be present.
[ -f "$QML_SRC_ROOT/QtQml/qmldir" ] && cp -a "$QML_SRC_ROOT/QtQml/qmldir" "$QML_DEST/QtQml/" 2>/dev/null || true
# Any files that ship directly under QtQml/ (Base module lives here on apt Qt).
for f in "$QML_SRC_ROOT/QtQml/"*; do
    [ -f "$f" ] && cp -a "$f" "$QML_DEST/QtQml/" 2>/dev/null || true
done
for mod in Base Models WorkerScript XmlListModel; do
    src="$QML_SRC_ROOT/QtQml/$mod"
    if [ -d "$src" ] && [ ! -d "$QML_DEST/QtQml/$mod" ]; then
        echo "   Copying QtQml/$mod ..."
        cp -a "$src" "$QML_DEST/QtQml/"
    fi
done

echo ""
echo "=== Step 6: Run linuxdeploy ==="
APPIMAGE_NAME="${APPIMAGE_NAME:-ZowiDesktop-${VERSION}-x86_64.AppImage}"
export LDAI_OUTPUT="$APPIMAGE_NAME"
export DEPLOYQt_VERSION=6
export LINUXDEPLOY_QT_ROOT="$QT_ROOT_DIR"
export QT_PLUGIN_PATH="$QT_ROOT_DIR/plugins"
export QML2_IMPORT_PATH="$QT_ROOT_DIR/qml"
if [ -x "$QT_ROOT_DIR/bin/qmake6" ]; then
    export QMAKE="$QT_ROOT_DIR/bin/qmake6"
else
    export QMAKE="$(command -v qmake6 || command -v qmake)"
fi
export PATH="$QT_ROOT_DIR/bin:$PATH"
export QML_SOURCES_PATHS="$PROJECT_ROOT/src/views"
export EXTRA_QT_MODULES="qml;quick"
export LD_LIBRARY_PATH="$QT_ROOT_DIR/lib:$QT_ROOT_DIR/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"

"$TOOLS_DIR/linuxdeploy-x86_64.AppImage" \
    --appdir "$APPDIR" \
    --plugin qt \
    --desktop-file "$APPDIR/usr/share/applications/zowi-desktop.desktop" \
    --icon-file "$APPDIR/zowi-desktop.png" \
    --output appimage

echo ""
echo "=== Step 6b: Verify critical QML modules are bundled ==="
# Fail loudly instead of shipping an AppImage that cannot start.
QTQML_QMLDIR="$APPDIR/usr/qml/QtQml/qmldir"
MISSING=""
# QtQml.Base is the implicit base module: on the apt Qt layout it is provided by
# the top-level QtQml module itself (usr/qml/QtQml/qmldir + libqmlplugin.so),
# not by a QtQml/Base subdirectory. Treat it as present when that qmldir exists.
if [ ! -f "$QTQML_QMLDIR" ] && [ ! -f "$APPDIR/usr/qml/QtQml/Base/qmldir" ]; then
    MISSING="$MISSING QtQml.Base"
fi
# QtQml.WorkerScript must be bundled as its own module directory.
if [ ! -f "$APPDIR/usr/qml/QtQml/WorkerScript/qmldir" ]; then
    if ! { [ -f "$QTQML_QMLDIR" ] && grep -qi "WorkerScript" "$QTQML_QMLDIR"; }; then
        MISSING="$MISSING QtQml.WorkerScript"
    fi
fi
if [ -n "$MISSING" ]; then
    echo "ERROR: required QML module(s) missing from AppDir:$MISSING" >&2
    echo "--- Contents of $APPDIR/usr/qml/QtQml ---" >&2
    ls -la "$APPDIR/usr/qml/QtQml" 2>&1 >&2 || true
    exit 1
fi
echo "   OK: QtQml.Base and QtQml.WorkerScript present."

echo ""
echo "=== Moving AppImage to build directory ==="
mv "$PROJECT_ROOT/$APPIMAGE_NAME" "$PROJECT_ROOT/build/" 2>/dev/null || true

echo ""
# Restore dev mode for the development tree so it is not left OFF after packaging
git checkout -- src/config.json 2>/dev/null || true

echo "=== Done ==="
ls -lh "$BUILD_DIR/$APPIMAGE_NAME"
