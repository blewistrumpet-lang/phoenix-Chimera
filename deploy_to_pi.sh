#!/bin/bash
# ChimeraPhoenix Raspberry Pi 5 Deployment Script
# Run this on your Mac to prepare deployment package

set -e

echo "🎯 ChimeraPhoenix → Raspberry Pi 5 Deployment"
echo "=============================================="
echo ""

# Configuration
PI_USER="${PI_USER:-pi}"
PI_HOST="${PI_HOST:-raspberrypi.local}"
PROJECT_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
DEPLOY_DIR="${PROJECT_DIR}/pi_deployment"

echo "📦 Step 1: Creating deployment package..."

# Create deployment directory
rm -rf "${DEPLOY_DIR}"
mkdir -p "${DEPLOY_DIR}"

# Copy essential files
echo "   Copying JUCE plugin source..."
rsync -av --exclude='Builds/MacOSX/build' \
          --exclude='*.o' \
          --exclude='*.a' \
          --exclude='.git' \
          "${PROJECT_DIR}/JUCE_Plugin/" "${DEPLOY_DIR}/JUCE_Plugin/"

echo "   Copying AI Server..."
rsync -av "${PROJECT_DIR}/AI_Server/" "${DEPLOY_DIR}/AI_Server/"

echo "   Copying setup script..."
cp "${PROJECT_DIR}/pi_setup.sh" "${DEPLOY_DIR}/" 2>/dev/null || true

echo ""
echo "✅ Deployment package created at: ${DEPLOY_DIR}"
echo ""
echo "📡 Step 2: Transfer to Raspberry Pi"
echo "   Choose your method:"
echo ""
echo "   Option A - SSH Transfer (requires Pi on network):"
echo "   --------------------------------------------------"
echo "   rsync -avz --progress ${DEPLOY_DIR}/ ${PI_USER}@${PI_HOST}:~/ChimeraPhoenix/"
echo ""
echo "   Option B - USB/SD Card Transfer:"
echo "   ---------------------------------"
echo "   1. Insert USB drive or SD card"
echo "   2. cp -r ${DEPLOY_DIR} /Volumes/YOUR_DRIVE/ChimeraPhoenix"
echo "   3. Eject and move to Pi"
echo "   4. On Pi: cp -r /media/pi/YOUR_DRIVE/ChimeraPhoenix ~/"
echo ""
echo "📋 Step 3: On the Raspberry Pi, run:"
echo "   cd ~/ChimeraPhoenix"
echo "   chmod +x pi_setup.sh"
echo "   ./pi_setup.sh"
echo ""
