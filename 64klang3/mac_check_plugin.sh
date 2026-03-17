#!/bin/bash
# Verify 64klang3 VST3 installation and give hints if DAW doesn't show it.
# Does NOT touch any DAW or Ableton cache.

SYSTEM_VST3="/Library/Audio/Plug-Ins/VST3/64klang3.vst3"
USER_VST3="${HOME}/Library/Audio/Plug-Ins/VST3/64klang3.vst3"

echo "=============================================="
echo "  64klang3 VST3 check"
echo "=============================================="
echo ""

# Prefer system path for checks; report both
INSTALL_PATH="$SYSTEM_VST3"
if [[ ! -d "$INSTALL_PATH" ]]; then
  INSTALL_PATH="$USER_VST3"
fi
if [[ ! -d "$INSTALL_PATH" ]]; then
  echo "ERROR: Plugin not found in either location:"
  echo "      $SYSTEM_VST3"
  echo "      $USER_VST3"
  echo "      Run: ./install.sh"
  exit 1
fi
echo "[OK] System: $([ -d "$SYSTEM_VST3" ] && echo 'yes' || echo 'no')"
echo "[OK] User:   $([ -d "$USER_VST3" ] && echo 'yes' || echo 'no')"
BINARY="${INSTALL_PATH}/Contents/MacOS/64klang3"

# 2. Structure
if [[ ! -f "$BINARY" ]]; then
  echo "ERROR: Binary missing: $BINARY"
  exit 1
fi
echo "[OK] Binary exists: $BINARY"

# 3. Architecture
ARCH=$(file "$BINARY")
if [[ "$ARCH" == *"arm64"* ]]; then
  echo "[OK] Architecture: arm64 (Apple Silicon)"
elif [[ "$ARCH" == *"x86_64"* ]]; then
  echo "[OK] Architecture: x86_64 (Intel)"
else
  echo "[WARN] Architecture: $ARCH"
fi

# 4. Bundle bit (package icon)
if GetFileInfo "$INSTALL_PATH" 2>/dev/null | grep -q "attribute:.*B"; then
  echo "[OK] Bundle (package) flag set"
else
  echo "[!] Bundle flag missing (SetFile -a B). Folder may show as icon."
  echo "    Fix: sudo SetFile -a B $INSTALL_PATH"
fi

# 5. Permissions
LS=$(ls -la "$BINARY")
if [[ "$LS" == *"root"* ]]; then
  echo "[OK] Owner: root (normal after sudo copy)"
fi

# 6. Code signature (required for some hosts)
if codesign -v "$INSTALL_PATH" 2>/dev/null; then
  echo "[OK] Signature valid"
else
  echo "[!] Signature missing or invalid. Fix: codesign -f -s - $INSTALL_PATH"
fi

echo ""
echo "----------------------------------------------"
echo "  If the DAW still doesn't list it:"
echo "----------------------------------------------"
echo "  1. Ableton: Preferences → Plug-Ins"
echo "     - Is 'Use VST3 Plug-In System Folders' enabled?"
echo "     - Check 'Plug-In Errors' / 'Failed' – is 64klang3 there?"
echo ""
echo "  2. Rescan: use 'Rescan' in Plug-Ins or restart the DAW."
echo ""
echo "  3. VST3 vs VST: 64klang3 appears in the VST3 list,"
echo "     not next to legacy VST (64klang2)."
echo ""
echo "  4. Crash: If the plugin crashes on load, the DAW may hide it."
echo "     Open Console.app, start the DAW, rescan –"
echo "     look for '64klang3' or 'VST' crash messages."
echo ""
echo "  5. Validator (optional): If you build the full SDK, run the validator:"
echo "     cmake --build build --target validator"
echo "     ./build/_deps/vst3sdk-build/.../validator /Library/Audio/Plug-Ins/VST3/64klang3.vst3"
echo ""
