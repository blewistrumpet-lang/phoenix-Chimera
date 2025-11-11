#!/bin/bash
set -euo pipefail

#==============================================================================
# HiFiBerry DAC+ADC Pro Setup Script for Pi 2
# Purpose: Configure Pi 2 with same HiFiBerry config as working Pi 1
# Date: November 2, 2025
#==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

log() { echo -e "${BLUE}[$(date '+%H:%M:%S')]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1" >&2; }
warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
info() { echo -e "${CYAN}[INFO]${NC} $1"; }

#==============================================================================
# BANNER
#==============================================================================

echo
echo "════════════════════════════════════════════════════════════════"
echo "  HiFiBerry DAC+ADC Pro Setup for Pi 2"
echo "════════════════════════════════════════════════════════════════"
echo

#==============================================================================
# PRE-FLIGHT CHECKS
#==============================================================================

log "Running pre-flight checks..."

# Check if running on Raspberry Pi
if [ ! -f /proc/device-tree/model ]; then
    error "This script must be run on a Raspberry Pi"
    exit 1
fi

PI_MODEL=$(tr -d '\0' < /proc/device-tree/model)
info "Detected: $PI_MODEL"

# Check if running as regular user (not root)
if [ "$EUID" -eq 0 ]; then
    error "Do NOT run this script as root. Run as regular user (it will sudo when needed)"
    exit 1
fi

# Check if /boot/firmware exists (modern Raspberry Pi OS)
if [ ! -d /boot/firmware ]; then
    error "/boot/firmware not found. Are you on modern Raspberry Pi OS?"
    error "Older systems use /boot/config.txt instead"
    exit 1
fi

CONFIG_FILE="/boot/firmware/config.txt"
success "Configuration file: $CONFIG_FILE"

#==============================================================================
# BACKUP EXISTING CONFIG
#==============================================================================

log "Creating backup of existing config..."

BACKUP_FILE="$HOME/config.txt.backup.$(date +%Y%m%d_%H%M%S)"
sudo cp "$CONFIG_FILE" "$BACKUP_FILE"
success "Backup created: $BACKUP_FILE"

#==============================================================================
# CHECK CURRENT CONFIGURATION
#==============================================================================

log "Checking current audio configuration..."

# Check for conflicting audio settings
ONBOARD_AUDIO=$(grep "^dtparam=audio=" "$CONFIG_FILE" || echo "not_found")
HIFIBERRY_OVERLAY=$(grep "^dtoverlay=hifiberry" "$CONFIG_FILE" || echo "not_found")
VC4_OVERLAY=$(grep "^dtoverlay=vc4-kms-v3d" "$CONFIG_FILE" || echo "not_found")

info "Current onboard audio: $ONBOARD_AUDIO"
info "Current HiFiBerry overlay: $HIFIBERRY_OVERLAY"
info "Current VC4 overlay: $VC4_OVERLAY"

#==============================================================================
# MODIFY BOOT CONFIG
#==============================================================================

log "Configuring /boot/firmware/config.txt for HiFiBerry..."

# Create temporary file for modifications
TMP_CONFIG=$(mktemp)
cp "$CONFIG_FILE" "$TMP_CONFIG"

# 1. Disable onboard audio (if enabled)
if grep -q "^dtparam=audio=on" "$TMP_CONFIG"; then
    warning "Disabling onboard audio..."
    sed -i 's/^dtparam=audio=on/dtparam=audio=off/' "$TMP_CONFIG"
elif grep -q "^#dtparam=audio=on" "$TMP_CONFIG"; then
    info "Onboard audio already commented out"
elif ! grep -q "dtparam=audio=" "$TMP_CONFIG"; then
    info "Adding dtparam=audio=off"
    echo "dtparam=audio=off" >> "$TMP_CONFIG"
else
    info "Onboard audio already disabled"
fi

# 2. Disable HDMI audio in vc4 overlay
if grep -q "^dtoverlay=vc4-kms-v3d$" "$TMP_CONFIG"; then
    warning "Disabling HDMI audio in vc4 overlay..."
    sed -i 's/^dtoverlay=vc4-kms-v3d$/dtoverlay=vc4-kms-v3d,noaudio/' "$TMP_CONFIG"
elif grep -q "^dtoverlay=vc4-kms-v3d,noaudio" "$TMP_CONFIG"; then
    info "VC4 noaudio already configured"
else
    info "VC4 overlay not found or already configured"
fi

# 3. Add HiFiBerry overlay (if not present)
if grep -q "^dtoverlay=hifiberry-dacplusadcpro" "$TMP_CONFIG"; then
    info "HiFiBerry overlay already present"
else
    warning "Adding HiFiBerry DAC+ADC Pro overlay..."
    echo "" >> "$TMP_CONFIG"
    echo "# HiFiBerry DAC+ADC Pro" >> "$TMP_CONFIG"
    echo "dtoverlay=hifiberry-dacplusadcpro" >> "$TMP_CONFIG"
fi

# 4. Remove any conflicting overlays
if grep -q "^dtoverlay=hifiberry-dac$" "$TMP_CONFIG"; then
    warning "Removing conflicting hifiberry-dac overlay..."
    sed -i '/^dtoverlay=hifiberry-dac$/d' "$TMP_CONFIG"
fi

# Write modified config
sudo cp "$TMP_CONFIG" "$CONFIG_FILE"
rm "$TMP_CONFIG"

success "Boot configuration updated"

#==============================================================================
# SHOW CHANGES
#==============================================================================

log "Current HiFiBerry-related settings in config.txt:"
echo
grep -E "dtparam=audio|dtoverlay=vc4|dtoverlay=hifiberry" "$CONFIG_FILE" | sed 's/^/  /'
echo

#==============================================================================
# INSTALL JACK AUDIO
#==============================================================================

log "Checking JACK audio installation..."

if ! which jackd &>/dev/null; then
    warning "JACK not found. Installing jackd2..."
    sudo apt update
    sudo apt install -y jackd2 libjack-jackd2-dev
    success "JACK installed"
else
    success "JACK already installed: $(which jackd)"
fi

# Add user to audio group (if not already)
if ! groups | grep -q audio; then
    warning "Adding $USER to audio group..."
    sudo usermod -a -G audio "$USER"
    success "Added to audio group (will take effect after logout)"
else
    info "Already in audio group"
fi

#==============================================================================
# CREATE JACK CONFIGURATION
#==============================================================================

log "Creating JACK configuration..."

JACKDRC="$HOME/.jackdrc"

if [ -f "$JACKDRC" ]; then
    warning "Backing up existing .jackdrc to .jackdrc.backup"
    cp "$JACKDRC" "$JACKDRC.backup"
fi

cat > "$JACKDRC" << 'EOF'
/usr/bin/jackd -R -dalsa -dhw:sndrpihifiberry -r48000 -p512 -n3 -i2 -o2
EOF

success "JACK configuration created: $JACKDRC"

#==============================================================================
# INSTALL LIBGPIOD (for GPIO hardware)
#==============================================================================

log "Checking libgpiod installation (for GPIO encoders/switches)..."

if ! dpkg -l | grep -q libgpiod-dev; then
    warning "Installing libgpiod-dev..."
    sudo apt install -y libgpiod-dev gpiod
    success "libgpiod installed"
else
    success "libgpiod already installed"
fi

#==============================================================================
# SYSTEM INFO
#==============================================================================

log "Gathering system information..."

echo
info "Raspberry Pi Model: $PI_MODEL"
info "OS Version: $(lsb_release -d | cut -f2)"
info "Kernel: $(uname -r)"
info "User: $USER"
info "Groups: $(groups)"
echo

#==============================================================================
# REBOOT PROMPT
#==============================================================================

echo
echo "════════════════════════════════════════════════════════════════"
success "✅ HiFiBerry Configuration Complete!"
echo "════════════════════════════════════════════════════════════════"
echo
warning "⚠️  REBOOT REQUIRED for changes to take effect"
echo
echo "After reboot, verify with:"
echo "  1. aplay -l | grep sndrpihifiberry"
echo "  2. cat ~/.jackdrc"
echo "  3. Test JACK: jackd -R -dalsa -dhw:sndrpihifiberry -r48000 -p512 -n3 -i2 -o2"
echo
echo "Configuration details:"
echo "  • Onboard audio: DISABLED"
echo "  • HDMI audio: DISABLED"
echo "  • HiFiBerry DAC+ADC Pro: ENABLED"
echo "  • JACK configured for 48kHz, 512 buffer, stereo I/O"
echo "  • GPIO library installed (for encoders/switches)"
echo
echo "Backup location: $BACKUP_FILE"
echo
echo "════════════════════════════════════════════════════════════════"
echo

read -p "Reboot now? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    log "Rebooting in 3 seconds..."
    sleep 3
    sudo reboot
else
    warning "Remember to reboot before using HiFiBerry!"
    echo
    echo "To reboot later: sudo reboot"
    echo
fi

exit 0
