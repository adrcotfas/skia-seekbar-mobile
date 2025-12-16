#!/bin/bash
# scripts/setup.sh
# Sets up the Skia SeekBar project with all dependencies

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== Skia SeekBar Project Setup ==="
echo ""

# Check prerequisites
echo "[1/5] Checking prerequisites..."
command -v git >/dev/null || { echo "ERROR: git required"; exit 1; }
command -v python3 >/dev/null || { echo "ERROR: python3 required"; exit 1; }
command -v cmake >/dev/null || { echo "ERROR: cmake required"; exit 1; }
echo "  ✓ All prerequisites found"

# Initialize submodules
echo ""
echo "[2/5] Initializing git submodules..."
cd "$PROJECT_ROOT"
git submodule update --init --recursive --depth 1
echo "  ✓ Submodules initialized"

# Add depot_tools to PATH (required for Skia tools)
echo ""
echo "[3/5] Setting up depot_tools..."
export PATH="$PROJECT_ROOT/third_party/depot_tools:$PATH"
echo "  ✓ depot_tools added to PATH"

# Sync Skia dependencies
echo ""
echo "[4/5] Syncing Skia dependencies (this may take a while)..."
cd "$PROJECT_ROOT/third_party/skia"
python3 tools/git-sync-deps
echo "  ✓ Skia dependencies synced"

# Fetch ninja for building Skia
echo ""
echo "[5/5] Fetching ninja..."
python3 bin/fetch-ninja
echo "  ✓ ninja fetched"

echo ""
echo "=== Setup complete! ==="

