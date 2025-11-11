# Raspberry Pi Build Instructions for Chimera Phoenix

## Prerequisites on Raspberry Pi

1. **Install JUCE Dependencies**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    libasound2-dev \
    libjack-jackd2-dev \
    libfreetype6-dev \
    libx11-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxcomposite-dev \
    mesa-common-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    freeglut3-dev \
    libwebkit2gtk-4.0-dev \
    libgtk-3-dev \
    libcurl4-openssl-dev
```

2. **Install GPIO Libraries (if using GPIO)**
```bash
sudo apt-get install -y wiringpi
# Or for newer systems:
sudo apt-get install -y libgpiod-dev
```

## Deployment Process

### Step 1: Transfer Files to Pi

From your Mac:
```bash
# Create archive of clean codebase
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix
tar czf chimera_phoenix_v3.1.tar.gz \
    JUCE_Plugin/Source/*.cpp \
    JUCE_Plugin/Source/*.h \
    JUCE_Plugin/ChimeraPhoenix.jucer \
    JUCE_Plugin/JuceLibraryCode/ \
    JUCE_Plugin/Resources/

# Transfer to Pi (replace pi_address with your Pi's IP)
scp chimera_phoenix_v3.1.tar.gz pi@pi_address:~/

# Or use rsync for incremental updates
rsync -avz --exclude 'Builds/' --exclude '.git' \
    JUCE_Plugin/ pi@pi_address:~/chimera_phoenix/JUCE_Plugin/
```

### Step 2: On the Raspberry Pi

```bash
# SSH to Pi
ssh pi@pi_address

# Extract files
tar xzf chimera_phoenix_v3.1.tar.gz

# Or if using rsync, cd to project
cd ~/chimera_phoenix
```

### Step 3: Generate Makefile with Projucer

```bash
# Download and build Projucer on Pi (one time)
git clone https://github.com/juce-framework/JUCE.git ~/JUCE
cd ~/JUCE/extras/Projucer/Builds/LinuxMakefile
make CONFIG=Release
sudo cp build/Projucer /usr/local/bin/

# Generate Linux Makefile for your project
cd ~/chimera_phoenix/JUCE_Plugin
Projucer --resave ChimeraPhoenix.jucer
```

### Step 4: Build Without GPIO

```bash
cd ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile

# Clean previous builds
make clean

# Build without GPIO (uses PluginEditor_Original)
make CONFIG=Release -j4

# The binary will be in:
# build/ChimeraPhoenix (standalone)
```

### Step 5: Build With GPIO

```bash
cd ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile

# Clean previous builds
make clean

# Build with GPIO enabled (uses PluginEditor_Pi)
make CONFIG=Release CPPFLAGS="-DENABLE_GPIO=1" -j4

# The binary will be in:
# build/ChimeraPhoenix (standalone with GPIO)
```

## Build Scripts

### build_no_gpio.sh
```bash
#!/bin/bash
echo "Building Chimera Phoenix WITHOUT GPIO support..."
cd ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile
make clean
make CONFIG=Release -j4
echo "Build complete! Binary at: build/ChimeraPhoenix"
```

### build_with_gpio.sh
```bash
#!/bin/bash
echo "Building Chimera Phoenix WITH GPIO support..."
cd ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile
make clean
make CONFIG=Release CPPFLAGS="-DENABLE_GPIO=1" -j4
echo "Build complete! Binary at: build/ChimeraPhoenix"
```

## Testing

### Test Standalone Without GPIO
```bash
cd ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile
./build/ChimeraPhoenix
```

### Test Standalone With GPIO (requires hardware)
```bash
cd ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile
sudo ./build/ChimeraPhoenix  # May need sudo for GPIO access
```

## Troubleshooting

### 1. Missing JUCE Modules
If you get "JUCE modules not found":
```bash
# Set JUCE path in your .bashrc
echo 'export JUCE_PATH=~/JUCE' >> ~/.bashrc
source ~/.bashrc
```

### 2. GPIO Permission Issues
```bash
# Add user to gpio group
sudo usermod -a -G gpio $USER
# Logout and login again
```

### 3. Audio Issues
```bash
# Check audio devices
aplay -l

# Test with JACK
jackd -d alsa -r 48000 -p 512 -n 2 &
./build/ChimeraPhoenix
```

### 4. Build Errors
```bash
# Check compiler version
g++ --version  # Should be 8.0 or higher

# Install missing dependencies
sudo apt-get install -y pkg-config
```

## Optimization for Pi

### Raspberry Pi 4 Optimizations
```bash
# Build with Pi 4 optimizations
make CONFIG=Release \
    CPPFLAGS="-march=armv8-a+crc -mtune=cortex-a72 -O3" \
    -j4
```

### Raspberry Pi 3 Optimizations
```bash
# Build with Pi 3 optimizations
make CONFIG=Release \
    CPPFLAGS="-march=armv7-a -mfpu=neon-vfpv4 -mtune=cortex-a53 -O3" \
    -j4
```

## Automated Deployment Script

Save as `deploy_to_pi.sh` on your Mac:
```bash
#!/bin/bash
PI_HOST="${1:-pi@raspberrypi.local}"
BUILD_GPIO="${2:-no}"

echo "Deploying to $PI_HOST..."

# Sync source files
rsync -avz --delete \
    --exclude 'Builds/' \
    --exclude '.git' \
    --exclude '*.o' \
    --exclude '*.d' \
    --exclude 'build/' \
    JUCE_Plugin/ $PI_HOST:~/chimera_phoenix/JUCE_Plugin/

# Build on Pi
if [ "$BUILD_GPIO" = "gpio" ]; then
    echo "Building WITH GPIO..."
    ssh $PI_HOST "cd ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile && make clean && make CONFIG=Release CPPFLAGS='-DENABLE_GPIO=1' -j4"
else
    echo "Building WITHOUT GPIO..."
    ssh $PI_HOST "cd ~/chimera_phoenix/JUCE_Plugin/Builds/LinuxMakefile && make clean && make CONFIG=Release -j4"
fi

echo "Deployment complete!"
```

Usage:
```bash
# Deploy and build without GPIO
./deploy_to_pi.sh pi@192.168.1.100

# Deploy and build with GPIO
./deploy_to_pi.sh pi@192.168.1.100 gpio
```

## Notes

1. **First Time Setup**: The Projucer needs to be installed on the Pi to generate the Linux Makefile
2. **JUCE Path**: Make sure JUCE is installed at `~/JUCE` or update paths accordingly
3. **GPIO vs Non-GPIO**: The code automatically selects the right UI based on the `ENABLE_GPIO` flag
4. **Safety System**: The safety wrapper is enabled by default for all builds
5. **Cross-Compilation**: While possible, native compilation on Pi is simpler and more reliable

## Verification After Build

Check that the correct UI was built:
```bash
# Check symbols in binary
strings build/ChimeraPhoenix | grep "PluginEditor"

# For non-GPIO build, should see:
# PluginEditor_Original

# For GPIO build, should see:
# PluginEditor_Pi
```

---

*Ready for deployment when your Raspberry Pi is connected!*