#!/bin/bash
# ChimeraPhoenix Raspberry Pi 5 Setup Script
# Run this ON the Raspberry Pi after transferring files

set -e

# Get the directory where this script is located (MUST BE FIRST before any cd commands!)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "🔧 ChimeraPhoenix Raspberry Pi 5 Setup"
echo "======================================"
echo ""
echo "Hardware: Raspberry Pi 5 (8GB)"
echo "Target: Standalone audio application"
echo "Script location: $SCRIPT_DIR"
echo ""

# Color codes for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if running on ARM
if [[ $(uname -m) != "aarch64" ]] && [[ $(uname -m) != "armv7l" ]]; then
    echo -e "${RED}⚠️  Warning: Not running on ARM architecture${NC}"
    echo "Detected: $(uname -m)"
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

echo -e "${BLUE}📦 Step 1: System Update${NC}"
sudo apt update
sudo apt upgrade -y

echo ""
echo -e "${BLUE}📦 Step 2: Install Build Dependencies${NC}"
sudo apt install -y \
    build-essential \
    git \
    cmake \
    pkg-config \
    libasound2-dev \
    libjack-jackd2-dev \
    ladspa-sdk \
    libfreetype6-dev \
    libx11-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxcursor-dev \
    libcurl4-openssl-dev \
    mesa-common-dev \
    libasound2-dev \
    freeglut3-dev \
    libxcomposite-dev \
    libwebkit2gtk-4.0-dev

echo ""
echo -e "${BLUE}📦 Step 3: Install Audio Server (JACK)${NC}"
sudo apt install -y jackd2 qjackctl

# Prevent JACK from asking about realtime priority during install
if [ ! -f /etc/security/limits.d/audio.conf ]; then
    echo "Configuring audio permissions..."
    sudo tee /etc/security/limits.d/audio.conf > /dev/null <<EOF
@audio   -  rtprio     95
@audio   -  memlock    unlimited
EOF
    sudo usermod -a -G audio $USER
fi

echo ""
echo -e "${BLUE}📦 Step 4: Clone/Update JUCE Framework${NC}"
if [ ! -d "$HOME/JUCE" ]; then
    echo "Cloning JUCE..."
    git clone --depth=1 --branch=7.0.12 https://github.com/juce-framework/JUCE.git "$HOME/JUCE"
else
    echo "JUCE already exists at $HOME/JUCE"
fi

# Build Projucer
echo ""
echo -e "${BLUE}🔨 Step 5: Building Projucer${NC}"
if [ ! -f "$HOME/JUCE/extras/Projucer/Builds/LinuxMakefile/build/Projucer" ]; then
    cd "$HOME/JUCE/extras/Projucer/Builds/LinuxMakefile"
    make -j4 CONFIG=Release
else
    echo "Projucer already built"
fi

PROJUCER="$HOME/JUCE/extras/Projucer/Builds/LinuxMakefile/build/Projucer"

echo ""
echo -e "${BLUE}🔨 Step 6: Generate Linux Makefile for ChimeraPhoenix${NC}"
cd "$SCRIPT_DIR/JUCE_Plugin"

# Use Projucer to resave and generate Linux build files
$PROJUCER --resave ChimeraPhoenix.jucer

echo ""
echo -e "${BLUE}🔨 Step 7: Build ChimeraPhoenix${NC}"

# Check if Linux Makefile exists
if [ -d "Builds/LinuxMakefile" ]; then
    cd Builds/LinuxMakefile

    # Build with ARM optimizations
    echo "Building for ARM (this will take 10-20 minutes)..."
    make -j4 CONFIG=Release \
        CXXFLAGS="-march=native -mtune=native -O3 -DJUCE_JACK=1"

    BUILD_STATUS=$?

    if [ $BUILD_STATUS -eq 0 ]; then
        echo ""
        echo -e "${GREEN}✅ Build successful!${NC}"
        echo "Binary location: $(pwd)/build/ChimeraPhoenix"

        # Make executable
        chmod +x build/ChimeraPhoenix

        # Create launcher script with correct path
        BUILD_PATH="$(pwd)/build"
        cat > ~/chimera_run.sh <<LAUNCHER
#!/bin/bash
# ChimeraPhoenix Launcher

# Start JACK if not running
if ! pgrep -x "jackd" > /dev/null; then
    echo "Starting JACK audio server..."
    jackd -R -dalsa -dhw:1 -r48000 -p512 -n3 &
    sleep 2
fi

# Launch ChimeraPhoenix
echo "Launching ChimeraPhoenix..."
cd "$BUILD_PATH"
./ChimeraPhoenix
LAUNCHER

        chmod +x ~/chimera_run.sh

        echo ""
        echo -e "${GREEN}🎉 Installation Complete!${NC}"
        echo ""
        echo "To run ChimeraPhoenix:"
        echo "   ~/chimera_run.sh"
        echo ""
        echo "Or manually:"
        echo "   cd $BUILD_PATH"
        echo "   ./ChimeraPhoenix"
        echo ""
    else
        echo -e "${RED}❌ Build failed with status $BUILD_STATUS${NC}"
        echo "Check the error messages above"
        exit 1
    fi
else
    echo -e "${RED}❌ Linux Makefile not generated${NC}"
    echo "The .jucer file may need Linux export target enabled"
    exit 1
fi

echo ""
echo -e "${BLUE}📡 Step 8: Trinity AI Server Setup${NC}"
echo ""
echo "The Trinity AI server needs to run on a machine with OpenAI API access."
echo "You have two options:"
echo ""
echo "Option A - Run on your Mac (recommended):"
echo "   The Pi will connect to http://YOUR_MAC_IP:8000"
echo "   No additional setup needed on Pi"
echo ""
echo "Option B - Run on Pi (requires API key on Pi):"
echo "   cd $SCRIPT_DIR/AI_Server"
echo "   pip3 install -r requirements.txt"
echo "   # Set your OpenAI API key"
echo "   python3 server_trinity_complete.py"
echo ""

echo -e "${GREEN}✅ Setup complete!${NC}"
echo ""
echo "Next steps:"
echo "1. Test audio: ~/chimera_run.sh"
echo "2. Start planning hardware integration (GPIO, display, controls)"
echo ""
