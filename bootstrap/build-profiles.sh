#!/usr/bin/env bash
# =============================================================================
# build-profiles.sh
#
# Sourced (not executed) by 03-configure-ipfs-chromium.sh.
# Provides functions that write optimized args.gn profiles.
#
# =============================================================================

# ── Shared helpers ──────────────────────────────────────────────────────────

_detect_ccache() {
    if command -v ccache &>/dev/null; then
        echo "cc_wrapper = \"$(which ccache)\""
    fi
}

_detect_cores() {
    nproc 2>/dev/null || echo 4
}

# ── Profile: minimal ───────────────────────────────────────────────────────
# Goal: get the first `chrome` binary built as fast as possible.
#
# Strategy:
#   - Release mode (no debug symbols → much less I/O and disk)
#   - Disable every feature that is NOT needed for IPFS development
#   - use_lld=true (LLVM linker — 5-10x faster than gold/bfd for linking)
#   - Reduce symbol_level to 0 (no debug info) or 1 (line tables only)
#   - NaCl, Widevine, VR, Cardboard, AR, printing, PDF, Vulkan tests,
#     Chrome OS, background tracing, field trials — all off
#   - Thin LTO off (saves the LTO passes on first build)
#   - No official branding (avoids extra resource generation)
#
# Typical savings: 30-50% faster first build vs. stock Debug.
#
write_profile_minimal() {
    local out="$1"
    cat > "${out}/args.gn" <<'GNEOF'
# ─────────────────────────────────────────────────────────
# Profile: MINIMAL  (fastest first build)
# ─────────────────────────────────────────────────────────

# ── Core IPFS requirement ──
enable_ipfs = true

# ── Release, no debug info ──
is_debug = false
symbol_level = 0                  # No debug symbols (saves ~30 GB I/O)
blink_symbol_level = 0            # No blink debug symbols either
dcheck_always_on = false          # No DCHECK overhead

# ── Fast linker ──
use_lld = true                    # LLVM lld — 5-10x faster than GNU ld

# ── Disable features we do not need ──
enable_nacl = false               # Native Client (deprecated, big)
enable_widevine = false           # DRM (we don't need it for IPFS dev)
enable_vr = false                 # WebXR / VR (not needed)
enable_cardboard = false          # Google Cardboard VR (not needed)
enable_arcore = false             # AR (not needed)
enable_printing = false           # Print to PDF / printers (not needed)
enable_print_preview = false      # Print preview UI
enable_basic_printing = false     # Basic printing support
enable_pdf = false                # Built-in PDF viewer (saves a lot)
enable_plugins = false            # PPAPI/NPAPI plugin support (deprecated)
enable_extensions = true          # Keep — some browser chrome needs this
enable_background_tracing = false # Telemetry tracing
enable_js_type_check = false      # TypeScript type-checking of WebUI

# ── Disable optional media codecs ──
# (Keep basic media working but skip the expensive optional bits)
proprietary_codecs = false
ffmpeg_branding = "Chromium"
enable_hevc_parser_and_hw_decoder = false

# ── Skip expensive build steps ──
use_thin_lto = false              # LTO is great for perf, terrible for build time
is_official_build = false         # Official builds enable LTO and PGO
treat_warnings_as_errors = false  # Don't fail on warnings during dev
enable_rust = false               # Skip Rust toolchain bootstrap

GNEOF

    # Append ccache if available
    _detect_ccache >> "${out}/args.gn"
}


# ── Profile: component-dev ─────────────────────────────────────────────────
# Goal: fastest possible INCREMENTAL builds (you changed 1 file).
#
# Strategy:
#   - is_component_build=true (shared libraries → only re-link the changed .so)
#   - Debug mode (compiler doesn't optimize → faster compilation)
#   - symbol_level=1 (line-table only — enough for stack traces, way less I/O)
#   - use_lld=true (fast linker)
#   - split_dwarf_inlining=false + use_dwarf5=true (smaller .dwo files)
#
# After the first build, changing a single .cc file in components/ipfs
# should rebuild + relink in under 10 seconds.
#
write_profile_component_dev() {
    local out="$1"
    cat > "${out}/args.gn" <<'GNEOF'
# ─────────────────────────────────────────────────────────
# Profile: COMPONENT-DEV  (fastest incremental rebuilds)
# ─────────────────────────────────────────────────────────

# ── Core IPFS requirement ──
enable_ipfs = true

# ── Component build (THE key setting for incremental speed) ──
# Instead of linking one 2 GB chrome binary, this produces hundreds of
# small shared libraries (.so files).  When you change 1 file, ninja
# only recompiles that file and re-links the ~2 MB libipfs.so — not
# the entire browser.
is_component_build = true

# ── Debug but lightweight ──
is_debug = true
dcheck_always_on = true           # Catch bugs early
symbol_level = 1                  # Line tables only (not full DWARF)
blink_symbol_level = 0            # We don't debug blink internals

# ── Fast linker ──
use_lld = true                    # LLVM lld — critical for component builds

# ── Smaller debug info ──
use_split_dwarf = true            # .dwo side files, not embedded in .o
split_dwarf_inlining = false      # Don't duplicate inlined info

# ── Same feature disables as minimal ──
enable_nacl = false
enable_widevine = false
enable_vr = false
enable_cardboard = false
enable_arcore = false
enable_printing = false
enable_print_preview = false
enable_basic_printing = false
enable_pdf = false
enable_plugins = false
enable_extensions = true
enable_background_tracing = false
enable_js_type_check = false
proprietary_codecs = false
ffmpeg_branding = "Chromium"
enable_hevc_parser_and_hw_decoder = false
use_thin_lto = false
is_official_build = false
treat_warnings_as_errors = false
enable_rust = false

GNEOF

    # Append ccache if available
    _detect_ccache >> "${out}/args.gn"
}


# ── Profile: stock-debug (original behavior) ───────────────────────────────
# The default from before these profiles existed.
#
write_profile_stock_debug() {
    local out="$1"
    cat > "${out}/args.gn" <<'GNEOF'
# ─────────────────────────────────────────────────────────
# Profile: STOCK-DEBUG  (original settings)
# ─────────────────────────────────────────────────────────
enable_ipfs = true
is_debug = true
dcheck_always_on = true
symbol_level = 2

GNEOF
    _detect_ccache >> "${out}/args.gn"
}

write_profile_stock_release() {
    local out="$1"
    cat > "${out}/args.gn" <<'GNEOF'
# ─────────────────────────────────────────────────────────
# Profile: STOCK-RELEASE  (original settings)
# ─────────────────────────────────────────────────────────
enable_ipfs = true
is_debug = false
is_component_build = false
dcheck_always_on = false

GNEOF
    _detect_ccache >> "${out}/args.gn"
}
