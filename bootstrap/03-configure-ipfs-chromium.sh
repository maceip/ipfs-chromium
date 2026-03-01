#!/usr/bin/env bash
# =============================================================================
# 03-configure-ipfs-chromium.sh
#
# WHAT THIS DOES:
#   Configures the ipfs-chromium build system on top of the Chromium source
#   tree you fetched in step 02.  After this script finishes you will have
#   a fully configured build directory, ready for "cmake --build".
#
#   Specifically:
#     1. Writes an optimized args.gn for your chosen build profile
#     2. Points CMake at your Chromium checkout
#     3. Generates the build directory with Ninja
#     4. Runs the "setup_in_tree" target (patches Chromium, syncs sources)
#     5. Prints the exact command to build the browser
#
#   It does NOT start the actual build (that takes hours).
#
# HOW TO RUN:
#   chmod +x 03-configure-ipfs-chromium.sh
#   ./03-configure-ipfs-chromium.sh
#
# OPTIONS:
#   You can set these environment variables before running:
#
#     CHROMIUM_DIR      Where Chromium was checked out.
#                       Default: ~/chromium
#
#     BUILD_DIR         Where to put the ipfs-chromium build.
#                       Default: ~/ipfs-chromium-build
#
#     BUILD_TYPE        "Debug" or "Release".
#                       Default: inferred from profile
#
#     CHROMIUM_PROFILE  The Chromium output profile name.
#                       Default: Default
#
#     BUILD_PROFILE     Which args.gn profile to use. One of:
#
#                       minimal        Fastest FIRST build. Release mode,
#                                      all unnecessary features disabled.
#                                      (Default)
#
#                       component-dev  Fastest INCREMENTAL builds. Debug
#                                      mode + component build so changing
#                                      1 file only re-links a small .so.
#
#                       stock-debug    Original project defaults (Debug).
#
#                       stock-release  Original project defaults (Release).
#
# TYPICAL WORKFLOW:
#   First time?  Use "minimal" to get a working browser as fast as possible:
#
#     ./03-configure-ipfs-chromium.sh
#     cmake --build ~/ipfs-chromium-build --target build_browser
#
#   Now switch to "component-dev" for fast iteration:
#
#     BUILD_PROFILE=component-dev CHROMIUM_PROFILE=Dev \
#       ./03-configure-ipfs-chromium.sh
#     cmake --build ~/ipfs-chromium-build --target build_browser
#
#   After that, editing a single .cc file and rebuilding takes seconds.
#
# =============================================================================
set -euo pipefail

# ── Defaults ────────────────────────────────────────────────────────────────
BUILD_PROFILE="${BUILD_PROFILE:-minimal}"
CHROMIUM_DIR="${CHROMIUM_DIR:-${HOME}/chromium}"
CHROMIUM_SRC="${CHROMIUM_DIR}/src"
BUILD_DIR="${BUILD_DIR:-${HOME}/ipfs-chromium-build}"
CHROMIUM_PROFILE="${CHROMIUM_PROFILE:-Default}"

# Infer BUILD_TYPE from profile if not explicitly set
if [ -z "${BUILD_TYPE:-}" ]; then
    case "${BUILD_PROFILE}" in
        minimal|stock-release)
            BUILD_TYPE="Release"
            ;;
        component-dev|stock-debug)
            BUILD_TYPE="Debug"
            ;;
        *)
            BUILD_TYPE="Release"
            ;;
    esac
fi

# Figure out where ipfs-chromium source lives (this repo)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IPFS_CHROMIUM_SRC="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Source the profile definitions
source "${SCRIPT_DIR}/build-profiles.sh"

echo ""
echo "========================================"
echo "  Step 3: Configure IPFS-Chromium"
echo "========================================"
echo ""
echo "  ipfs-chromium source : ${IPFS_CHROMIUM_SRC}"
echo "  Chromium source      : ${CHROMIUM_SRC}"
echo "  Build directory      : ${BUILD_DIR}"
echo "  Build type           : ${BUILD_TYPE}"
echo "  Chromium profile     : ${CHROMIUM_PROFILE}"
echo "  Build profile        : ${BUILD_PROFILE}"
echo ""

# ── Sanity checks ──────────────────────────────────────────────────────────
if [ ! -d "${CHROMIUM_SRC}/.git" ]; then
    echo "ERROR: Chromium source not found at ${CHROMIUM_SRC}"
    echo ""
    echo "  Either run 02-fetch-chromium.sh first, or set CHROMIUM_DIR:"
    echo "    CHROMIUM_DIR=/path/to/chromium ./03-configure-ipfs-chromium.sh"
    exit 1
fi

if ! command -v cmake &>/dev/null; then
    echo "ERROR: cmake not found. Run 01-system-packages.sh first."
    exit 1
fi

if ! command -v ninja &>/dev/null; then
    echo "ERROR: ninja not found. Run 01-system-packages.sh first."
    exit 1
fi

# ── 1. Write args.gn ─────────────────────────────────────────────────────
CHROMIUM_OUT="${CHROMIUM_SRC}/out/${CHROMIUM_PROFILE}"
mkdir -p "${CHROMIUM_OUT}"

echo ">>> Writing args.gn for profile: ${BUILD_PROFILE}"
case "${BUILD_PROFILE}" in
    minimal)
        write_profile_minimal "${CHROMIUM_OUT}"
        ;;
    component-dev)
        write_profile_component_dev "${CHROMIUM_OUT}"
        ;;
    stock-debug)
        write_profile_stock_debug "${CHROMIUM_OUT}"
        ;;
    stock-release)
        write_profile_stock_release "${CHROMIUM_OUT}"
        ;;
    *)
        echo "ERROR: Unknown BUILD_PROFILE '${BUILD_PROFILE}'."
        echo "  Choose one of: minimal, component-dev, stock-debug, stock-release"
        exit 1
        ;;
esac

echo ""
echo "  args.gn written to: ${CHROMIUM_OUT}/args.gn"
echo "  Contents:"
echo "  ─────────────────────────────────────"
sed 's/^/    /' "${CHROMIUM_OUT}/args.gn"
echo "  ─────────────────────────────────────"
echo ""

# ── 2. CMake configure ─────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  Running CMake configure"
echo "========================================"
echo ""

mkdir -p "${BUILD_DIR}"

cmake \
    -G Ninja \
    -S "${IPFS_CHROMIUM_SRC}" \
    -B "${BUILD_DIR}" \
    -D CMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -D CHROMIUM_SOURCE_TREE="${CHROMIUM_SRC}" \
    -D CHROMIUM_PROFILE="${CHROMIUM_PROFILE}" \
    -D USE_DOXYGEN=FALSE

echo ""
echo "  [OK] CMake configure complete."

# ── 3. Setup in-tree ───────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  Patching Chromium (setup_in_tree)"
echo "========================================"
echo ""
echo "  This copies ipfs-chromium sources into the Chromium tree and"
echo "  applies version-specific patches."
echo ""

cmake --build "${BUILD_DIR}" --target setup_in_tree

echo ""
echo "  [OK] Chromium tree is patched and ready."

# ── 4. Done ─────────────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  DONE!  Everything is configured."
echo "========================================"
echo ""
echo "  Your build directory  : ${BUILD_DIR}"
echo "  Chromium output dir   : ${CHROMIUM_OUT}"
echo "  Build profile         : ${BUILD_PROFILE}"
echo ""
echo "  ====================================================="
echo "  TO BUILD THE BROWSER (this will take a long time):"
echo ""
echo "    cmake --build ${BUILD_DIR} --target build_browser"
echo ""
echo "  ====================================================="
echo ""

if [ "${BUILD_PROFILE}" = "minimal" ]; then
    echo "  TIP: You used the 'minimal' profile for a fast first build."
    echo "  Once that succeeds, switch to 'component-dev' for fast iteration:"
    echo ""
    echo "    BUILD_PROFILE=component-dev CHROMIUM_PROFILE=Dev \\"
    echo "      ./03-configure-ipfs-chromium.sh"
    echo ""
    echo "  Then changing 1 file and rebuilding takes seconds, not hours."
    echo ""
fi

if [ "${BUILD_PROFILE}" = "component-dev" ]; then
    echo "  TIP: You're using 'component-dev' — incremental builds are fast."
    echo "  After the first build, changing a .cc file in components/ipfs"
    echo "  and rebuilding should take under 10 seconds."
    echo ""
    echo "  Quick rebuild cycle:"
    echo "    1. Edit a file in component/ (this repo)"
    echo "    2. cmake --build ${BUILD_DIR} --target in_tree_build"
    echo "       (this syncs your change and rebuilds just the IPFS component)"
    echo "    3. cmake --build ${BUILD_DIR} --target build_browser"
    echo "       (re-links the browser — fast because only libipfs.so changed)"
    echo ""
fi

echo "  Other useful targets:"
echo "    in_tree_lib       Build just the IPFS client library"
echo "    in_tree_build     Build just the IPFS component"
echo "    build_browser     Build the full Chromium browser"
echo "    build_electron    Build Electron instead"
echo ""
echo "  The browser binary will appear at:"
echo "    ${CHROMIUM_OUT}/chrome"
echo ""
