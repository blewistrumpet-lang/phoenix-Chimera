#!/bin/bash

# Chimera Phoenix - Deploy to Raspberry Pi Script
# Usage: ./deploy_to_pi.sh [pi_host] [gpio|no-gpio]
# Example: ./deploy_to_pi.sh pi@192.168.1.100 gpio

set -e

# Configuration
PI_HOST="${1:-pi@raspberrypi.local}"
BUILD_TYPE="${2:-no-gpio}"
PROJECT_DIR="$(dirname "$0")"
REMOTE_DIR="~/chimera_phoenix"

echo "================================================"
echo "Chimera Phoenix v3.1 - Raspberry Pi Deployment"
echo "================================================"
echo "Target: $PI_HOST"
echo "Build Type: $BUILD_TYPE"
echo ""

# Check connection
echo "Checking connection to Pi..."
if ! ssh -q -o ConnectTimeout=5 $PI_HOST "echo 'Connected!'"; then
    echo "ERROR: Cannot connect to $PI_HOST"
    echo "Please check:"
    echo "  1. Pi is powered on and connected to network"
    echo "  2. SSH is enabled on the Pi"
    echo "  3. IP address is correct"
    exit 1
fi

# Create remote directory
echo "Creating remote directory..."
ssh $PI_HOST "mkdir -p $REMOTE_DIR/JUCE_Plugin"

# Sync source files
echo "Syncing source files..."
rsync -avz --delete \
    --exclude 'Builds/MacOSX' \
    --exclude '.git' \
    --exclude '*.o' \
    --exclude '*.d' \
    --exclude 'build/' \
    --exclude '.DS_Store' \
    --exclude 'test_*.cpp' \
    --exclude '*.backup' \
    $PROJECT_DIR/JUCE_Plugin/ $PI_HOST:$REMOTE_DIR/JUCE_Plugin/

# Copy build scripts
echo "Copying build scripts..."
cat > /tmp/build_no_gpio.sh << 'EOF'
#!/bin/bash
echo "Building Chimera Phoenix WITHOUT GPIO support..."
cd ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile

if [ ! -f Makefile ]; then
    echo "Generating Makefile with Projucer..."
    cd ~/chimera_phoenix/JUCE_Plugin
    Projucer --resave ChimeraPhoenix.jucer
    cd Builds/LinuxMakefile
fi

make clean
time make CONFIG=Release -j4

if [ -f build/ChimeraPhoenix ]; then
    echo ""
    echo "✅ Build successful!"
    echo "Binary location: ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix"
    echo ""
    echo "To run: ./build/ChimeraPhoenix"
else
    echo "❌ Build failed!"
    exit 1
fi
EOF

cat > /tmp/build_with_gpio.sh << 'EOF'
#!/bin/bash
echo "Building Chimera Phoenix WITH GPIO support..."
cd ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile

if [ ! -f Makefile ]; then
    echo "Generating Makefile with Projucer..."
    cd ~/chimera_phoenix/JUCE_Plugin
    Projucer --resave ChimeraPhoenix.jucer
    cd Builds/LinuxMakefile
fi

make clean
time make CONFIG=Release CPPFLAGS="-DENABLE_GPIO=1" -j4

if [ -f build/ChimeraPhoenix ]; then
    echo ""
    echo "✅ Build successful with GPIO!"
    echo "Binary location: ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix"
    echo ""
    echo "To run with GPIO: sudo ./build/ChimeraPhoenix"
else
    echo "❌ Build failed!"
    exit 1
fi
EOF

scp /tmp/build_no_gpio.sh /tmp/build_with_gpio.sh $PI_HOST:$REMOTE_DIR/
ssh $PI_HOST "chmod +x $REMOTE_DIR/build_*.sh"

# Check if Projucer is installed
echo "Checking Projucer installation..."
if ! ssh $PI_HOST "which Projucer > /dev/null 2>&1"; then
    echo ""
    echo "⚠️  Projucer not found on Pi!"
    echo "Please install Projucer first:"
    echo "  1. SSH to Pi: ssh $PI_HOST"
    echo "  2. Run: ~/chimera_phoenix/install_projucer.sh"
    echo ""

    # Create Projucer install script
    cat > /tmp/install_projucer.sh << 'EOF'
#!/bin/bash
echo "Installing JUCE and Projucer..."
sudo apt-get update
sudo apt-get install -y git build-essential pkg-config \
    libfreetype6-dev libx11-dev libxinerama-dev libxrandr-dev \
    libxcursor-dev libxcomposite-dev mesa-common-dev \
    libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
    libwebkit2gtk-4.0-dev libgtk-3-dev

if [ ! -d ~/JUCE ]; then
    git clone https://github.com/juce-framework/JUCE.git ~/JUCE
fi

cd ~/JUCE/extras/Projucer/Builds/LinuxMakefile
make CONFIG=Release -j4
sudo cp build/Projucer /usr/local/bin/
echo "Projucer installed!"
EOF
    scp /tmp/install_projucer.sh $PI_HOST:$REMOTE_DIR/
    ssh $PI_HOST "chmod +x $REMOTE_DIR/install_projucer.sh"
fi

# Build based on type
echo ""
echo "Starting build process..."
if [ "$BUILD_TYPE" = "gpio" ]; then
    ssh $PI_HOST "$REMOTE_DIR/build_with_gpio.sh"
else
    ssh $PI_HOST "$REMOTE_DIR/build_no_gpio.sh"
fi

echo ""
echo "================================================"
echo "Deployment Complete!"
echo "================================================"
echo ""
echo "To test on Pi:"
echo "  ssh $PI_HOST"
echo "  cd $REMOTE_DIR/JUCE_Plugin/Builds/LinuxMakefile"
if [ "$BUILD_TYPE" = "gpio" ]; then
    echo "  sudo ./build/ChimeraPhoenix"
else
    echo "  ./build/ChimeraPhoenix"
fi
echo ""