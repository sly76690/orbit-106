#!/bin/bash
# Builds the macOS installer:  build/Orbit106-<version>.pkg
# It installs the AU and VST3 plug-ins, the standalone application, the rendered artwork and the sounds (Presets/Sounds) for every user.
# Usage:  bash tools/paquet.sh        (after a build and a final render: bash tools/rendu.sh final)
set -e
cd "$(dirname "$0")/.."
VERSION=$(sed -n 's/^project(Orbit106 VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt); VERSION=${VERSION:-0.1.0}
OUT="build/Orbit106_artefacts/Release"
SKIN="$HOME/Library/Application Support/SF Instruments/Orbit 106/Skin"
STAGE="build/pkg-root"; PKG="build/Orbit106-$VERSION.pkg"

for item in "$OUT/AU/Orbit 106.component" "$OUT/VST3/Orbit 106.vst3" "$OUT/Standalone/Orbit 106.app"; do
  [ -e "$item" ] || { echo "Manquant : $item — compile d'abord (cmake --build build)."; exit 1; }
done
for image in background.jpg knob.png fader.png led.png index_amber.png index_teal.png index_neutral.png layout.rendered.json; do
  [ -f "$SKIN/$image" ] || { echo "Rendu manquant : $image — lance d'abord  bash tools/rendu.sh final"; exit 1; }
done
if ! diff -q <(tr -d '\r' < "$SKIN/layout.json") <(tr -d '\r' < "$SKIN/layout.rendered.json") >/dev/null; then
  echo "Le rendu ne correspond plus à la disposition actuelle : ouvre Orbit une fois, puis  bash tools/rendu.sh final"; exit 1
fi

rm -rf "$STAGE"; rm -f "$PKG"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/Components" "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Applications" \
         "$STAGE/Library/Application Support/SF Instruments/Orbit 106/Skin"
ditto "$OUT/AU/Orbit 106.component" "$STAGE/Library/Audio/Plug-Ins/Components/Orbit 106.component"
ditto "$OUT/VST3/Orbit 106.vst3" "$STAGE/Library/Audio/Plug-Ins/VST3/Orbit 106.vst3"
ditto "$OUT/Standalone/Orbit 106.app" "$STAGE/Applications/Orbit 106.app"
for image in background.jpg knob.png fader.png led.png index_amber.png index_teal.png index_neutral.png layout.rendered.json; do
  cp "$SKIN/$image" "$STAGE/Library/Application Support/SF Instruments/Orbit 106/Skin/"
done
# The sounds shipped with Orbit (menu SOUNDS), from the project's Presets/Sounds folder.
SOUNDS="$STAGE/Library/Application Support/SF Instruments/Orbit 106/Presets/Sounds"; mkdir -p "$SOUNDS"
count=0; for sound in Presets/Sounds/*.orbitpreset; do [ -f "$sound" ] && cp "$sound" "$SOUNDS/" && count=$((count + 1)); done
echo "Sons inclus : $count"
find "$STAGE" -name ".DS_Store" -delete
xattr -cr "$STAGE"
chmod -R u+w,go+rX "$STAGE"   # pkgbuild complains about files it cannot write (read-only copies)

# Without this, the installer may "relocate" a bundle: update a copy it finds elsewhere (the build folder) instead of installing.
PLIST="build/pkg-components.plist"
pkgbuild --analyze --root "$STAGE" "$PLIST" >/dev/null
index=0
while /usr/libexec/PlistBuddy -c "Print :$index" "$PLIST" >/dev/null 2>&1; do
  for key in BundleIsRelocatable BundleIsVersionChecked; do   # never relocate; always replace, whatever version is installed
    /usr/libexec/PlistBuddy -c "Set :$index:$key false" "$PLIST" 2>/dev/null || /usr/libexec/PlistBuddy -c "Add :$index:$key bool false" "$PLIST"
  done
  index=$((index + 1))
done

pkgbuild --root "$STAGE" --component-plist "$PLIST" --identifier com.sfinstruments.orbit106.pkg --version "$VERSION" --install-location / "$PKG" >/dev/null
rm -rf "$STAGE" "$PLIST"
echo "Installeur prêt : $PKG"
