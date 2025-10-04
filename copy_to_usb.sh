#!/bin/bash
# Copy deployment package to USB drive

echo "🔍 Looking for USB drives..."
echo ""
echo "Available volumes:"
ls -1 /Volumes/
echo ""
read -p "Enter USB drive name (e.g., 'USB_DRIVE'): " USB_NAME
echo ""
echo "📦 Copying pi_deployment to /Volumes/$USB_NAME/ChimeraPhoenix..."
cp -r pi_deployment "/Volumes/$USB_NAME/ChimeraPhoenix"
echo ""
echo "✅ Copy complete!"
echo ""
echo "Next steps:"
echo "1. Safely eject the USB drive"
echo "2. Plug into Raspberry Pi"
echo "3. On Pi terminal, run:"
echo "   cp -r /media/pi/$USB_NAME/ChimeraPhoenix ~/"
echo "   cd ~/ChimeraPhoenix"
echo "   chmod +x pi_setup.sh"
echo "   ./pi_setup.sh"
