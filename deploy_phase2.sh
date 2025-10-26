#!/bin/bash
# Phase 2 Deployment Script - Run this on the Pi
# Usage: ./deploy_phase2.sh [pi_hostname]

set -e  # Exit on error

PI_HOST="${1:-192.168.68.65}"  # Default to .65
REMOTE_USER="branden"
REMOTE_PATH="~/phoenix-Chimera"

echo "=========================================="
echo "Phase 2 GPIO Fix Deployment"
echo "Target: ${REMOTE_USER}@${PI_HOST}"
echo "=========================================="

# Check if we're already on the Pi
if [ -f /proc/device-tree/model ] && grep -q "Raspberry Pi" /proc/device-tree/model; then
    echo "✓ Running on Raspberry Pi - deploying locally"

    cd ~/phoenix-Chimera || { echo "Error: phoenix-Chimera directory not found"; exit 1; }

    echo "→ Fetching Phase 2 branch..."
    git fetch origin fix/phase2-preset-ab-coalesce

    echo "→ Checking out branch..."
    git checkout fix/phase2-preset-ab-coalesce

    echo "→ Pulling latest changes..."
    git pull origin fix/phase2-preset-ab-coalesce

    echo "→ Navigating to build directory..."
    cd pi_deployment/JUCE_Plugin/Builds/LinuxMakefile

    echo "→ Cleaning previous build..."
    rm -rf build

    echo "→ Building (this may take 2-3 minutes)..."
    make -j4 2>&1 | tee /tmp/chimera_build.log

    if [ $? -eq 0 ]; then
        echo ""
        echo "=========================================="
        echo "✅ BUILD SUCCESSFUL"
        echo "=========================================="
        echo ""
        echo "To run: ./build/ChimeraPhoenix"
        echo "To monitor logs: tail -f /tmp/chimera_debug.txt"
        echo ""
        echo "Verification checklist in: ~/phoenix-Chimera/PHASE2_OUTCOME.md"
    else
        echo ""
        echo "=========================================="
        echo "❌ BUILD FAILED"
        echo "=========================================="
        echo "Check log: /tmp/chimera_build.log"
        exit 1
    fi
else
    echo "✓ Running from Mac/desktop - deploying via SSH"
    echo "→ Syncing code to Pi..."

    rsync -avz --delete \
        --exclude 'build/' \
        --exclude '.git/' \
        --exclude '*.o' \
        ./ ${REMOTE_USER}@${PI_HOST}:${REMOTE_PATH}/

    echo "→ Running remote build..."
    ssh ${REMOTE_USER}@${PI_HOST} "cd ${REMOTE_PATH} && ./deploy_phase2.sh"
fi
