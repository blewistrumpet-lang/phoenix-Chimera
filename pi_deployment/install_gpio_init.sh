#!/bin/bash
#==============================================================================
# Install GPIO Pull-Up Initialization Service
# Run this on Pi 2 to enable automatic GPIO configuration on boot
#==============================================================================

set -euo pipefail

echo "Installing Chimera GPIO initialization service..."

# Copy init script to /usr/local/bin
sudo cp chimera_gpio_init.sh /usr/local/bin/
sudo chmod +x /usr/local/bin/chimera_gpio_init.sh
echo "✓ Init script installed to /usr/local/bin/chimera_gpio_init.sh"

# Copy systemd service file
sudo cp chimera-gpio-init.service /etc/systemd/system/
echo "✓ Service file installed to /etc/systemd/system/chimera-gpio-init.service"

# Reload systemd and enable service
sudo systemctl daemon-reload
sudo systemctl enable chimera-gpio-init.service
echo "✓ Service enabled"

# Start service now
sudo systemctl start chimera-gpio-init.service
echo "✓ Service started"

# Verify status
sudo systemctl status chimera-gpio-init.service --no-pager

echo ""
echo "════════════════════════════════════════════════════════════════"
echo "✅ GPIO initialization service installed successfully!"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "GPIO pull-ups will now be configured automatically on every boot."
echo ""
echo "To check status: sudo systemctl status chimera-gpio-init.service"
echo "To disable: sudo systemctl disable chimera-gpio-init.service"
echo ""

exit 0
