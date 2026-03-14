#!/bin/bash
# Rebuild 64klang3 VST3 and install to /Library/Audio/Plug-Ins/VST3/
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
VST3_OUT="${BUILD_DIR}/VST3/Release/64klang3.vst3"
INSTALL_DIR="/Library/Audio/Plug-Ins/VST3"

echo "==> Building 64klang3..."
cd "$SCRIPT_DIR"

if [[ ! -d "$BUILD_DIR" ]]; then
  echo "==> Configuring (first run)..."
  mkdir -p "$BUILD_DIR"
  cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi

cmake --build "$BUILD_DIR" --target 64klang3 --config Release

if [[ ! -d "$VST3_OUT" ]]; then
  echo "Error: Build did not produce $VST3_OUT"
  exit 1
fi

echo "==> Installing to $INSTALL_DIR (sudo)..."
sudo cp -R "$VST3_OUT" "$INSTALL_DIR/"
# Clear extended attributes so codesign does not fail with "detritus not allowed"
sudo xattr -cr "${INSTALL_DIR}/64klang3.vst3"
sudo codesign -f -s - "${INSTALL_DIR}/64klang3.vst3"
sudo SetFile -a B "${INSTALL_DIR}/64klang3.vst3" 2>/dev/null || true

# Also install to user VST3 folder (some DAWs only scan this)
USER_VST3="${HOME}/Library/Audio/Plug-Ins/VST3"
mkdir -p "$USER_VST3"
if [[ -d "$USER_VST3" ]]; then
  echo "==> Installing to $USER_VST3 (user folder)..."
  cp -R "$VST3_OUT" "$USER_VST3/"
  xattr -cr "${USER_VST3}/64klang3.vst3"
  codesign -f -s - "${USER_VST3}/64klang3.vst3"
  SetFile -a B "${USER_VST3}/64klang3.vst3" 2>/dev/null || true
fi

echo "==> Done. 64klang3 is in both system and user VST3 folders. Rescan plugins in your DAW if needed."
