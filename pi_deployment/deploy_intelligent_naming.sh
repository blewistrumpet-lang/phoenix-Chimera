#!/bin/bash

# Deploy Intelligent Naming System to Raspberry Pi
# Created: October 18, 2025

PI_HOST="192.168.68.68"
PI_USER="pi"
PI_PATH="/home/pi/chimera_pi/AI_Server"
LOCAL_PATH="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/pi_deployment/AI_Server"

echo "========================================"
echo "Deploying Intelligent Naming System to Pi"
echo "========================================"
echo ""
echo "This script will deploy:"
echo "  1. preset_namer.py - Core naming module"
echo "  2. visionary_complete.py - Updated with intelligent naming"
echo "  3. INTELLIGENT_NAMING_SYSTEM.md - Documentation"
echo ""
echo "Target: ${PI_USER}@${PI_HOST}:${PI_PATH}"
echo ""

# Check if files exist locally
if [ ! -f "${LOCAL_PATH}/preset_namer.py" ]; then
    echo "❌ Error: preset_namer.py not found in ${LOCAL_PATH}"
    exit 1
fi

if [ ! -f "${LOCAL_PATH}/visionary_complete.py" ]; then
    echo "❌ Error: visionary_complete.py not found in ${LOCAL_PATH}"
    exit 1
fi

if [ ! -f "${LOCAL_PATH}/INTELLIGENT_NAMING_SYSTEM.md" ]; then
    echo "❌ Error: INTELLIGENT_NAMING_SYSTEM.md not found in ${LOCAL_PATH}"
    exit 1
fi

echo "✅ All files found locally"
echo ""

# Method 1: Try SCP (requires password or SSH key)
echo "Attempting deployment via SCP..."
echo "You may need to enter the Pi password..."
echo ""

scp "${LOCAL_PATH}/preset_namer.py" "${PI_USER}@${PI_HOST}:${PI_PATH}/"
if [ $? -eq 0 ]; then
    echo "✅ preset_namer.py copied"
else
    echo "❌ Failed to copy preset_namer.py"
    echo ""
    echo "Alternative: Create a tar archive for manual transfer"
    echo "Creating archive..."

    cd "${LOCAL_PATH}"
    tar -czf intelligent_naming_deploy.tar.gz \
        preset_namer.py \
        visionary_complete.py \
        INTELLIGENT_NAMING_SYSTEM.md

    echo ""
    echo "📦 Archive created: intelligent_naming_deploy.tar.gz"
    echo ""
    echo "To manually deploy:"
    echo "1. Transfer intelligent_naming_deploy.tar.gz to Pi"
    echo "2. On the Pi, run:"
    echo "   cd /home/pi/chimera_pi/AI_Server"
    echo "   tar -xzf intelligent_naming_deploy.tar.gz"
    echo "   sudo systemctl restart trinity_server_pi"
    exit 1
fi

scp "${LOCAL_PATH}/visionary_complete.py" "${PI_USER}@${PI_HOST}:${PI_PATH}/"
if [ $? -eq 0 ]; then
    echo "✅ visionary_complete.py copied"
else
    echo "❌ Failed to copy visionary_complete.py"
    exit 1
fi

scp "${LOCAL_PATH}/INTELLIGENT_NAMING_SYSTEM.md" "${PI_USER}@${PI_HOST}:${PI_PATH}/"
if [ $? -eq 0 ]; then
    echo "✅ INTELLIGENT_NAMING_SYSTEM.md copied"
else
    echo "⚠️  Documentation copy failed (non-critical)"
fi

echo ""
echo "========================================"
echo "Files deployed successfully!"
echo "========================================"
echo ""
echo "Testing deployment..."
echo ""

# Test the deployment
ssh "${PI_USER}@${PI_HOST}" << 'EOF'
cd /home/pi/chimera_pi/AI_Server
echo "Testing intelligent naming on Pi..."
python3 -c "
try:
    from preset_namer import IntelligentPresetNamer
    namer = IntelligentPresetNamer()
    test = namer.generate_name('warm vintage tape', [{'engine_name': 'TapeEcho'}], {})
    print(f'✅ Intelligent naming working! Test name: {test}')
except Exception as e:
    print(f'❌ Error: {e}')
"

echo ""
echo "Checking visionary integration..."
python3 -c "
try:
    from visionary_complete import CompleteVisionary
    v = CompleteVisionary()
    if hasattr(v, 'use_intelligent_naming'):
        print(f'✅ Visionary integration: Intelligent naming = {v.use_intelligent_naming}')
    else:
        print('⚠️  Visionary does not have intelligent naming attribute')
except Exception as e:
    print(f'❌ Error loading visionary: {e}')
"
EOF

echo ""
echo "========================================"
echo "Deployment complete!"
echo "========================================"
echo ""
echo "To apply changes, restart the Trinity server on Pi:"
echo "  ssh ${PI_USER}@${PI_HOST}"
echo "  sudo systemctl restart trinity_server_pi"
echo ""
echo "Or use the launch script:"
echo "  ./launch_chimera_hifiberry.sh"