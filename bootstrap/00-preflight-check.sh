#!/usr/bin/env bash
# =============================================================================
# 00-preflight-check.sh
#
# WHAT THIS DOES:
#   Checks your Ubuntu machine to see if it is ready to build Chromium.
#   It does NOT install anything. It just looks around and tells you
#   what is good and what is missing.
#
# HOW TO RUN:
#   chmod +x 00-preflight-check.sh
#   ./00-preflight-check.sh
#
# =============================================================================
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

pass_count=0
warn_count=0
fail_count=0

pass()  { echo -e "  ${GREEN}[OK]${NC}   $1"; ((pass_count++)); }
warn()  { echo -e "  ${YELLOW}[WARN]${NC} $1"; ((warn_count++)); }
fail()  { echo -e "  ${RED}[FAIL]${NC} $1"; ((fail_count++)); }

echo ""
echo "========================================"
echo "  IPFS-Chromium Build Preflight Check"
echo "========================================"
echo ""

# ── OS ──────────────────────────────────────────────────────────────────────
echo "--- Operating System ---"
if [ -f /etc/os-release ]; then
    . /etc/os-release
    echo "  Detected: ${PRETTY_NAME}"
    case "${ID}" in
        ubuntu|debian)
            pass "Supported distro (${ID})"
            ;;
        fedora|centos|rhel)
            warn "Partially supported (${ID}) — install-build-deps.sh may not work"
            ;;
        *)
            warn "Untested distro (${ID}) — you may need to adapt package names"
            ;;
    esac
else
    warn "Could not detect OS (no /etc/os-release)"
fi

# ── Disk space ──────────────────────────────────────────────────────────────
echo ""
echo "--- Disk Space ---"
avail_gb=$(df --output=avail -BG . | tail -1 | tr -d ' G')
echo "  Available in current directory: ${avail_gb} GB"
if [ "${avail_gb}" -ge 100 ]; then
    pass "Plenty of disk space (${avail_gb} GB >= 100 GB)"
elif [ "${avail_gb}" -ge 60 ]; then
    warn "Tight on disk (${avail_gb} GB). Chromium needs ~55 GB for source alone."
else
    fail "Not enough disk space (${avail_gb} GB). Need at least 60 GB, 100+ recommended."
fi

# ── RAM ─────────────────────────────────────────────────────────────────────
echo ""
echo "--- Memory ---"
total_ram_kb=$(grep MemTotal /proc/meminfo | awk '{print $2}')
total_ram_gb=$((total_ram_kb / 1024 / 1024))
echo "  Total RAM: ${total_ram_gb} GB"
if [ "${total_ram_gb}" -ge 16 ]; then
    pass "Good amount of RAM (${total_ram_gb} GB)"
elif [ "${total_ram_gb}" -ge 8 ]; then
    warn "RAM is low (${total_ram_gb} GB). 16 GB+ recommended. Consider adding swap."
else
    fail "Not enough RAM (${total_ram_gb} GB). Linking chrome needs ~8 GB minimum."
fi

# ── CPU ─────────────────────────────────────────────────────────────────────
echo ""
echo "--- CPU ---"
cpu_count=$(nproc 2>/dev/null || echo 1)
echo "  Logical cores: ${cpu_count}"
if [ "${cpu_count}" -ge 8 ]; then
    pass "Good core count (${cpu_count})"
elif [ "${cpu_count}" -ge 4 ]; then
    warn "Build will be slow with ${cpu_count} cores. 8+ recommended."
else
    fail "Very few cores (${cpu_count}). The build will take a very long time."
fi

# ── Required commands ───────────────────────────────────────────────────────
echo ""
echo "--- Required Tools ---"

check_cmd() {
    local cmd="$1"
    local min_version="${2:-}"
    local install_hint="${3:-}"

    if command -v "${cmd}" &>/dev/null; then
        local ver
        ver=$("${cmd}" --version 2>&1 | head -1) || ver="(version unknown)"
        pass "${cmd} found: ${ver}"
    else
        fail "${cmd} NOT found.${install_hint:+ Fix: ${install_hint}}"
    fi
}

check_cmd "git"      ""  "sudo apt-get install git"
check_cmd "python3"  ""  "sudo apt-get install python3"
check_cmd "cmake"    ""  "sudo apt-get install cmake   (need 3.22+)"
check_cmd "ninja"    ""  "sudo apt-get install ninja-build"
check_cmd "g++"      ""  "sudo apt-get install g++"
check_cmd "gcc"      ""  "sudo apt-get install gcc"
check_cmd "curl"     ""  "sudo apt-get install curl"

# cmake version check
if command -v cmake &>/dev/null; then
    cmake_ver=$(cmake --version | head -1 | grep -oP '\d+\.\d+')
    cmake_major=$(echo "${cmake_ver}" | cut -d. -f1)
    cmake_minor=$(echo "${cmake_ver}" | cut -d. -f2)
    if [ "${cmake_major}" -gt 3 ] || { [ "${cmake_major}" -eq 3 ] && [ "${cmake_minor}" -ge 22 ]; }; then
        pass "cmake version ${cmake_ver} >= 3.22"
    else
        fail "cmake version ${cmake_ver} is too old. Need 3.22+"
    fi
fi

echo ""
echo "--- Optional but Recommended ---"

check_cmd "ccache"   ""  "sudo apt-get install ccache  (saves HOURS on rebuilds)"
check_cmd "conan"    ""  "pipx install conan"
check_cmd "pipx"     ""  "sudo apt-get install pipx"

# Check for lsb_release (used by install-build-deps.sh)
check_cmd "lsb_release" "" "sudo apt-get install lsb-release"

# depot_tools
echo ""
echo "--- depot_tools ---"
if command -v gn &>/dev/null; then
    pass "gn found in PATH (depot_tools is set up)"
elif [ -d "${HOME}/depot_tools" ]; then
    warn "depot_tools found at ~/depot_tools but not in PATH"
    echo "       Fix: export PATH=\"\${HOME}/depot_tools:\${PATH}\""
else
    warn "depot_tools not found. Run 01-system-packages.sh to install."
fi

# ── Summary ─────────────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo "  Results:  ${pass_count} passed, ${warn_count} warnings, ${fail_count} failed"
echo "========================================"

if [ "${fail_count}" -gt 0 ]; then
    echo -e "  ${RED}Some checks failed.${NC} Fix them before proceeding."
    echo "  Run: ./01-system-packages.sh  to install missing dependencies."
    exit 1
elif [ "${warn_count}" -gt 0 ]; then
    echo -e "  ${YELLOW}Mostly ready, but check the warnings above.${NC}"
    exit 0
else
    echo -e "  ${GREEN}All clear! You are ready to build.${NC}"
    exit 0
fi
