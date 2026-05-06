# 64klang

Official 64klang repository

For discussion or feedback please use our **#64klang** channel on [Discord](https://discord.gg/bH7QBxq4Ts)

---

## Overview

64klang is a modular, node-graph-based software synthesizer package intended to easily produce music for **64k intros** (small executables with a maximum filesize of 65536 bytes containing realtime audio and visuals) or **32k executable music**.

It consists of a VST3 plugin with an ImGui node-graph editor, a standalone player for embedding synthesized music into size-limited executables, and a runtime blob-loading library for tool integration.

The probably quite unique thing about 64klang is its ability to have basically unlimited options to connect things for sound creation and processing. The complete node graph is evaluated **per sample**, which makes it possible to do sample-exact feedback loops and enables options such as physical modelling (delay-based) on top of the usual AM, FM, and subtractive synthesis.

![64klang3](https://raw.githubusercontent.com/gopher-atz/64klang/master/64klang3.png)

---

## 64klang3 (current version)

64klang3 is a complete rewrite as a **VST3 plugin** with a platform-independent ImGui node-graph GUI. It is the active version under development.

### Key features

- VST3 plugin (Windows / macOS / Linux)
- ImGui-based zoomable/pannable node-graph editor
- 60+ node types: oscillators, filters, envelopes, delays, reverb, EQ, samplers, arpeggiator, spectral processing (FOURIOZA Paulstretch), and more
- 44100 Hz, SSE4.1-accelerated stereo engine (`sample_t` = two packed `double`s per stereo pair)
- Multi-instance support: multiple plugin instances share the singleton engine; first instance is render owner, others forward MIDI only
- Runtime blob loading: export patch + song as `.blob` files from the plugin; load them at runtime in any codebase without recompilation (`64klang_blob` static library)

> **Platform note:** Three node features require Windows APIs and produce silence on macOS/Linux: the **Sampler** node (GSM 6.10 compression via Windows ACM), the **GM.DLS Sampler** node (reads `GM.DLS` from the Windows system), and the **TTS** node (Windows SAPI speech synthesis). All other nodes are fully cross-platform.

### Quick start

See [BUILDING.md](BUILDING.md) for full build instructions.

**64-bit** (standard DAW plugin):
```bash
# Configure (from repo root)
cmake -B build64 -A x64 ./64klang3
cmake --build build64 --target 64klang3 --config Release
```
Output: `build64/VST3/Release/64klang3.vst3`  
Install to: `C:\Program Files\Common Files\VST3`

**32-bit** (most common target for 64k intro tools and integration):
```bash
cmake -B build32 -A Win32 ./64klang3
cmake --build build32 --target 64klang3 --config Release
```
Output: `build32/VST3/Release/64klang3.vst3`  
Install to: `C:\Program Files (x86)\Common Files\VST3`

#### Embedding the synth engine into a 64k intro

After exporting your patch and song from the plugin (**File → Export Patch** / **File → Export Song**) you get two C headers: `64k2Patch.h` and `64k2Song.h`. Compile these directly into your executable alongside a small set of engine source files — no external dependencies, no CRT required in 32-bit Release builds.

**Source files to add to your project** (all from `64klang3/src/`):

| File | Purpose |
|------|---------|
| `core/Synth.cpp` + `core/Synth.h` | Public API (`_64klang_Init`, `_64klang_Render`, …) |
| `core/SynthNode.cpp` + `core/SynthNode.h` | Per-sample node processing |
| `core/SynthAllocator.cpp` + `core/SynthAllocator.h` | Custom allocator (no STL in hot path) |
| `core/sample_t.cpp` + `core/sample_t.h` | SSE4.1 stereo sample type |

Do **not** include `SynthController.cpp`, `TinyXML`, or any `gui/`/`vst3/` source — those are plugin-only. Do **not** define `COMPILE_VSTI`.

**Minimal integration** (see [64klang3/player/main.cpp](64klang3/player/main.cpp) for a complete waveOut reference):

```cpp
// 1. Pull in the exported data
#define INCLUDE_NODES
#include "64k2Patch.h"   // defines SynthNodes, SynthMonoConstantOffset,
                         //   SynthStereoConstantOffset, SynthMaxOffset
#include "64k2Song.h"    // defines SynthStream, MAX_SAMPLES

// 2. Initialise the engine (call once, before any rendering)
_64klang_Init(SynthStream, SynthNodes,
              SynthMonoConstantOffset, SynthStereoConstantOffset, SynthMaxOffset);

// 3. Render the full song into an interleaved stereo float buffer
//    (blocks until done — run on a background thread for streaming playback)
static float soundBuffer[MAX_SAMPLES * 2];
_64klang_Render(soundBuffer);   // poll _64klang_RenderDone() for progress
```

**Linker flags for a minimal 32-bit Release build (MSVC)**:
```
/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup /NODEFAULTLIB /arch:SSE2
```
Add `msacm32.lib winmm.lib` if using waveOut playback.

### Usage guide

See [64klang3-usage.md](64klang3-usage.md) for a complete reference of mouse and keyboard interactions in the node-graph editor.

### Source layout

```
64klang3/
  src/core/        — synth engine (sample_t, SynthNode, Synth, SynthController, SynthAllocator)
  src/gui/         — ImGui node-graph editor
  src/vst3/        — VST3 plugin wrapper
  src/platform/    — platform-specific implementations
  player/          — size-optimised standalone playback EXE (Windows, compiled-in song data)
  blobplayer/      — runtime blob-loading static lib + example host
  tools/DebugExe/  — standalone WASAPI + MIDI test host (Windows)
  cmake/           — CMake helpers
```

### Runtime blob loading

The plugin exports `.blob` sidecar files alongside the usual C headers when you use **Export Song** / **Export Patch**. Link against `64klang_blob.lib` (built with `USE_BLOBS`, no GUI dependency) to load songs at runtime without recompilation:

```c
k64_blob_init_from_files("64k2Patch.h.blob", "64k2Song.h.blob");
float* buf = malloc(k64_blob_song_length() * 2 * sizeof(float));
k64_blob_render(buf);  // blocks; run on background thread for streaming
```

---

## Examples

Some 64k intros using 64klang:

- http://www.pouet.net/prod.php?which=69669
- http://www.pouet.net/prod.php?which=69654
- http://www.pouet.net/prod.php?which=71570
- http://www.pouet.net/prod.php?which=71977

Some 32k exe music tracks using 64klang (by Paul Kraus (pOWL) and Jochen Feldkötter (Virgill)):
- https://soundcloud.com/powl-music/triangularity
- https://soundcloud.com/powl-music/the-last-sequencer
- https://soundcloud.com/virgill/sets/64klang2-modular-synth
- https://soundcloud.com/virgill/sets/64klang2-synthesis-experiments

---

## History

64klang is the big brother of 4klang (https://github.com/gopher-atz/4klang).

The first version of 64klang was created around 2010/2011 for use in a 64k intro together with Fairlight (http://www.pouet.net/prod.php?which=57449). It was literally an extended 4klang — extending the concept of a signal stack to a more generic node graph, with a synth core written in Assembler and a native Win32 GUI.

64klang2 (2012–2014) rewrote the core in C++ with heavy SSE4.1 use and a .NET WPF GUI. It shipped as a VST2 plugin and was used in several 64k productions and 32k exe-music tracks. The legacy 64klang2 code now lives in `64klang2/` and is no longer under active development.

64klang3 (2026-...) is the current rewrite: VST3, 32bit/64bit, cross-platform, ImGui GUI, CMake build system.

---

## Credits

64klang was developed by **Dominik Ries** (gopher / Alcatraz).

Special thanks go out to Ralph Borson (https://github.com/revivalizer) for his invaluable blog posts on SIMD parallelisation in soft-synth development.

Also 64klang would not be in its current state without the help and input of many talented friends and demosceners (just to name a few):

pOWL, virgill, xtr1m, muhmac, reed, jco, punqtured, …
