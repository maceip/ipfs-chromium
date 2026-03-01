# Building IPFS-Chromium on Ubuntu

A step-by-step guide to set up your Ubuntu machine for building Chromium with
IPFS support. Each step is one script. Run them in order.

```
  You are here         What you'll have when done
  ─────────────        ────────────────────────────
  Fresh Ubuntu    ──>  Fully configured build, ready to compile
```

---

## Before You Start

| Thing          | What you need                                    |
| -------------- | ------------------------------------------------ |
| **OS**         | Ubuntu 22.04 or 24.04 (other Debian-likes may work) |
| **Disk space** | **100 GB free** (Chromium source is ~55 GB alone) |
| **RAM**        | 16 GB minimum, 32 GB recommended                 |
| **Internet**   | Stable connection (downloading ~30 GB of source)  |
| **Time**       | About 1-3 hours for setup, many more hours to actually build |
| **Patience**   | Yes                                               |

---

## The Steps

There are four scripts in this folder. Run them in order, one at a time.
Each script tells you what to do next when it finishes.

### Step 0: Check your machine

```bash
cd bootstrap/
chmod +x *.sh
./00-preflight-check.sh
```

This does not install anything. It just looks around your machine and tells
you what's ready and what's missing. Read its output.

If everything is green, skip to step 2. If things are red, continue to step 1.

---

### Step 1: Install system packages

```bash
./01-system-packages.sh
```

This uses `sudo apt-get` to install compilers, cmake, ninja, git, and other
tools. It also installs:

- **Conan** (a C++ package manager, installed via `pipx`)
- **depot_tools** (Google's build toolchain, cloned to `~/depot_tools`)
- **ccache** (makes rebuilds much faster)

When it finishes, **close your terminal and open a new one**. This is
important because it added `depot_tools` to your PATH.

To verify:

```bash
which gn       # should print something like /home/you/depot_tools/gn
which cmake    # should print /usr/bin/cmake
which ninja    # should print /usr/bin/ninja
```

---

### Step 2: Fetch Chromium source

```bash
./02-fetch-chromium.sh
```

This is the **slow** step. It downloads the entire Chromium source tree
(~30 GB) into `~/chromium/`. Then it runs Google's `install-build-deps.sh`
to install any remaining system packages Chromium needs.

Go for a walk. Make some food. This takes a while.

Want to put Chromium somewhere else? Set the variable before running:

```bash
CHROMIUM_DIR=/mnt/big-disk/chromium ./02-fetch-chromium.sh
```

---

### Step 3: Configure the build

```bash
./03-configure-ipfs-chromium.sh
```

This wires ipfs-chromium into the Chromium source tree:

1. Writes an optimized `args.gn` for your chosen build profile
2. Creates a CMake build directory at `~/ipfs-chromium-build/`
3. Runs CMake to configure everything
4. Patches the Chromium tree to include the IPFS component

By default it uses the **minimal** profile (fastest first build). See
"Build Profiles" below for other options.

If you used a custom Chromium directory in step 2, use the same one here:

```bash
CHROMIUM_DIR=/mnt/big-disk/chromium ./03-configure-ipfs-chromium.sh
```

When this finishes, it prints the exact command to build the browser. That
command is:

```bash
cmake --build ~/ipfs-chromium-build --target build_browser
```

**This guide stops here on purpose.** The actual build takes many hours and
many gigabytes of RAM. Run it when you're ready.

---

## Build Profiles

Step 3 supports four build profiles that control how Chromium is compiled.
Pick the right one for your situation:

### `minimal` (default) — fastest first build

```bash
./03-configure-ipfs-chromium.sh
# or explicitly:
BUILD_PROFILE=minimal ./03-configure-ipfs-chromium.sh
```

**Use this when:** You have never built Chromium before and want the first
build to finish as fast as possible.

**What it does:**
- Release mode, no debug symbols (`symbol_level=0`)
- Disables ~15 features you don't need for IPFS development:
  NaCl, Widevine DRM, VR/AR, printing, PDF viewer, plugins, background
  tracing, proprietary media codecs, Rust toolchain, LTO
- Uses the `lld` linker (5-10x faster than GNU ld)
- Enables ccache automatically if available

**Estimated savings:** 30-50% faster than stock Debug.

### `component-dev` — fastest incremental rebuilds

```bash
BUILD_PROFILE=component-dev CHROMIUM_PROFILE=Dev \
  ./03-configure-ipfs-chromium.sh
```

**Use this when:** You already have a successful first build and now want
to iterate quickly (change one `.cc` file, rebuild, test).

**What it does:**
- `is_component_build=true` — this is the key setting. Instead of
  producing one gigantic `chrome` binary (~2 GB), Chromium is split into
  hundreds of small shared libraries (`.so` files). When you change a
  file in `components/ipfs/`, ninja only recompiles that one file and
  re-links `libipfs.so` (~2 MB) — not the entire browser.
- Debug mode with lightweight symbols (`symbol_level=1`, line tables only)
- `use_split_dwarf=true` — debug info goes into `.dwo` sidecar files
  instead of bloating the `.o` files
- Same feature disables as `minimal`

**After the first build, changing 1 file and rebuilding takes seconds.**

### `stock-debug` — original project defaults

```bash
BUILD_PROFILE=stock-debug ./03-configure-ipfs-chromium.sh
```

Full debug build with `symbol_level=2`. Slow, large, but full debugging
support. Use this only if you need to step through Chromium internals
with a debugger.

### `stock-release` — original project defaults

```bash
BUILD_PROFILE=stock-release ./03-configure-ipfs-chromium.sh
```

Release build with the original project settings. No feature disables.

---

## Recommended Workflow

```
  FIRST TIME                        DAY-TO-DAY DEVELOPMENT
  ──────────────────────────        ──────────────────────────

  Step 0: Preflight check
  Step 1: Install packages
  Step 2: Fetch Chromium
  Step 3: Configure (minimal)       Step 3: Configure (component-dev)
  Build browser (slow, ~hours)      Build browser (slow first time)
  Verify it works                   Edit a file in component/
                                    Rebuild (fast, ~seconds)
                                    Test, repeat
```

**The two-profile approach:**

1. Use `minimal` for your first build. This gets you a working browser
   as fast as possible by compiling in Release mode with no debug symbols
   and all unnecessary features turned off.

2. Once you know the build works, switch to `component-dev` with a
   separate profile directory. This builds in Debug mode with shared
   libraries. The first component-dev build takes about as long as
   minimal, but every subsequent rebuild after changing a single file
   will finish in seconds.

You can have both profiles configured at the same time — they use
different `CHROMIUM_PROFILE` directories (`Default` and `Dev`).

---

## Why Incremental Builds Are Fast

When you set `is_component_build=true`, Chromium doesn't produce one
giant binary. Instead it compiles each component as a shared library:

```
  Without component build:           With component build:
  ────────────────────────            ─────────────────────
  Change 1 file                       Change 1 file
        |                                   |
  Compile 1 .cc file                  Compile 1 .cc file
        |                                   |
  Re-link ALL of chrome (~2 GB)       Re-link libipfs.so (~2 MB)
        |                                   |
  ~2-5 minutes                        ~3-10 seconds
```

The key settings that make this fast:

| Setting                         | What it does                                 |
| ------------------------------- | -------------------------------------------- |
| `is_component_build = true`     | Shared libraries instead of one big binary    |
| `use_lld = true`                | LLVM linker, 5-10x faster than GNU ld         |
| `symbol_level = 1`              | Line tables only (enough for stack traces)    |
| `blink_symbol_level = 0`        | No debug info for blink (we don't change it)  |
| `use_split_dwarf = true`        | Debug info in .dwo files, not in .o files     |
| `split_dwarf_inlining = false`  | Less duplicated debug info                    |
| `ccache`                        | Caches compilation results across builds      |

---

## Quick Reference

### All the variables you can customize

| Variable           | Default                      | What it controls                    |
| ------------------ | ---------------------------- | ----------------------------------- |
| `CHROMIUM_DIR`     | `~/chromium`                 | Where Chromium source lives         |
| `BUILD_DIR`        | `~/ipfs-chromium-build`      | Where the build output goes         |
| `BUILD_TYPE`       | inferred from profile        | `Debug` or `Release`                |
| `CHROMIUM_PROFILE` | `Default`                    | Chromium output profile name        |
| `BUILD_PROFILE`    | `minimal`                    | `minimal`, `component-dev`, `stock-debug`, `stock-release` |

### Build targets (after step 3)

```bash
# Build the full browser (takes hours first time)
cmake --build ~/ipfs-chromium-build --target build_browser

# Build just the IPFS client library (much faster)
cmake --build ~/ipfs-chromium-build --target in_tree_lib

# Build just the IPFS browser component
cmake --build ~/ipfs-chromium-build --target in_tree_build

# Build Electron instead of Chrome
cmake --build ~/ipfs-chromium-build --target build_electron

# Package the browser for distribution
cmake --build ~/ipfs-chromium-build --target package_browser
```

### If you already have Chromium checked out

You can skip step 2 entirely. Just point step 3 at your existing checkout:

```bash
CHROMIUM_DIR=/path/to/your/chromium ./03-configure-ipfs-chromium.sh
```

The only requirement is that `$CHROMIUM_DIR/src/.git` exists (i.e. you ran
`fetch chromium` at some point and have the source).

---

## What Each Profile Disables

Features turned off by `minimal` and `component-dev` (but NOT by `stock-*`):

| Feature               | Why it's safe to disable                          |
| --------------------- | ------------------------------------------------- |
| `enable_nacl`         | Native Client is deprecated                       |
| `enable_widevine`     | DRM — not needed for IPFS development             |
| `enable_vr`           | WebXR / VR support                                |
| `enable_cardboard`    | Google Cardboard VR                               |
| `enable_arcore`       | Augmented reality                                 |
| `enable_printing`     | Print to PDF / printers                           |
| `enable_print_preview`| Print preview UI                                  |
| `enable_basic_printing`| Basic printing support                           |
| `enable_pdf`          | Built-in PDF viewer (this one saves a lot)        |
| `enable_plugins`      | PPAPI/NPAPI plugin support (deprecated)           |
| `enable_background_tracing` | Telemetry tracing                            |
| `enable_js_type_check`| TypeScript type-checking of WebUI                 |
| `proprietary_codecs`  | H.264/AAC codecs                                  |
| `enable_hevc_parser_and_hw_decoder` | HEVC video codec                  |
| `use_thin_lto`        | Link Time Optimization (great for perf, bad for build time) |
| `enable_rust`         | Rust toolchain (avoids bootstrapping rustc)        |

`enable_extensions` is kept ON because some browser chrome depends on it.

---

## Troubleshooting

**"gclient not found"**
Close your terminal and open a new one. Step 1 added depot_tools to your
PATH but it only takes effect in new terminals.

**"Not enough disk space"**
You need ~100 GB free. Chromium source is ~55 GB, build output is ~30+ GB.

**"install-build-deps.sh failed"**
This sometimes happens on newer Ubuntu versions. The build often still works.
Try continuing to step 3 and building — if it fails then, file an issue.

**Build runs out of memory**
Add swap space, or limit parallel jobs:
```bash
cmake --build ~/ipfs-chromium-build --target build_browser -- -j4
```
The `-j4` limits ninja to 4 parallel compile jobs instead of using all cores.

**"ccache" makes no difference on first build**
That's normal. ccache is only useful on the second build onwards. The first
build fills the cache; subsequent builds reuse cached object files and are
much faster.

**Switching profiles after a build**
You can switch profiles by re-running step 3 with a different
`BUILD_PROFILE`. Use a different `CHROMIUM_PROFILE` to avoid blowing away
the previous build output:
```bash
# Keep "Default" for minimal, use "Dev" for component-dev
BUILD_PROFILE=component-dev CHROMIUM_PROFILE=Dev \
  ./03-configure-ipfs-chromium.sh
```
