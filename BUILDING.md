# Building 64klang3

## Windows

### Prerequisites

CMake: https://cmake.org/download/

Visual Studio: https://visualstudio.microsoft.com/de/downloads/

### Build steps

For **64bit** plugin, on the repository root run:
```bash
cmake -B build64 -A x64 ./64klang3
cmake --build build64 --target 64klang3 --config Release
```
Output will be in `build64\VST3\Release\64klang3.vst3`

Symlink or copy to: `C:\Program Files\Common Files\VST3`

---

For **32bit** plugin, on the repository root run:
```bash
cmake -B build32 -A Win32 ./64klang3
cmake --build build32 --target 64klang3 --config Release
```

Output will be in `build32\VST3\Release\64klang3.vst3`

Symlink or copy to: `C:\Program Files (x86)\Common Files\VST3`


## Linux

### Install dependencies

**Ubunt / Debian**
```bash
sudo apt install cmake g++ git libx11-dev
```

**Fedora / RHEL:**

```bash
sudo dnf install cmake gcc-c++ git libX11-devel
```

### Build steps

```bash
./64klang3/build_linux.sh
```

This uses the `build/` directory at the repo root and builds with `Release` by default. For a debug build:

```bash
./64klang3/build_linux.sh Debug
```

The VST3 plugin bundle will be produced in the build tree (e.g. under a path like `VST3/Release/64klang3.vst3` or similar, depending on the VST3 SDK layout). Install or symlink it into your user or system VST3 directory so your DAW can load it, for example:

- `~/.vst3/` (user)
- `/usr/lib/vst3/` or `/usr/local/lib/vst3/` (system)
