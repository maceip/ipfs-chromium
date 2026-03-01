#!/usr/bin/env bash
# =============================================================================
# 02-fetch-chromium.sh
#
# WHAT THIS DOES:
#   Downloads the Chromium source code and installs Chromium's own build
#   dependencies.  This is the SLOW step — it pulls ~30 GB of source code
#   and runs Google's install-build-deps.sh to apt-install everything
#   Chromium needs that we didn't cover in step 01.
#
#   Specifically:
#     1. Creates a working directory for the Chromium checkout
#     2. Runs `fetch --nohooks chromium`  (downloads source)
#     3. Runs `gclient runhooks`          (post-download setup)
#     4. Runs `install-build-deps.sh`     (Chromium's own apt deps)
#
#   It does NOT configure or build anything.
#
# HOW TO RUN:
#   chmod +x 02-fetch-chromium.sh
#   ./02-fetch-chromium.sh
#
# OPTIONS:
#   You can set these environment variables before running:
#
#     CHROMIUM_DIR   Where to put the checkout.
#                    Default: ~/chromium
#
# HOW LONG DOES THIS TAKE?
#   Expect 30 minutes to 2+ hours depending on your internet speed and disk.
#   The download is roughly 30 GB.
#
# AFTER THIS SCRIPT:
#   Run: ./03-configure-ipfs-chromium.sh
#
# =============================================================================
set -euo pipefail

CHROMIUM_DIR="${CHROMIUM_DIR:-${HOME}/chromium}"

echo ""
echo "========================================"
echo "  Step 2: Fetch Chromium Source"
echo "========================================"
echo ""
echo "  Target directory : ${CHROMIUM_DIR}"
echo ""

# ── Sanity checks ──────────────────────────────────────────────────────────
if ! command -v gclient &>/dev/null; then
    echo "ERROR: 'gclient' not found in PATH."
    echo ""
    echo "  Did you run 01-system-packages.sh and re-open your terminal?"
    echo "  If depot_tools is at ~/depot_tools, run:"
    echo "    export PATH=\"\${HOME}/depot_tools:\${PATH}\""
    exit 1
fi

avail_gb=$(df --output=avail -BG "${HOME}" | tail -1 | tr -d ' G')
if [ "${avail_gb}" -lt 55 ]; then
    echo "WARNING: Only ${avail_gb} GB free.  Chromium needs ~55 GB."
    echo "         You can press Ctrl-C now, or continue at your own risk."
    echo ""
    sleep 5
fi

# ── 1. Create directory and fetch ──────────────────────────────────────────
echo ">>> Creating ${CHROMIUM_DIR} ..."
mkdir -p "${CHROMIUM_DIR}"
cd "${CHROMIUM_DIR}"

if [ -d "src/.git" ]; then
    echo ""
    echo "  Chromium source already exists at ${CHROMIUM_DIR}/src"
    echo "  Skipping fetch.  Running gclient sync instead..."
    echo ""
    cd src
    gclient sync --nohooks
else
    echo ""
    echo ">>> Running 'fetch --nohooks chromium' ..."
    echo "    This downloads ~30 GB.  Go for a walk. Pet a dog. Take a nap."
    echo ""
    fetch --nohooks chromium
    cd src
fi

# ── 2. Run hooks ───────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  Running gclient hooks"
echo "========================================"
echo ""
gclient runhooks

# ── 3. Install Chromium's system dependencies ──────────────────────────────
echo ""
echo "========================================"
echo "  Installing Chromium's build deps"
echo "========================================"
echo ""
echo "  This runs Google's install-build-deps.sh, which will use 'sudo apt-get'"
echo "  to install ~100 more packages that Chromium needs."
echo ""

if [ -f "build/install-build-deps.sh" ]; then
    # The --no-prompt flag avoids interactive questions.
    # --no-chromeos-fonts avoids a ~1 GB download you don't need.
    sudo bash build/install-build-deps.sh \
        --no-prompt \
        --no-chromeos-fonts \
        || {
            echo ""
            echo "  WARNING: install-build-deps.sh exited with errors."
            echo "  This sometimes happens on newer Ubuntu versions."
            echo "  The build may still work — continue and see."
            echo ""
        }
else
    echo "  WARNING: build/install-build-deps.sh not found."
    echo "  The fetch may not have completed correctly."
fi

# ── 4. Print result ────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  DONE!  Chromium source is ready."
echo "========================================"
echo ""
echo "  Chromium source tree : ${CHROMIUM_DIR}/src"
echo "  Size on disk         : $(du -sh "${CHROMIUM_DIR}" 2>/dev/null | cut -f1)"
echo ""
echo "  Next step:"
echo ""
echo "    ./03-configure-ipfs-chromium.sh"
echo ""
