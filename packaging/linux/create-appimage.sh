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
echo "=== Step 6: Run linuxdeploy ==="
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
echo "=== Moving AppImage to build directory ==="
mv "$PROJECT_ROOT/ZowiDesktop-x86_64-build-${TIMESTAMP}.AppImage" "$PROJECT_ROOT/build/" 2>/dev/null || true

echo ""
echo "=== Done ==="
ls -lh "$BUILD_DIR/ZowiDesktop-x86_64-build-${TIMESTAMP}.AppImage"
