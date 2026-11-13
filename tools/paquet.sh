#!/bin/bash
# Build the installer inside the project. Does not install or modify Library.
# The current panel and Midnight Signals are embedded in the compiled binaries.
set -euo pipefail
cd "$(dirname "$0")/.."
PROJECT_DIR="$(pwd -P)"
case "$PROJECT_DIR" in /|"$HOME") echo "Refusing packaging outside a project."; exit 1;; esac
for directory in build .local-cache; do
  [ ! -L "$directory" ] || { echo "Refusing symlink: $directory"; exit 1; }
done
mkdir -p build .local-cache
export TMPDIR="$PROJECT_DIR/.local-cache"
VERSION=$(sed -n 's/^project(Orbit106 VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
[ -n "$VERSION" ] || { echo "Missing project version"; exit 1; }
OUT="build/Orbit106_artefacts/Release"
PKG="$PROJECT_DIR/build/Orbit106-$VERSION.pkg"
[ ! -L "$PKG" ] || { echo "Refusing output symlink"; exit 1; }
for item in "$OUT/AU/Orbit 106.component" "$OUT/VST3/Orbit 106.vst3" "$OUT/Standalone/Orbit 106.app"; do
  [ -d "$item" ] || { echo "Missing: $item; build first."; exit 1; }
done
WORK=$(mktemp -d "$PROJECT_DIR/build/pkg-stage.XXXXXX")
trap 'rm -rf "$WORK"' EXIT
STAGE="$WORK/root"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/Components" "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Applications"
ditto "$OUT/AU/Orbit 106.component" "$STAGE/Library/Audio/Plug-Ins/Components/Orbit 106.component"
ditto "$OUT/VST3/Orbit 106.vst3" "$STAGE/Library/Audio/Plug-Ins/VST3/Orbit 106.vst3"
ditto "$OUT/Standalone/Orbit 106.app" "$STAGE/Applications/Orbit 106.app"
SOUNDS="$STAGE/Library/Application Support/SF Instruments/Orbit 106/Presets/Sounds"
mkdir -p "$SOUNDS"
count=0
for sound in Presets/Sounds/*.orbitpreset Presets/MidnightSignals/*.orbitpreset; do
  [ -f "$sound" ] || continue
  cp "$sound" "$SOUNDS/"
  count=$((count + 1))
done
cp Presets/MidnightSignals/README.md "$STAGE/Library/Application Support/SF Instruments/Orbit 106/Midnight Signals.md"
echo "Sons inclus : $count (dont 16 Midnight Signals, également intégrés aux binaires)"
find "$STAGE" -name ".DS_Store" -delete
xattr -cr "$STAGE"
chmod -R u+w,go+rX "$STAGE"
# Re-sign staged bundles after the VST3 manifest/resource generation has finished.
for bundle in "$STAGE/Applications/Orbit 106.app" "$STAGE/Library/Audio/Plug-Ins/Components/Orbit 106.component" "$STAGE/Library/Audio/Plug-Ins/VST3/Orbit 106.vst3"; do
  codesign --force --deep --sign - "$bundle"
  codesign --verify --deep --strict "$bundle"
done
python3 - "$STAGE" "$PKG.payload.json" <<'PYMANIFEST'
import hashlib, json, sys
from pathlib import Path
root = Path(sys.argv[1])
manifest = {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}
Path(sys.argv[2]).write_text(json.dumps(manifest, indent=2))
PYMANIFEST
# Keep the installation at its intended destination, rather than relocating to a build copy.
PLIST="$WORK/components.plist"
pkgbuild --analyze --root "$STAGE" "$PLIST" >/dev/null
index=0
while /usr/libexec/PlistBuddy -c "Print :$index" "$PLIST" >/dev/null 2>&1; do
  for key in BundleIsRelocatable BundleIsVersionChecked; do
    /usr/libexec/PlistBuddy -c "Set :$index:$key false" "$PLIST" 2>/dev/null || /usr/libexec/PlistBuddy -c "Add :$index:$key bool false" "$PLIST"
  done
  index=$((index + 1))
done
pkgbuild --root "$STAGE" --component-plist "$PLIST" --identifier com.sfinstruments.orbit106.pkg --version "$VERSION" --install-location / "$WORK/Orbit106.pkg"
mv -f "$WORK/Orbit106.pkg" "$PKG"
echo "Installeur prêt : $PKG"
