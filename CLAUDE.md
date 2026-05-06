# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

64klang is a modular, node-graph-based software synthesizer for the demoscene, targeting 64k intros and 32k executable music. It produces a VST3 plugin (C++ engine + ImGui node-graph GUI) and a standalone player for embedding synthesized music into size-limited executables.

Key constraints: 44100 Hz sample rate only, SSE4.1 CPU required (x86/x64; ARM uses NEON via platform.h), Windows/macOS/Linux supported, singleton synth engine managing all 16 MIDI channels.

---

## Active Codebase: 64klang3 (VST3 + CMake)

**All new development happens here.** The legacy 64klang2 code has been moved to `64klang2/` and is no longer under active development.

### Location

```
64klang3/
```

### Build System

CMake 3.20+. Third-party dependencies (VST3 SDK v3.8.0, ImGui v1.91.6) are fetched automatically via `FetchContent`.

```
# Configure (once, run from repo root)
cmake -B build64 -A x64 ./64klang3
cmake -B build32 -A Win32 ./64klang3   # 32-bit

# Build the VST3 plugin
cmake --build build64 --target 64klang3 --config Release
```

Pre-generated project files live in `build32/` and `build64/` at the repo root and can be opened directly in Visual Studio.

### Targets

| Target | Type | Platform |
|--------|------|----------|
| `64klang_core` | Static lib | all |
| `64klang3` | VST3 plugin | all |
| `DebugExe` | Win32 EXE test host | Windows only |
| `Player` (subdirectory) | Size-optimised EXE, compiled-in song data | Windows only |
| `64klang_blob` (subdirectory) | Runtime blob-loading static lib | all |
| `BlobHostExample` (subdirectory) | Minimal waveOut blob host | Windows only |

### Source Layout

```
64klang3/src/
  core/           — synth engine (shared by all targets)
  gui/            — ImGui node-graph editor
  vst3/           — VST3 plugin wrapper
  platform/       — platform-specific implementations
64klang3/tools/DebugExe/  — standalone WASAPI + MIDI test host
64klang3/player/          — Player target (compiled-in song data, no GUI)
64klang3/blobplayer/      — 64klang_blob static lib + BlobHostExample (runtime blob loading)
64klang3/cmake/           — CMake helpers (Dependencies.cmake)
```

### Architecture

#### Signal Flow

The engine evaluates the node graph **per sample** (not per block). Every signal is stereo, represented as `sample_t` — a wrapper around `__m128d` (two packed doubles for L/R). This enables sample-exact feedback loops and physical modelling.

#### Core Engine (`src/core/`)

- **`sample_t.h/cpp`** — SSE4.1 stereo sample type with vectorised math (sin, exp2, log2, rand, arithmetic, comparisons)
- **`SynthNode.h/cpp`** — Fundamental processing unit. `SynthNode.h` defines the `NodeIDs` enum (60+ types) and per-node input enumerations. `SynthNode.cpp` has all tick functions (per-sample) and init functions. Tick functions are `__fastcall` via `SynthFunction` typedef.
- **`Synth.h/cpp`** — Public API: `_64klang_Init()`, `_64klang_Render()`, `_64klang_NoteOn()`, `_64klang_NoteOff()`, `_64klang_Tick()`
- **`SynthController.h/cpp`** — Singleton managing the node graph, patch load/save (XML via TinyXML), MIDI dispatch, `panic()`, `tick()`. Exposes `static std::timed_mutex DataAccessMutex` guarding all graph mutations.
- **`SynthAllocator.h/cpp`** — Custom allocator for SynthNodes (no STL in hot path)

#### Node Graph Hierarchy

```
Synth (root) → Channel Root (×16) → Voice Manager → Voice Root → [processing nodes]
```

Global (pink) nodes exist once; voice (blue) nodes are instanced per note by VoiceManager.

#### GUI (`src/gui/`)

ImGui-based node-graph editor, platform-independent.

- **`ImGuiPlugin.h/cpp`** — Top-level GUI API (`createCanvas`, `destroyCanvas`, `init`, `shutdown`, `render`, viewport save/restore, native file dialogs)
- **`NodeCanvas.h/cpp`** — Zoomable/pannable node-graph canvas
- **`NodeConfig.h/cpp`** + **`NodeConfigData.h`** — XML-driven node type registry (input counts, ranges, modes, context menus)
- **`Widgets.h/cpp`** — Shared ImGui widgets including the spectrum/signal visualiser panel
- **`ArpEditor.h/cpp`** — Arpeggiator step-editor popup

#### VST3 Wrapper (`src/vst3/`)

- **`Plugin.h/cpp`** (`K64Plugin : AudioEffect`) — Audio processor. Singleton `s_renderOwner` ensures only one instance calls `sc->tick()`; non-owners forward MIDI and output silence. `DataAccessMutex` is acquired only by the render owner (zero-timeout) to avoid cross-instance contention. `panic()` is called on `terminate()` to kill stuck notes.
- **`Controller.h/cpp`** (`K64Controller`) — VST3 edit controller; hosts MIDI CC parameter mapping.
- **`PluginView.h/cpp`** (`K64PluginView`) — Editor window. Singleton GL/ImGui context shared across alias instances; `s_allViews` list enables automatic renderer failover when the active editor is closed while another is open.
- **`Factory.cpp`** — VST3 factory registration (plugin + controller UIDs)

#### Key Preprocessor Defines

- `COMPILE_VSTI` — defined for all VST3/DebugExe builds; enables `_64klang_NoteOn/Off/Tick` etc. Not defined for the Player or blob library.
- `USE_BLOBS` — defined for the `64klang_blob` target; enables the two-argument `_64klang_Init(songStream, patchData)` that reads offsets from the front of the patch blob. Full-feature build (no `*_SKIP` defines). Mutually exclusive with `COMPILE_VSTI`.
- `TIXML_USE_STL` — TinyXML uses `std::string`
- `STOREDSAMPLES_SKIP` — omits GSM sample-decoder tables from the Player/blob build when no Sampler nodes are used

#### Platform-Specific Features

Three node types depend on Windows APIs and produce silence via `StubPlatform.cpp` on macOS/Linux:

| Node | Windows API | Stub behaviour |
|------|-------------|----------------|
| **Sampler** | Windows ACM (`msacm32`) — GSM 6.10 codec | Returns silence (no decoded samples) |
| **GM.DLS Sampler** | Reads `GM.DLS` from `%SystemRoot%\System32` | Returns silence (no wave data) |
| **TTS** | Windows SAPI speech synthesis | Returns silence |

#### Multi-instance Behaviour

Multiple `K64Plugin` instances (e.g. Renoise alias instruments) share the singleton `SynthController`. The first to call `initialize()` becomes `s_renderOwner` and is the only instance that advances the engine. Others forward MIDI and output silence. `DataAccessMutex` is held only by the render owner during `sc->tick()`, so GUI operations (patch edit, node drag) and non-owner `process()` calls never contend.

#### Blob / Runtime Loading (`64klang3/blobplayer/`)

The `64klang_blob` static library compiles the engine with `USE_BLOBS` defined and no `SynthController`/ImGui/TinyXML dependency. It exposes a plain-C API for loading patch + song `.blob` files at runtime:

- **`64klang_blob_api.h`** — public C API: `k64_blob_init`, `k64_blob_init_from_files`, `k64_blob_render`, `k64_blob_song_length`, `k64_blob_bpm`
- **`64klang_blob_api.cpp`** — implementation; copies blobs internally, calls `_64klang_Init(songStream, patchData)`, caches BPM and song length from the blob header
- **`example_host.cpp`** (`BlobHostExample`) — minimal Win32 waveOut host; loads blobs by filename, renders on a background thread, plays back via waveOut

Every plugin **Export Patch** / **Export Song** operation writes `.h` and `.blob` sidecar files. The `.blob` format always uses full-feature encoding (no skip optimisations), making it suitable for any host codebase.

---

## Legacy Codebase (to be removed)

The original 64klang2 implementation has been moved to `64klang2/`. It will be deleted once 64klang3 is production-ready. Do not start new work here.

- **`64klang2/VSTiPluginSourceCode/`** — VS2022/v142 solution; VST2 plugin (C++ engine + C#/WPF GUI + C++/CLI wrapper)
- **`64klang2/Player/`** — VS2015 solution; standalone Win32 playback EXE for 64k intros
- **`64klang2/VSTiPlugin/`** — precompiled plugin zip and example instruments/songs

### Legacy Build (reference only)

```
msbuild 64klang2/VSTiPluginSourceCode/64klang2.sln /p:Configuration=Release /p:Platform=Win32
msbuild 64klang2/Player/Player.sln /p:Configuration=Release /p:Platform=Win32
```
