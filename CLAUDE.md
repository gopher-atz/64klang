# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

64klang is a modular, node-graph-based software synthesizer for the demoscene, targeting 64k intros and 32k executable music. It consists of a 32-bit VSTi plugin (C++ engine + .NET WPF GUI) and a standalone player for embedding synthesized music into size-limited executables.

Key constraints: 44100 Hz sample rate only, SSE4.1 CPU required, Windows-only, singleton VSTi instance managing all 16 MIDI channels.

## Build System

Everything is built with Visual Studio / MSBuild. No CMake, Makefiles, or CI.

### VSTi Plugin (full synth + GUI)

Solution: `VSTiPluginSourceCode/64klang2.sln` (VS2022 format, v142 toolset)

```
msbuild VSTiPluginSourceCode/64klang2.sln /p:Configuration=Release /p:Platform=Win32
```

Five projects with explicit dependency ordering:
1. **64klang2Core** (C++ DLL) — synth engine
2. **64klang2GUI** (C# DLL, .NET 4.8) — WPF node-graph editor
3. **64klang2Wrapper** (C++/CLI DLL) — bridge between Core and GUI
4. **64klang2VSTi** (C++ DLL) — VST2 plugin entry point (depends on Core + Wrapper)
5. **DebugExe** (C++ EXE) — standalone test host with WASAPI audio + MIDI input (depends on Wrapper)

Configurations: `Debug`, `Release`, `Final` (demoscene-optimized). All post-build events copy DLLs to `C:\VST`.

### Player (standalone playback for intros)

Solution: `Player/Player.sln` (VS2015+)

```
msbuild Player/Player.sln /p:Configuration=Release /p:Platform=Win32
```

Configurations: `Debug|Win32`, `Debug|x64`, `Release|Win32`, `Release|x64`.

### No Tests

There is no test framework or automated test suite. The DebugExe project serves as the interactive test harness (loads synth + GUI, accepts live MIDI, outputs via WASAPI).

## Architecture

### Signal Flow

The entire synth engine evaluates a node graph **per sample** (not per block). Every signal is stereo, represented as `sample_t` — a wrapper around `__m128d` (two packed doubles for left/right channels). This enables sample-exact feedback loops, physical modelling, and cross-channel connections.

### Core Engine (`VSTiPluginSourceCode/64klang2Core/`)

- **`sample_t.h/cpp`** — SSE4.1 stereo sample type with vectorized math (sin, exp2, log2, rand, arithmetic, comparisons)
- **`SynthNode.h/cpp`** — The fundamental processing unit. `SynthNode.h` defines the `NodeIDs` enum (60+ node types) and per-node input enumerations. `SynthNode.cpp` contains all tick functions (per-sample processing) and init functions for every node type. Tick functions use `__fastcall` calling convention via `SynthFunction` typedef.
- **`Synth.h/cpp`** — Public API: `_64klang_Init()`, `_64klang_Render()`, `_64klang_NoteOn()`, `_64klang_NoteOff()`, `_64klang_Tick()`
- **`SynthController.h/cpp`** — Singleton managing the node graph, patch load/save (XML via embedded TinyXML), song export, VSTi MIDI bridging
- **`SynthAllocator.h/cpp`** — Custom memory allocator for SynthNodes (no STL containers in hot path)

### Node Graph Hierarchy

```
Synth (root) → Channel Root (×16) → Voice Manager → Voice Root → [processing nodes]
```

Global (pink) nodes exist once; voice (blue) nodes are instanced per note by VoiceManager. Global→voice connections are allowed, but not voice→global.

### GUI (`VSTiPluginSourceCode/64klang2GUI/`)

C# WPF application. Key files:
- `SynthWindow.xaml/.cs` — main window
- `SynthCanvas.xaml/.cs` — zoomable/pannable node-graph canvas
- `GUISynthNode.xaml/.cs` — individual node visuals
- `64klang2Config.xml` — XML schema defining all node types, input counts, parameter ranges, mode enumerations, and context menu structure (drives the entire GUI)
- `MultiParse/` — embedded expression parser for the Formula node

### Wrapper (`VSTiPluginSourceCode/64klang2Wrapper/`)

C++/CLI `SynthWrapper` singleton bridging the C++ Core to the .NET GUI. Manages GUI window lifecycle (`openWindow`, `closeWindow`, `invalidateGUI`, `loadPatchAndUpdateGUI`).

### VSTi (`VSTiPluginSourceCode/64klang2VSTi/`)

VST2 plugin class (`VSTXSynth`) deriving from Steinberg `AudioEffectX`. `vstxsynthproc.cpp` handles audio processing and MIDI event dispatch. Module definition via `vstsdk/vstplug.def`.

### Player (`Player/Player/`)

Minimal Win32 app for embedding in 64k intros. Includes the same engine source files (compiled without `COMPILE_VSTI` define). Patch/song data are `#included` as C arrays from headers exported by the VSTi (`64k2Patch.h`, `64k2Song.h`).

### External Libraries

- `VSTiPluginSourceCode/vstsdk/` — Steinberg VST2 SDK
- `VSTiPluginSourceCode/stk/` — Synthesis ToolKit (used for Bowed string node, FreeVerb, etc.)
- Embedded TinyXML in Core for patch file I/O

## Key Preprocessor Defines

- `COMPILE_VSTI` — defined when building as VSTi plugin (enables controller/GUI integration); not defined for Player builds
- Debug/Release/Final configurations control optimization levels; Final disables buffer security checks, exceptions, and frame pointers

## Important Conventions

- All audio processing uses `sample_t` (SSE4.1 `__m128d`), never scalar doubles in the hot path
- Node tick functions are `__fastcall` and accessed via function pointer tables
- The VSTi DLLs must be on the system PATH for DAW discovery
- Patch files are XML; exported song/patch data for the Player are C header files with raw byte arrays
