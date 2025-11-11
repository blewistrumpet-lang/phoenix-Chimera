#!/bin/bash
# Chimera Phoenix v3.1 - Compact Pi Deployment & Build Script
# Single script for complete deployment and build verification

set -e

# Configuration
PI_DEV="pi@192.168.68.63"  # Development Pi
PI_PROD="pi@192.168.68.68" # Production Pi with HiFiBerry
PROJECT_DIR="$(dirname "$0")"

echo "========================================="
echo "Chimera Phoenix v3.1 - Pi Build Package"
echo "========================================="

# Function to check Pi connectivity
check_pi() {
    local host=$1
    if ssh -q -o ConnectTimeout=2 $host "echo 'Connected'" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

# Function to deploy and build
deploy_and_build() {
    local host=$1
    local pi_name=$2

    echo ""
    echo "[$pi_name] Deploying to $host..."

    # Create minimal source package
    echo "Creating source package..."
    tar czf /tmp/chimera_compact.tar.gz \
        -C "$PROJECT_DIR" \
        --exclude 'Builds/MacOSX' \
        --exclude '.git' \
        --exclude '*.o' \
        --exclude 'test_*.cpp' \
        --exclude '.DS_Store' \
        JUCE_Plugin/Source/*.cpp \
        JUCE_Plugin/Source/*.h \
        JUCE_Plugin/ChimeraPhoenix.jucer \
        JUCE_Plugin/JuceLibraryCode/ \
        2>/dev/null

    # Deploy
    echo "Transferring files..."
    scp -q /tmp/chimera_compact.tar.gz $host:~/

    # Build both versions
    ssh $host << 'REMOTE_SCRIPT'
#!/bin/bash
echo "Extracting source..."
mkdir -p ~/chimera_phoenix/JUCE_Plugin
tar xzf ~/chimera_compact.tar.gz -C ~/chimera_phoenix/

# Check for Projucer
if ! which Projucer > /dev/null 2>&1; then
    echo "ERROR: Projucer not installed!"
    echo "Run: ~/chimera_phoenix/install_projucer.sh first"
    exit 1
fi

# Generate Makefile
echo "Generating Linux Makefile..."
cd ~/chimera_phoenix/JUCE_Plugin
Projucer --resave ChimeraPhoenix.jucer

# Build WITHOUT GPIO
echo ""
echo "=== Building WITHOUT GPIO ==="
cd Builds/LinuxMakefile
make clean > /dev/null 2>&1
time make CONFIG=Release -j4

if [ -f build/ChimeraPhoenix ]; then
    echo "✅ Non-GPIO build SUCCESS"
    ./build/ChimeraPhoenix --version 2>/dev/null || echo "Binary ready"
else
    echo "❌ Non-GPIO build FAILED"
fi

# Build WITH GPIO
echo ""
echo "=== Building WITH GPIO ==="
make clean > /dev/null 2>&1
time make CONFIG=Release CPPFLAGS="-DENABLE_GPIO=1" -j4

if [ -f build/ChimeraPhoenix ]; then
    echo "✅ GPIO build SUCCESS"
    echo "Note: GPIO version requires sudo to run"
else
    echo "❌ GPIO build FAILED"
fi

# Quick validation
echo ""
echo "=== Build Validation ==="
ls -lh build/ChimeraPhoenix 2>/dev/null || echo "No binary found"
file build/ChimeraPhoenix 2>/dev/null || echo "Cannot determine file type"
ldd build/ChimeraPhoenix 2>/dev/null | head -5 || echo "Cannot check dependencies"

# Cleanup
rm -f ~/chimera_compact.tar.gz
echo ""
echo "[$HOSTNAME] Build complete!"
REMOTE_SCRIPT
}

# Main execution
echo "Checking Pi connectivity..."

# Try Development Pi first
if check_pi $PI_DEV; then
    echo "✅ Development Pi is online"
    deploy_and_build $PI_DEV "DEV Pi"
else
    echo "⚠️  Development Pi offline ($PI_DEV)"
fi

# Try Production Pi
if check_pi $PI_PROD; then
    echo "✅ Production Pi is online"
    read -p "Deploy to production Pi? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        deploy_and_build $PI_PROD "PROD Pi"
    fi
else
    echo "⚠️  Production Pi offline ($PI_PROD)"
fi

# Summary
echo ""
echo "========================================="
echo "Deployment Summary"
echo "========================================="
if check_pi $PI_DEV || check_pi $PI_PROD; then
    echo "Builds completed. Check output above for results."
else
    echo "No Pi systems available. Connect and run:"
    echo "  ./pi_deployment_compact.sh"
fi