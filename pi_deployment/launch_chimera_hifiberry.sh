#!/bin/bash
set -euo pipefail

#==============================================================================
# Chimera Phoenix Plugin - HiFiBerry DAC+ADC Production Launch Script
# Created: 2025-10-13
# Hardware: HiFiBerry DAC+ADC Pro (hw:0) + USB Mic (hw:1)
#==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

LOCK_FILE="/tmp/chimera_plugin.lock"

log() { echo -e "${BLUE}[$(date '+%H:%M:%S')]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1" >&2; }
warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }

#==============================================================================
# PRE-FLIGHT CHECKS
#==============================================================================

log "Starting Chimera Phoenix with HiFiBerry DAC+ADC Pro..."

# Check if already running
if [ -f "$LOCK_FILE" ]; then
    error "Plugin already running. To force restart: rm $LOCK_FILE && $0"
    exit 1
fi

trap "rm -f $LOCK_FILE" EXIT
touch $LOCK_FILE

# Verify HiFiBerry is detected
if ! aplay -l 2>/dev/null | grep -q "sndrpihifiberry"; then
    error "HiFiBerry DAC+ADC Pro not found!"
    aplay -l
    exit 1
fi
success "HiFiBerry DAC+ADC Pro detected (hw:0)"

# Verify USB mic is detected
if ! arecord -l 2>/dev/null | grep -q "USB PnP Sound Device"; then
    warning "USB microphone not detected - voice recording will not work"
else
    success "USB microphone detected (hw:1)"
fi

# Verify JACK is installed
if ! which jackd &>/dev/null; then
    error "jackd not found. Install: sudo apt-get install jackd2"
    exit 1
fi
success "JACK installed"

# Check plugin binary
PLUGIN_BINARY="$HOME/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix"
if [ ! -x "$PLUGIN_BINARY" ]; then
    error "Plugin binary not found: $PLUGIN_BINARY"
    exit 1
fi
success "Plugin binary verified"

# Verify API key
ENV_FILE="$HOME/phoenix-Chimera/AI_Server/.env"
if [ ! -f "$ENV_FILE" ]; then
    error "Missing .env file: $ENV_FILE"
    exit 1
fi
if ! grep -q "^OPENAI_API_KEY=sk-" "$ENV_FILE"; then
    error "Invalid OPENAI_API_KEY in $ENV_FILE"
    exit 1
fi
success "API key configured"

#==============================================================================
# CLEANUP CONFLICTING PROCESSES
#==============================================================================

log "Cleaning up audio processes..."

# Kill PulseAudio and PipeWire (they conflict with JACK)
for proc in pulseaudio pipewire pipewire-pulse; do
    if pgrep -x "$proc" &>/dev/null; then
        warning "Killing $proc (conflicts with JACK)..."
        pkill -9 "$proc" 2>/dev/null || true
    fi
done

# Stop existing JACK
if pgrep jackd &>/dev/null; then
    log "Stopping existing JACK server..."
    pkill -TERM jackd
    sleep 2
    pkill -9 jackd 2>/dev/null || true
fi

# Stop existing Trinity server
if pgrep -f trinity_server_pi &>/dev/null; then
    log "Stopping existing Trinity server..."
    pkill -TERM -f trinity_server_pi
    sleep 2
    pkill -9 -f trinity_server_pi 2>/dev/null || true
fi

# Stop existing plugin
if pgrep -f ChimeraPhoenix &>/dev/null; then
    log "Stopping existing plugin..."
    pkill -TERM -f ChimeraPhoenix
    sleep 2
    pkill -9 -f ChimeraPhoenix 2>/dev/null || true
fi

sleep 1
success "Cleanup completed"

#==============================================================================
# START JACK SERVER WITH HIFIBERRY
#==============================================================================

log "Starting JACK server with HiFiBerry..."

mkdir -p "$HOME/phoenix-Chimera/logs"
JACK_LOG="$HOME/phoenix-Chimera/logs/jack.log"

# Use HiFiBerry configuration from ~/.jackdrc
# -R: realtime mode
# -d alsa: ALSA driver
# -d hw:sndrpihifiberry: HiFiBerry device
# -r 48000: 48kHz sample rate
# -p 512: buffer size
# -n 3: number of periods
# -i 2 -o 2: 2 input, 2 output channels
jackd -R -d alsa -d hw:sndrpihifiberry -r 48000 -p 512 -n 3 -i 2 -o 2 > "$JACK_LOG" 2>&1 &
JACK_PID=$!

log "JACK server started (PID: $JACK_PID)"

# Wait for JACK to be ready
log "Waiting for JACK..."
JACK_READY=0
for i in {1..20}; do
    if jack_lsp &>/dev/null 2>&1; then
        JACK_READY=1
        break
    fi
    sleep 0.5
done

if [ $JACK_READY -eq 0 ]; then
    error "JACK failed to start. Check: $JACK_LOG"
    tail -20 "$JACK_LOG"
    exit 1
fi

success "JACK server ready with HiFiBerry"

#==============================================================================
# START TRINITY AI SERVER
#==============================================================================

log "Starting Trinity AI server..."

cd "$HOME/phoenix-Chimera/AI_Server"

# Export environment variables
export $(grep -v '^#' .env | xargs)

TRINITY_LOG="$HOME/phoenix-Chimera/logs/trinity.log"
nohup python3 trinity_server_pi.py > "$TRINITY_LOG" 2>&1 &
TRINITY_PID=$!

log "Trinity server started (PID: $TRINITY_PID)"

# Wait for Trinity to be ready
log "Waiting for Trinity..."
TRINITY_READY=0
for i in {1..30}; do
    if curl -s http://localhost:8000/health &>/dev/null; then
        TRINITY_READY=1
        break
    fi
    sleep 1
done

if [ $TRINITY_READY -eq 0 ]; then
    error "Trinity failed to start. Check: $TRINITY_LOG"
    tail -20 "$TRINITY_LOG"
    pkill -9 jackd 2>/dev/null || true
    exit 1
fi

# Verify health
HEALTH=$(curl -s http://localhost:8000/health | grep -o '"status":"[^"]*"' | cut -d'"' -f4)
if [ "$HEALTH" != "healthy" ]; then
    error "Trinity health check failed: $HEALTH"
    tail -20 "$TRINITY_LOG"
    exit 1
fi

success "Trinity server ready and healthy"

#==============================================================================
# START PLUGIN
#==============================================================================

log "Starting Chimera Phoenix plugin..."

PLUGIN_LOG="$HOME/phoenix-Chimera/logs/plugin.log"
DISPLAY=:0 "$PLUGIN_BINARY" > "$PLUGIN_LOG" 2>&1 &
PLUGIN_PID=$!

log "Plugin started (PID: $PLUGIN_PID)"

# Wait for plugin to initialize
sleep 3

# Verify plugin is still running
if ! ps -p $PLUGIN_PID &>/dev/null; then
    error "Plugin crashed on startup. Check: $PLUGIN_LOG"
    tail -20 "$PLUGIN_LOG"
    pkill -9 jackd trinity_server_pi 2>/dev/null || true
    exit 1
fi

success "Plugin is running"

#==============================================================================
# FINAL VERIFICATION
#==============================================================================

log "Verifying all services..."

ALL_OK=1
if ! pgrep jackd &>/dev/null; then
    error "JACK is not running"
    ALL_OK=0
fi

if ! pgrep -f trinity_server_pi &>/dev/null; then
    error "Trinity is not running"
    ALL_OK=0
fi

if ! pgrep -f ChimeraPhoenix &>/dev/null; then
    error "Plugin is not running"
    ALL_OK=0
fi

if [ $ALL_OK -eq 0 ]; then
    error "System verification failed"
    exit 1
fi

#==============================================================================
# SUMMARY
#==============================================================================

echo
echo "════════════════════════════════════════════════════════════════"
success "🎛️  Chimera Phoenix Running with HiFiBerry DAC+ADC Pro"
echo "════════════════════════════════════════════════════════════════"
echo
echo "✓ JACK Server:     PID $JACK_PID (HiFiBerry hw:0 @ 48kHz)"
echo "✓ Trinity Server:  PID $TRINITY_PID (http://localhost:8000)"
echo "✓ Plugin:          PID $PLUGIN_PID (JUCE Standalone)"
echo
echo "Audio Routing:"
echo "  • Main I/O:      HiFiBerry DAC+ADC Pro (hw:0) → JACK"
echo "  • Voice Input:   USB Microphone (hw:1) → Whisper (direct ALSA)"
echo
echo "Logs:"
echo "  • JACK:          $JACK_LOG"
echo "  • Trinity:       $TRINITY_LOG"
echo "  • Plugin:        $PLUGIN_LOG"
echo
echo "Commands:"
echo "  • Status:        ps aux | grep -E 'jackd|trinity|ChimeraPhoenix'"
echo "  • Stop All:      pkill -f 'jackd|trinity_server_pi|ChimeraPhoenix'"
echo "  • View Logs:     tail -f ~/phoenix-Chimera/logs/*.log"
echo
echo "════════════════════════════════════════════════════════════════"
echo

exit 0
