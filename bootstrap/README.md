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

1. Creates a CMake build directory at `~/ipfs-chromium-build/`
2. Runs CMake to configure everything
3. Patches the Chromium tree to include the IPFS component

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

## Quick Reference

### All the variables you can customize

| Variable           | Default                    | What it controls                    |
| ------------------ | -------------------------- | ----------------------------------- |
| `CHROMIUM_DIR`     | `~/chromium`               | Where Chromium source lives         |
| `BUILD_DIR`        | `~/ipfs-chromium-build`    | Where the build output goes         |
| `BUILD_TYPE`       | `Debug`                    | `Debug` or `Release`                |
| `CHROMIUM_PROFILE` | same as `BUILD_TYPE`       | Chromium output profile name        |

### Build targets (after step 3)

```bash
# Build the full browser (takes hours)
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
