#!/bin/bash
################################################################################
# ChimeraPhoenix Pi - Complete Launch Script
# Launches all necessary systems for full plugin functionality
################################################################################

set -e  # Exit on error

echo "========================================================================"
echo "ChimeraPhoenix Pi - System Launch"
echo "========================================================================"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Directories
PLUGIN_DIR="$HOME/phoenix-Chimera/pi_deployment"
AI_SERVER_DIR="$HOME/phoenix-Chimera/AI_Server"
PLUGIN_BINARY="$PLUGIN_DIR/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi"

################################################################################
# Step 1: Check if JACK is running
################################################################################
echo ""
echo -e "${BLUE}[1/5] Checking JACK Audio Server...${NC}"

if pgrep -x "jackd" > /dev/null; then
    echo -e "${GREEN}✓ JACK is already running${NC}"
else
    echo -e "${YELLOW}⚠ JACK is not running${NC}"
    echo "Please start JACK manually with your preferred settings, e.g.:"
    echo "  jackd -d alsa -r 44100 -p 256 -n 2"
    echo ""
    read -p "Start JACK automatically? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Starting JACK with default settings..."
        jackd -d alsa -r 44100 -p 256 -n 2 &
        sleep 3
        echo -e "${GREEN}✓ JACK started${NC}"
    else
        echo -e "${RED}✗ Cannot continue without JACK${NC}"
        exit 1
    fi
fi

################################################################################
# Step 2: Check Trinity AI Server
################################################################################
echo ""
echo -e "${BLUE}[2/5] Checking Trinity AI Server...${NC}"

if pgrep -f "python3 main.py" > /dev/null; then
    echo -e "${GREEN}✓ Trinity AI Server is already running${NC}"

    # Verify it's responding
    if curl -s http://localhost:8000/health > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Server is responding on port 8000${NC}"
    else
        echo -e "${YELLOW}⚠ Server is running but not responding${NC}"
        echo "Restarting Trinity AI Server..."
        pkill -f "python3 main.py"
        sleep 2
        cd "$AI_SERVER_DIR"
        nohup python3 main.py > /tmp/trinity_server.log 2>&1 < /dev/null &
        sleep 3
        echo -e "${GREEN}✓ Trinity AI Server restarted${NC}"
    fi
else
    echo -e "${YELLOW}⚠ Trinity AI Server is not running${NC}"
    echo "Starting Trinity AI Server..."
    cd "$AI_SERVER_DIR"
    nohup python3 main.py > /tmp/trinity_server.log 2>&1 < /dev/null &
    sleep 3

    # Verify startup
    if curl -s http://localhost:8000/health > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Trinity AI Server started successfully${NC}"
    else
        echo -e "${RED}✗ Failed to start Trinity AI Server${NC}"
        echo "Check logs: tail -f /tmp/trinity_server.log"
        exit 1
    fi
fi

################################################################################
# Step 3: Check OpenAI API Key
################################################################################
echo ""
echo -e "${BLUE}[3/5] Checking OpenAI Configuration...${NC}"

if [ -f "$AI_SERVER_DIR/.env" ]; then
    if grep -q "OPENAI_API_KEY" "$AI_SERVER_DIR/.env"; then
        echo -e "${GREEN}✓ OpenAI API key configured${NC}"
    else
        echo -e "${YELLOW}⚠ OpenAI API key not found in .env${NC}"
        echo "Voice-to-preset will work in rule-based mode only"
    fi
else
    echo -e "${YELLOW}⚠ .env file not found${NC}"
    echo "Voice-to-preset will work in rule-based mode only"
fi

################################################################################
# Step 4: Check Plugin Binary
################################################################################
echo ""
echo -e "${BLUE}[4/5] Checking Plugin Binary...${NC}"

if [ ! -f "$PLUGIN_BINARY" ]; then
    echo -e "${RED}✗ Plugin binary not found: $PLUGIN_BINARY${NC}"
    echo "Building plugin..."
    cd "$PLUGIN_DIR/JUCE_Plugin/Builds/LinuxMakefile"
    make CONFIG=Debug -j4

    if [ -f "$PLUGIN_BINARY" ]; then
        echo -e "${GREEN}✓ Plugin built successfully${NC}"
    else
        echo -e "${RED}✗ Plugin build failed${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}✓ Plugin binary found${NC}"
fi

# Check if plugin is already running
if pgrep -f "ChimeraPhoenix_Pi" > /dev/null; then
    echo -e "${YELLOW}⚠ Plugin is already running${NC}"
    read -p "Kill existing instance and restart? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        pkill -f "ChimeraPhoenix_Pi"
        sleep 1
        echo -e "${GREEN}✓ Existing instance killed${NC}"
    else
        echo "Using existing instance"
        exit 0
    fi
fi

################################################################################
# Step 5: Launch Plugin
################################################################################
echo ""
echo -e "${BLUE}[5/5] Launching ChimeraPhoenix Plugin...${NC}"

# Check for X display
if [ -z "$DISPLAY" ]; then
    echo -e "${YELLOW}⚠ DISPLAY not set, setting to :0${NC}"
    export DISPLAY=:0
fi

echo ""
echo "========================================================================"
echo -e "${GREEN}System Ready - Launching ChimeraPhoenix...${NC}"
echo "========================================================================"
echo ""
echo "System Status:"
echo "  ✓ JACK Audio Server: Running"
echo "  ✓ Trinity AI Server: http://localhost:8000"
echo "  ✓ OpenAI Integration: $([ -f "$AI_SERVER_DIR/.env" ] && echo "Configured" || echo "Rule-based only")"
echo "  ✓ Plugin Binary: Ready"
echo ""
echo "Features Available:"
echo "  • 57 Audio Processing Engines"
echo "  • Voice-to-Preset AI Generation"
echo "  • 5-Slot Serial Processing Chain"
echo "  • Real-time Parameter Control"
echo ""
echo "Press Ctrl+C to exit"
echo "========================================================================"
echo ""

# Setup cleanup trap to kill Trinity server when plugin exits
cleanup() {
    echo ""
    echo -e "${YELLOW}Cleaning up...${NC}"
    if pgrep -f "python3 main.py" > /dev/null; then
        echo -e "${BLUE}Stopping Trinity AI Server...${NC}"
        pkill -f "python3 main.py"
        echo -e "${GREEN}✓ Trinity AI Server stopped${NC}"
    fi
    echo "Plugin closed"
}

trap cleanup EXIT INT TERM

# Store Trinity server PID for later cleanup
TRINITY_PID=$(pgrep -f "python3 main.py")

# Launch plugin
cd "$PLUGIN_DIR"
"$PLUGIN_BINARY"

# Cleanup will be called automatically by trap
