# 64klang3 Modernization Plan

Migrate 64klang from a 4-DLL Windows-only VST2 plugin to a single cross-platform VST3 plugin with ImGui GUI, built with CMake.

---

## Target Architecture

One `.vst3` bundle. No separate DLLs. Core compiled as static library, linked into the VST3 plugin.

```
64klang3/
  CMakeLists.txt                  # root build, FetchContent for all deps
  cmake/
    Dependencies.cmake            # FetchContent declarations (pinned tags/commits)
  src/
    core/                         # was: 64klang2Core (portable C++)
      platform.h                  # NEW: portability macros
      sample_t.h/cpp              # + uintptr_t p[2] union member
      SynthNode.h/cpp             # portable, no #ifdef _M_X64
      SynthController.h/cpp       # std::mutex, no Win32
      SynthAllocator.h/cpp        # _mm_malloc, no GlobalAlloc
      Synth.h/cpp                 # stdint types, no windows.h
      tinyxml*                    # unchanged
    vst3/
      Factory.cpp                 # GetPluginFactory entry point
      Plugin.cpp                  # IComponent + IAudioProcessor
      Controller.cpp              # IEditController
      PluginView.cpp              # IPlugView wrapping ImGui
    gui/
      ImGuiPlugin.cpp/h           # main render loop
      NodeCanvas.cpp/h            # node graph (zoom, pan, wires, nodes)
      Widgets.cpp/h               # Knob, VUMeter, BitPattern, Arp, etc.
      NodeConfig.cpp/h            # loads 64klang2Config.xml
    platform/
      windows/                    # SAPI, GM.DLS, ACM implementations
      stub/                       # cross-platform stubs/alternatives
  tools/
    DebugExe/                     # standalone host, no C++/CLI
```

No `external/` directory — all third-party dependencies are fetched at configure time via CMake `FetchContent`, pinned to specific commits/tags in `cmake/Dependencies.cmake`:

```cmake
# cmake/Dependencies.cmake
include(FetchContent)

FetchContent_Declare(vst3sdk
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
  GIT_TAG        v3.7.11_build_10   # pin to release tag
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.6            # pin to release tag
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(vst3sdk imgui)
```

The `Player/` project stays separate and Windows-only (demoscene use case). The original `VSTiPluginSourceCode/` stays untouched during development.

---

## Key Design Decisions

### 1. `__fastcall` → Portable macro

```c
// platform.h
#if defined(_MSC_VER) && defined(_M_IX86)
  #define SYNTHCALL __fastcall
#elif defined(__GNUC__) && defined(__i386__)
  #define SYNTHCALL __attribute__((fastcall))
#else
  #define SYNTHCALL  // no-op on x64 and ARM
#endif
```

Replace all ~70 `__fastcall` declarations with `SYNTHCALL`. The `SynthFunction` typedef becomes `void(SYNTHCALL *)(SynthNode*)`.

### 2. Pointer-in-sample packing (eliminates ~28 `#ifdef _M_X64` blocks)

Add `uintptr_t p[2]` to the `sample_t` union. All 28 sites in SynthNode.cpp that pack pointers into `v[n].i[0]` (32-bit) or `v[n].l[0]` (64-bit) become `v[n].p[0]` — no ifdef needed.

### 3. VST3 IPlugView + ImGui

`IPlugView::attached(parent)` receives a platform window handle. The plugin creates a child window inside it, initializes a graphics context (D3D11 on Windows, OpenGL on Mac/Linux), and runs ImGui targeting that child. A timer drives the render loop. The host's message pump handles events.

### 4. State streaming

VST2 used temp files for `getChunk`/`setChunk`. VST3 uses `IBStream`. Add `SynthController::savePatchToString()` / `loadPatchFromString()` using TinyXML's in-memory parse/print. No temp files.

### 5. `COMPILE_VSTI` → always defined in VST3 build

The Player project keeps its own copy of core files and does NOT define it (as today). No change to Player.

### 6. Win32 mutex → `std::timed_mutex`

The 1ms-timeout audio thread pattern (`WaitForSingleObject(mutex, 1)`) becomes `mutex.try_lock_for(1ms)`. All ~20 lock/unlock sites are mechanical replacements.

### 7. GUI↔Core: direct calls (no wrapper)

The C++/CLI Wrapper's ~50 delegate callbacks become direct `SynthController` method calls from ImGui code. The `RebuildGUI()` function iterates `SynthController::numGUINodes()` and builds the ImGui canvas node list. `UpdateGUIPositions()` writes canvas positions back to core before save.

---

## Phases

### Phase 0: Repository Structure + CMake Skeleton ✅ COMPLETE

- Create `64klang3/` directory structure
- Write `cmake/Dependencies.cmake` with `FetchContent` declarations for VST3 SDK and Dear ImGui, each pinned to a specific release tag
- Write root `CMakeLists.txt`:
  - `include(cmake/Dependencies.cmake)` to fetch all third-party deps at configure time
  - `64klang_core` static library (no sources yet)
  - `64klang3` VST3 plugin target (via `smtg_add_vst3plugin`)
  - `DebugExe` executable (Windows-only)
  - `imgui` static library (built from fetched sources)
- Copy core sources into `src/core/` (originals stay untouched)
- **Verify**: `cmake --configure` succeeds (deps download and configure)

<details>
<summary>Completion Notes</summary>

**Files created:**
- `64klang3/CMakeLists.txt` — root build file with imgui static lib (D3D11/Win32 backends on Windows), 64klang_core static lib, 64klang3 VST3 plugin via `smtg_add_vst3plugin`, DebugExe (Windows-only)
- `64klang3/cmake/Dependencies.cmake` — FetchContent for vst3sdk (v3.7.11_build_10) and imgui (v1.91.6)
- All core source files copied from `VSTiPluginSourceCode/64klang2Core/` into `64klang3/src/core/`
- VST3 stub files created in `64klang3/src/vst3/`
- GUI stub files created in `64klang3/src/gui/`
- Platform abstraction files created in `64klang3/src/platform/`

**Decisions:**
- VST3 SDK example plugins disabled (`SMTG_ADD_VST3_PLUGINS_SAMPLES OFF`)
- VSTGUI disabled (`SMTG_ADD_VSTGUI OFF`) — using ImGui instead
- STK library removed from build (not used by core engine — BOWED node is custom implementation)
- `cmake --configure` verified: deps download and configure successfully on MSVC x86 (Win32)
</details>

### Phase 1: Core Portability ✅ COMPLETE

Make `src/core/` compile without Windows headers in the hot path. One file at a time.

**1.1 Create `platform.h`** ✅ — All portability macros: `SYNTHCALL`, `K64_ALIGN16`, Win32 type fallbacks (`DWORD`→`uint32_t` etc.), `K64_API` visibility, `MAKEFOURCC`, `K64_CODE_SECTION`.

**1.2 `sample_t.h/cpp`** ✅
- `#include <intrin.h>` → conditional `<immintrin.h>` for GCC/Clang
- `_MM_ALIGN16` → `K64_ALIGN16`
- `__int64 l[2]` → `int64_t l[2]`
- Add `uintptr_t p[2]` to union
- All `__fastcall` → `SYNTHCALL`

**1.3 `SynthAllocator.h/cpp`** ✅
- `GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, size)` → `_mm_malloc(size, 16)` + `memset`
- `GlobalFree` → `_mm_free`
- Remove `#include "windows.h"`

**1.4 `SynthNode.h`** ✅
- Remove `#include "windows.h"`, include `platform.h`
- `SynthFunction` typedef: `__fastcall` → `SYNTHCALL`
- `VMVoice._padding1_` guard: `_M_X64` → `sizeof(void*) > 4`
- All Win32 types → stdint types

**1.5 `SynthNode.cpp`** ✅ (largest file, most changes)
- ~60 `#pragma code_seg` → `K64_CODE_SECTION` macro
- ~28 `#ifdef _M_X64` pointer packing → `n->v[x].p[0] = (uintptr_t)ptr`
- SAPI_tick: wrap in `#ifdef _WIN32` (entire function, both x86 and x64 paths)
- GMDLS_tick: wrap Windows-specific loading in `#ifdef _WIN32`
- `#pragma warning(disable:4244)` → compiler flag in CMake

**1.6 `SynthController.h/cpp`** ✅
- `HANDLE DataAccessMutex` → `std::timed_mutex DataAccessMutex`
- `HINSTANCE ModuleInstance` → `void* ModuleInstance`
- `CreateMutex` / `WaitForSingleObject` / `ReleaseMutex` → `std::timed_mutex` methods
- `MessageBoxA` (~10 sites) → `fprintf(stderr, ...)` + optional error callback
- `__cpuid` → portable wrapper (MSVC `__cpuid` vs GCC `__get_cpuid`)
- `MY64KLANG2CORE_API` → `K64_API`
- Add `savePatchToString()` / `loadPatchFromString()` (TinyXML string I/O)

**1.7 `Synth.h/cpp`** ✅
- Public API: `DWORD` → `uint32_t`, `BYTE` → `uint8_t`, `LPBYTE` → `uint8_t*`
- `GetSystemDirectoryA` + `CreateFileA`/`ReadFile` (GM.DLS) → platform abstraction
- ACM GSM codec calls → platform abstraction

**1.8 Platform abstractions** ✅ (3 interfaces, Win32 + stub implementations)
- `ICodec`: `convertGSMtoPCM` / `convertPCMtoGSM` — Windows ACM vs libgsm
- `IGMDLSLoader`: `loadGMDLS(...)` — Windows `gm.dls` vs bundled SF2 or null stub
- `ITTSEngine`: `synthesize(text, ...)` — Windows SAPI vs eSpeak-NG or silence

**Verify**: `cmake --build . --target 64klang_core` compiles on MSVC x86 (Win32)

<details>
<summary>Completion Notes</summary>

**Key issues encountered and resolved:**

- `sizeof(void*)` cannot be used in `#if` preprocessor directives — changed VMVoice padding guard to `#if !defined(_M_X64) && !defined(__x86_64__) && !defined(__aarch64__)`
- SynthNode.cpp pointer packing: automated with Python regex to replace ~28 `#ifdef _M_X64` blocks with `uintptr_t p[0]`. Required two passes — first pass missed multi-line blocks with two assignments and read-back patterns.
- `platform.h` initially used `#ifndef _WINDOWS_` guard for Win32 type fallbacks, but this caused conflicts because `platform.h` was included before `windows.h` via `sapi.h`. Fixed by including `<windows.h>` directly in `platform.h` on Windows.
- `WIN32_LEAN_AND_MEAN` excluded multimedia headers (`mmsystem.h`, `mmreg.h`) needed by ACM/SAPI code — removed it.
- `MAKEFOURCC` defined by both `platform.h` and `mmsyscom.h` — made our definition non-Windows only.
- `DWORD` is `unsigned long` on Windows vs `uint32_t` is `unsigned int` — same size but different types, causing reference conversion failures. Fixed by using matching types at call sites.
- `tinyxml.cpp` missing `<cfloat>` for `DBL_DIG` — added include.
- `TIXML_USE_STL` needed for `std::string` TinyXML constructors — added as compile definition.
- SynthController.cpp: `_64klang_Render` → `_64klang_Tick` (porting agent used wrong function name).
- SAPI section: wrapped entire block (declarations, CLSIDs, SAPI_tick function) in `#ifdef _WIN32` with a no-op stub for non-Windows. Added `#include <mmreg.h>` before WAVEFORMATEX usage.
</details>

### Phase 2: Full CMake Build ✅ COMPLETE

- Wire all core sources into `64klang_core` target
- Platform-specific sources: conditional `target_sources` for Windows vs stubs
- Compiler flags: `/arch:SSE4.1` (MSVC), `-msse4.1` (GCC/Clang)
- Link libraries: `msacm32`, `wmvcore` on Windows only
- ~~STK library integration (for Bowed node): add as static library target~~ (not needed — BOWED node is custom implementation, does not use STK)
- **Verify**: static library links cleanly

<details>
<summary>Completion Notes</summary>

**Build output:** `build\Release\64klang_core.lib` — compiles with 0 errors, 0 warnings on MSVC x86 (Win32).

**10 source files compile cleanly:**
`sample_t.cpp`, `SynthNode.cpp`, `SynthAllocator.cpp`, `Synth.cpp`, `SynthController.cpp`, `tinyxml.cpp`, `tinystr.cpp`, `tinyxmlerror.cpp`, `tinyxmlparser.cpp`, `WindowsPlatform.cpp`

**CMake configuration:**
- Platform sources: `WindowsPlatform.cpp` on Win32, `StubPlatform.cpp` otherwise
- Compile defs: `COMPILE_VSTI`, `TIXML_USE_STL`
- Win32 link libs: `msacm32`, `wmvcore`, `ole32`
- MSVC warnings suppressed: `/wd4244` (conversion), `/wd4267` (size_t), `/wd4018` (signed/unsigned)
- GCC/Clang: `-msse4.1`, `-Wno-conversion`, `-Wno-sign-conversion`
</details>

### Phase 3: VST3 Plugin Shell (audio only, no GUI) ✅ COMPLETE

**`Factory.cpp`**: `BEGIN_FACTORY_DEF("Alcatraz")`, register processor + controller classes.

**`Plugin.cpp`** (derives from `Vst::AudioEffect`):
- `initialize()`: create `SynthController` singleton
- `setBusArrangements()`: 0 inputs, 1 stereo output
- `process(ProcessData&)`:
  - Try-lock mutex (1ms timeout, output silence on fail)
  - Iterate `data.inputEvents` → `ApplyMidiEvent()` (port of `VstXSynth::ApplyEvent`)
  - Call `SynthController::tick(left, right, numSamples)`
  - Read BPM from `data.processContext->tempo`
- `getState()`/`setState()`: XML via `IBStream` using new string methods

**`Controller.cpp`**: Minimal `EditController`, 0 parameters, returns `IPlugView`.

**Verify**: Plugin loads in REAPER/Bitwig, accepts MIDI, produces audio, state save/load works.

<details>
<summary>Completion Notes</summary>

**Build output:** `build\VST3\Release\64klang3.vst3\Contents\x86-win\64klang3.vst3` (517 KB)

**Files implemented:**
- `Factory.cpp` — VST3 factory with processor UID `64ABCDEF-12340001-AAAABBBB-CCCCDDD1` and controller UID `...-CCCCDDD2`, vendor "Alcatraz", category `kInstrumentSynth`
- `Plugin.h/cpp` — `K64Plugin` (AudioEffect): 0 inputs, 1 stereo output, MIDI event input, process() with `try_lock_for(1ms)`, getState/setState via `savePatchToString`/`loadPatchFromString` + `IBStream`
- `Controller.h/cpp` — `K64Controller` (EditController): 0 parameters, creates `K64PluginView`
- `PluginView.h/cpp` — `K64PluginView` (CPluginView): stub with 1280x800 default size, platform type support for HWND/NSView/X11

**Fixes applied during build:**
- Added `#include "pluginterfaces/base/ibstream.h"` to Plugin.cpp (IBStream was only forward-declared)
- Added `#include "pluginterfaces/base/fstrdefs.h"` to PluginView.cpp and Controller.cpp (for `FIDStringsEqual`)

**VST3 Validator results:** Module loads successfully, finds both classes (processor + controller), validator test suite runs. Post-build symlink to `Program Files` fails without admin (benign).

**Pending functional verification:** Loading in DAW, MIDI→audio, state persistence (requires synth initialization with a valid patch).
</details>

### Phase 4: ImGui GUI — Display Only

**`PluginView.cpp`**: `IPlugView` implementation
- `attached()`: create child HWND, init D3D11 + ImGui, start render timer
- `removed()`: destroy ImGui context and child window
- `onSize()`: resize swap chain

**`NodeConfig.cpp`**: Parse `64klang2Config.xml` into in-memory structs (node type defs, input metadata, mode enumerations, menu categories).

**`NodeCanvas.cpp`**: Read-only node graph display
- Build node list from `SynthController::numGUINodes()` / `gnXxx()` accessors
- Render nodes as colored rects (pink=global, blue=voice, gold=selected)
- Render 4-segment polyline wires (LightSkyBlue/DeepPink/gold)
- Pan (right-drag) and zoom (mouse wheel)

**Verify**: Editor opens, displays the loaded patch's node graph correctly.

### Phase 5: ImGui GUI — Full Interaction

**Node graph interactions**:
- Left-click select (Ctrl=multi, Shift=recursive-input-select)
- Left-drag to move nodes
- Rubber-band selection rectangle
- Wire drag from output pin to input pin → `connectInput()`
- Click connected input pin → `disconnectInput()`
- Right-click context menu → node creation (categories from XML)
- Delete key → `deleteNode()`
- Copy/paste selection

**Knob widget** (`Widgets.cpp`):
- Dual L/R stereo knobs with sync checkbox
- Vertical drag: normal sensitivity + Ctrl for 1/128 precision
- 14 display mappings (Hz, ms, dB, ratio, etc.) driven by XML metadata
- Red modulator marker line showing live signal value

**Mode controls**:
- RadioButtons for ≤4 items, Combo for >4 items, Checkboxes for flags

**Special editors**:
- ArpeggiatorEdit: 32-step piano roll (draw list rectangles, click-drag)
- BitPattern: 8+8 toggle buttons per TriggerSequencer pattern
- Formula editor: `InputTextMultiline` + parse/plot
- SAPI text editor: `InputTextMultiline` + update button
- WaveFileDialog: 32-slot table with load/clear/sample-rate

**Per-frame live updates** (mutex try-lock, skip frame if busy):
- VU meters on SynthRoot/ChannelRoot
- VoiceManager active voice count
- Knob modulator markers
- TriggerSequencer playback position
- Arpeggiator play cursor

**Node edit panels**: Float at node position (on-canvas) or dock in left panel (Ctrl+click).

**Verify**: Full parity with WPF GUI — all node/knob/mode editing, live signal display.

### Phase 6: Toolbar + Patch I/O

- Load/Save/Reset/Export patch buttons with native file dialogs
- Edit Wavetables dialog
- Export Song (recording toggle + quantization combo)
- Jump To channel combo → scroll to channel
- Search textbox → highlight matching nodes
- Panic button
- Always On Top toggle
- Voice count label

**Verify**: Complete workflow — load patch, edit, save, export song.

### Phase 7: DebugExe Modernization

- Remove C++/CLI dependency — call `SynthController` directly
- Keep `AudioOut.h` (WASAPI) and `MidiIn.h` (WinMM)
- Create standalone Win32 window with D3D11 + ImGui (reuse GUI code)
- No .NET runtime required

**Verify**: DebugExe runs, loads patches, accepts MIDI, shows ImGui GUI.

### Phase 8: 64-bit + Cross-Platform

**x64 Windows**: Should mostly work after Phase 1. Verify no remaining `_M_X64` issues.

**macOS**: `PluginView.cpp` gets `#ifdef __APPLE__` path — `NSView` child + Metal/OpenGL ImGui backend. Platform stubs for SAPI/GMDLS/ACM.

**Linux**: `PluginView.cpp` gets X11 child window + OpenGL backend. File dialogs via `zenity`/`kdialog` subprocess.

**Verify**: x64 Windows builds and loads. Mac/Linux compile (functional validation secondary).

### Phase 9: Cleanup + Validation

- Remove dead code paths
- Run VST3 Plugin Validator
- Update `CLAUDE.md` for new project
- CMake install rules for VST3 bundle

---

## Platform-Specific Subsystems (complete inventory)

| Subsystem | Windows | Mac/Linux |
|-----------|---------|-----------|
| SAPI TTS | COM `ISpVoice` (+ x86 inline asm path) | eSpeak-NG subprocess or silence stub |
| GM.DLS | `%SystemRoot%\System32\drivers\gm.dls` via `CreateFileA` | Bundled SF2 or null stub |
| ACM GSM6.10 | `msacm32.lib` `acmStream*` | libgsm |
| Mutex | ~~`CreateMutex`/`WaitForSingleObject`~~ → `std::timed_mutex` | Same |
| Allocator | ~~`GlobalAlloc`~~ → `_mm_malloc` | Same |
| Error dialogs | ~~`MessageBoxA`~~ → `fprintf(stderr, ...)` | Same |
| File I/O | ~~`CreateFileA`/`ReadFile`~~ → `std::ifstream` | Same |
| CPU detection | `__cpuid` (MSVC) | `__get_cpuid` (GCC/Clang) |
| SSE control | `_mm_getcsr`/`_mm_setcsr` — wrap in `#ifdef __SSE__` | Same on x86 |

## Critical Source Files

| File | Role in migration |
|------|-------------------|
| `VSTiPluginSourceCode/64klang2Core/SynthNode.cpp` | Largest file: ~28 pointer-packing sites, ~60 code_seg pragmas, SAPI/GMDLS platform code |
| `VSTiPluginSourceCode/64klang2Core/SynthController.cpp` | All Win32 mutex/MessageBox calls, ACM codec, CPU detection; defines the full API ImGui will call |
| `VSTiPluginSourceCode/64klang2Core/sample_t.h` | Foundation type; needs `uintptr_t p[2]`, portable headers, `K64_ALIGN16` |
| `VSTiPluginSourceCode/64klang2Core/Synth.cpp` | Public API with Win32 types, GM.DLS file loading, ACM codec calls |
| `VSTiPluginSourceCode/64klang2Wrapper/64klang2Wrapper.cpp` | Reference for all ~50 GUI↔Core callbacks to reimplement as direct calls |
| `VSTiPluginSourceCode/64klang2GUI/64klang2Config.xml` | Complete node type metadata driving the ImGui GUI |
| `VSTiPluginSourceCode/64klang2VSTi/vstxsynthproc.cpp` | Audio processing + MIDI dispatch logic to port to VST3 `process()` |

## Verification

- **Phase 1-2**: `cmake --build . --target 64klang_core` compiles on MSVC x86
- **Phase 3**: Plugin loads in DAW, processes MIDI→audio, state persists
- **Phase 4**: Editor window opens, displays node graph
- **Phase 5-6**: Full interactive editing at parity with WPF GUI
- **Phase 7**: DebugExe runs without .NET
- **Phase 8**: x64 build works; Mac/Linux compile
- **Phase 9**: VST3 Plugin Validator passes
