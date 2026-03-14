# Building 64klang

## Linux

### Install dependencies

**Fedora / RHEL:**

```bash
sudo dnf install cmake gcc-c++ git mesa-libGL-devel libX11-devel
```

### Build steps

```bash
./scripts/build_linux.sh
```

This uses the `build/` directory at the repo root and builds with `Release` by default. For a debug build:

```bash
./scripts/build_linux.sh Debug
```

The VST3 plugin bundle will be produced in the build tree (e.g. under a path like `VST3/Release/64klang3.vst3` or similar, depending on the VST3 SDK layout). Install or symlink it into your user or system VST3 directory so your DAW can load it, for example:

- `~/.vst3/` (user)
- `/usr/lib/vst3/` or `/usr/local/lib/vst3/` (system)
