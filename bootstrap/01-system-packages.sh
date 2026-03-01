#!/usr/bin/env bash
# =============================================================================
# 01-system-packages.sh
#
# WHAT THIS DOES:
#   Installs every Ubuntu package and tool you need before you can fetch and
#   build Chromium with the IPFS component.
#
#   Specifically it installs:
#     1. System packages via apt  (compilers, cmake, ninja, git, etc.)
#     2. pipx + Conan             (C++ dependency manager)
#     3. depot_tools              (Google's toolchain: gn, gclient, ninja wrappers)
#     4. ccache                   (optional but saves hours on rebuilds)
#
#   It does NOT download Chromium source or start any builds.
#
# HOW TO RUN:
#   chmod +x 01-system-packages.sh
#   ./01-system-packages.sh
#
# AFTER THIS SCRIPT:
#   Close and re-open your terminal (so PATH changes take effect),
#   then run: ./02-fetch-chromium.sh
#
# =============================================================================
set -euo pipefail

echo ""
echo "========================================"
echo "  Step 1: Install system packages"
echo "========================================"
echo ""

# ── 1. APT packages ────────────────────────────────────────────────────────
echo ">>> Updating apt package list..."
sudo apt-get update

echo ""
echo ">>> Installing build-essential packages..."
sudo apt-get install --yes \
    build-essential   \
    g++               \
    gcc               \
    clang             \
    lld               \
    git               \
    git-lfs           \
    curl              \
    wget              \
    python3           \
    python3-pip       \
    python3-venv      \
    cmake             \
    ninja-build       \
    ccache            \
    pkg-config        \
    pipx              \
    lsb-release       \
    binutils          \
    libc6-dev         \
    doxygen           \
    graphviz          \
    valgrind          \
    libfuse2

# Perl modules used by lcov (coverage tooling)
sudo apt-get install --yes \
    libcapture-tiny-perl \
    libdatetime-perl     \
    || echo "  (perl modules are optional — coverage reports may not work without them)"

echo ""
echo "  [OK] System packages installed."

# ── 2. pipx + Conan ────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  Step 2: Install Conan (C++ package manager)"
echo "========================================"
echo ""

# Make sure pipx's bin dir is in PATH for this session
export PATH="${HOME}/.local/bin:${PATH}"
pipx ensurepath

if command -v conan &>/dev/null; then
    echo "  Conan is already installed: $(conan --version)"
else
    echo ">>> Installing Conan via pipx..."
    pipx install conan
fi

echo ""
echo ">>> Setting up default Conan profile..."
conan profile detect --force
echo "  [OK] Conan profile created at: $(conan profile path default)"

# ── 3. depot_tools ──────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  Step 3: Install depot_tools"
echo "========================================"
echo ""

DEPOT_TOOLS_DIR="${HOME}/depot_tools"

if [ -d "${DEPOT_TOOLS_DIR}" ]; then
    echo "  depot_tools already exists at ${DEPOT_TOOLS_DIR}"
    echo "  Updating..."
    (cd "${DEPOT_TOOLS_DIR}" && git pull --quiet)
else
    echo ">>> Cloning depot_tools to ${DEPOT_TOOLS_DIR}..."
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git \
        "${DEPOT_TOOLS_DIR}"
fi

# Add depot_tools to PATH in shell config if not already there
SHELL_RC="${HOME}/.bashrc"
if [ -n "${ZSH_VERSION:-}" ] || [ "$(basename "${SHELL:-}")" = "zsh" ]; then
    SHELL_RC="${HOME}/.zshrc"
fi

if ! grep -q "depot_tools" "${SHELL_RC}" 2>/dev/null; then
    echo "" >> "${SHELL_RC}"
    echo "# Added by ipfs-chromium bootstrap" >> "${SHELL_RC}"
    echo "export PATH=\"${DEPOT_TOOLS_DIR}:\${PATH}\"" >> "${SHELL_RC}"
    echo "  [OK] Added depot_tools to ${SHELL_RC}"
else
    echo "  depot_tools PATH entry already in ${SHELL_RC}"
fi

# Make it available right now too
export PATH="${DEPOT_TOOLS_DIR}:${PATH}"

echo ""
echo "  [OK] depot_tools ready at: ${DEPOT_TOOLS_DIR}"

# ── 4. ccache ───────────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  Step 4: Configure ccache"
echo "========================================"
echo ""

if command -v ccache &>/dev/null; then
    # Set a generous cache size — Chromium is enormous
    ccache --max-size=50G
    echo "  [OK] ccache configured (50 GB max)."
    echo "  Tip: 'ccache -s' shows stats.  First build fills the cache; rebuilds are fast."
else
    echo "  (ccache not found — skipping. It's optional but saves hours.)"
fi

# ── 5. Verify ───────────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  Verification"
echo "========================================"
echo ""

echo "  Tool versions:"
echo "    git      : $(git --version)"
echo "    python3  : $(python3 --version 2>&1)"
echo "    cmake    : $(cmake --version | head -1)"
echo "    ninja    : $(ninja --version)"
echo "    g++      : $(g++ --version | head -1)"
echo "    conan    : $(conan --version 2>&1)"
echo "    ccache   : $(ccache --version 2>&1 | head -1 || echo 'not installed')"
echo ""

if [ -x "${DEPOT_TOOLS_DIR}/gclient" ]; then
    echo "    gclient  : found at ${DEPOT_TOOLS_DIR}/gclient"
else
    echo "    gclient  : ${DEPOT_TOOLS_DIR}/gclient (will self-bootstrap on first use)"
fi

echo ""
echo "========================================"
echo "  DONE!  System packages are installed."
echo "========================================"
echo ""
echo "  IMPORTANT: Close and re-open your terminal, then run:"
echo ""
echo "    ./02-fetch-chromium.sh"
echo ""
