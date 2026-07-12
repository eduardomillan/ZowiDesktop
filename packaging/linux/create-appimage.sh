#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
APPDIR="$PROJECT_ROOT/build/AppDir"
TOOLS_DIR="$PROJECT_ROOT/build/.tools"

# Qt6 location — override with: export QT_ROOT_DIR=/path/to/qt6
QT_ROOT_DIR="${QT_ROOT_DIR:-/home/eduardo/Qt/6.5.2/gcc_64}"

echo "=== Step 1: Build project ==="
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_PREFIX_PATH="$QT_ROOT_DIR" \
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
echo "=== Step 5: Run linuxdeploy ==="
TIMESTAMP=$(date +%Y%m%d)
export LDAI_OUTPUT="ZowiDesktop-x86_64-build-${TIMESTAMP}.AppImage"
export DEPLOYQt_VERSION=6
export LINUXDEPLOY_QT_ROOT="$QT_ROOT_DIR"
export QT_PLUGIN_PATH="$QT_ROOT_DIR/plugins"
export QML2_IMPORT_PATH="$QT_ROOT_DIR/qml"
export QMAKE="$QT_ROOT_DIR/bin/qmake6"
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
echo "=== Step 6: Ensure Qt6 platform plugins are bundled ==="
# linuxdeploy-plugin-qt may miss Qt6 plugins; copy them manually if needed
QT_PLATFORMS="$APPDIR/usr/plugins/platforms"
if [ ! -f "$QT_PLATFORMS/libqxcb.so" ]; then
    echo "   libqxcb.so not found in AppDir, copying from Qt6..."
    mkdir -p "$QT_PLATFORMS"
    cp "$QT_ROOT_DIR/plugins/platforms/libqxcb.so" "$QT_PLATFORMS/"

    # xcbglintegrations
    mkdir -p "$APPDIR/usr/plugins/xcbglintegrations"
    cp "$QT_ROOT_DIR/plugins/xcbglintegrations/"*.so "$APPDIR/usr/plugins/xcbglintegrations/"

    # imageformats
    mkdir -p "$APPDIR/usr/plugins/imageformats"
    cp "$QT_ROOT_DIR/plugins/imageformats/"*.so "$APPDIR/usr/plugins/imageformats/"

    # platformsupport (Qt6 might need this)
    if [ -d "$QT_ROOT_DIR/plugins/platformsupport" ]; then
        mkdir -p "$APPDIR/usr/plugins/platformsupport"
        cp "$QT_ROOT_DIR/plugins/platformsupport/"*.so "$APPDIR/usr/plugins/platformsupport/" 2>/dev/null || true
    fi

    # tls
    if [ -d "$QT_ROOT_DIR/plugins/tls" ]; then
        mkdir -p "$APPDIR/usr/plugins/tls"
        cp "$QT_ROOT_DIR/plugins/tls/"*.so "$APPDIR/usr/plugins/tls/" 2>/dev/null || true
    fi

    # Copy xcb dependencies
    for lib in libxcb-cursor.so.0 libxcb-icccm.so.4 libxcb-image.so.0 libxcb-keysyms.so.1 libxcb-render-util.so.0 libxcb-shm.so.0 libxcb-util.so.1 libxcb-xinerama.so.0 libxcb-xkb.so.1; do
        found=$(find "$QT_ROOT_DIR/lib" /usr/lib/x86_64-linux-gnu -name "$lib" 2>/dev/null | head -1)
        if [ -n "$found" ]; then
            cp "$found" "$APPDIR/usr/lib/" 2>/dev/null || true
        fi
    done

    # xkbcommon
    for lib in libxkbcommon.so.0 libxkbcommon-x11.so.0; do
        found=$(find "$QT_ROOT_DIR/lib" /usr/lib/x86_64-linux-gnu -name "$lib" 2>/dev/null | head -1)
        if [ -n "$found" ]; then
            cp "$found" "$APPDIR/usr/lib/" 2>/dev/null || true
        fi
    done
fi

echo ""
echo "=== Moving AppImage to build directory ==="
mv "$PROJECT_ROOT/ZowiDesktop-x86_64-build-${TIMESTAMP}.AppImage" "$PROJECT_ROOT/build/" 2>/dev/null || true

echo ""
echo "=== Done ==="
ls -lh "$BUILD_DIR/ZowiDesktop-x86_64-build-${TIMESTAMP}.AppImage"
