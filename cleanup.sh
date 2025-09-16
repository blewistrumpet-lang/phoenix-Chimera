#!/bin/bash

# Chimera Phoenix Repository Cleanup Script
# September 2025

echo "=== Chimera Phoenix Repository Cleanup ==="
echo ""

# Function to show size before cleanup
show_size() {
    echo "Repository size before cleanup:"
    du -sh . | cut -f1
    echo ""
}

# Function to remove files with confirmation
remove_with_count() {
    local pattern="$1"
    local description="$2"
    local count=$(ls $pattern 2>/dev/null | wc -l | tr -d ' ')
    
    if [ "$count" -gt 0 ]; then
        echo "Found $count $description"
        rm -f $pattern
        echo "  ✓ Removed"
    else
        echo "No $description found"
    fi
}

show_size

echo "=== Phase 1: Remove Object Files ==="
remove_with_count "*.o" "object files in root"
remove_with_count "JUCE_Plugin/*.o" "object files in JUCE_Plugin"
remove_with_count "JUCE_Plugin/Tests/*.o" "object files in Tests"
echo ""

echo "=== Phase 2: Remove Test Executables ==="
# Remove all test executables (no extension on macOS)
for file in test_*; do
    if [ -f "$file" ] && [ -x "$file" ] && [ ! "${file##*.}" = "cpp" ] && [ ! "${file##*.}" = "h" ] && [ ! "${file##*.}" = "sh" ]; then
        rm -f "$file"
    fi
done
echo "  ✓ Removed test executables"
echo ""

echo "=== Phase 3: Remove Backup Files ==="
remove_with_count "JUCE_Plugin/Source/*_BACKUP.cpp" "backup cpp files"
remove_with_count "JUCE_Plugin/Source/*_backup.cpp" "backup cpp files"
remove_with_count "JUCE_Plugin/Source/*_old.cpp" "old cpp files"
remove_with_count "JUCE_Plugin/Source/*_OLD.cpp" "old cpp files"
remove_with_count "JUCE_Plugin/Source/*_Original.cpp" "original cpp files"
remove_with_count "JUCE_Plugin/Source/*_Original.h" "original header files"
remove_with_count "JUCE_Plugin/Source/*_broken.cpp" "broken implementation files"
echo ""

echo "=== Phase 4: Remove Duplicate Engine Implementations ==="
# BitCrusher variants
remove_with_count "JUCE_Plugin/Source/BitCrusher_Basic.*" "BitCrusher Basic variant"
remove_with_count "JUCE_Plugin/Source/BitCrusher_Simple.*" "BitCrusher Simple variant"
remove_with_count "JUCE_Plugin/Source/BitCrusher_Original.*" "BitCrusher Original variant"
remove_with_count "JUCE_Plugin/Source/BitCrusher_Studio.*" "BitCrusher Studio variant"

# IntelligentHarmonizer variants
remove_with_count "JUCE_Plugin/Source/IntelligentHarmonizer_*PSOLA*.cpp" "Harmonizer PSOLA variants"
remove_with_count "JUCE_Plugin/Source/IntelligentHarmonizer_WORKING*.cpp" "Harmonizer working variants"
remove_with_count "JUCE_Plugin/Source/IntelligentHarmonizer_FIXED.cpp" "Harmonizer fixed variant"
remove_with_count "JUCE_Plugin/Source/IntelligentHarmonizer_broken*.cpp" "Harmonizer broken variants"

# Reverb backups
echo "Removing reverb backup directory..."
rm -rf JUCE_Plugin/Source/REVERB_BACKUP_BEFORE_REBUILD/
echo "  ✓ Removed"
echo ""

echo "=== Phase 5: Remove Old Editor Implementations ==="
remove_with_count "JUCE_Plugin/Source/PluginEditor_Original.*" "original editor files"
remove_with_count "JUCE_Plugin/Source/PluginEditorBasic.*" "basic editor files"
remove_with_count "JUCE_Plugin/Source/PluginEditorComplete.*" "complete editor files"
remove_with_count "JUCE_Plugin/Source/PluginEditorWorking.*" "working editor files"
remove_with_count "JUCE_Plugin/Source/PluginEditorTestBypass.*" "test bypass editor"
remove_with_count "JUCE_Plugin/Source/PluginEditorNexusDynamic*.*" "dynamic nexus variants"
remove_with_count "JUCE_Plugin/Source/PluginEditorSimpleFinal.*" "simple final editor"
echo ""

echo "=== Phase 6: Remove Test Report Files ==="
remove_with_count "*_TEST_RESULTS.md" "test result markdown files"
remove_with_count "*_REPORT.md" "report markdown files (keeping STATUS_REPORT_2025.md)"
remove_with_count "*_Analysis.txt" "analysis text files"
remove_with_count "Reports/*.txt" "report text files"
echo ""

echo "=== Phase 7: Remove Temporary and Generated Files ==="
remove_with_count "*.pyc" "Python cache files"
remove_with_count "__pycache__" "Python cache directories"
remove_with_count ".DS_Store" "macOS metadata files"
remove_with_count "*~" "backup files"
remove_with_count "*.orig" "merge conflict files"
echo ""

echo "=== Phase 8: Remove Makefiles for Tests ==="
remove_with_count "Makefile.*test*" "test makefiles"
remove_with_count "JUCE_Plugin/Makefile.*test*" "JUCE test makefiles"
echo ""

echo "=== Phase 9: Clean Build Directories ==="
if [ -d "JUCE_Plugin/Builds/MacOSX/build" ]; then
    echo "Cleaning Xcode build directory..."
    rm -rf JUCE_Plugin/Builds/MacOSX/build
    echo "  ✓ Cleaned"
fi
echo ""

echo "=== Final Statistics ==="
echo "Repository size after cleanup:"
du -sh . | cut -f1
echo ""

# Count remaining important files
echo "Remaining important files:"
echo "  Source files (.cpp): $(find JUCE_Plugin/Source -name "*.cpp" -not -path "*/OLD_IMPLEMENTATIONS/*" | wc -l | tr -d ' ')"
echo "  Header files (.h): $(find JUCE_Plugin/Source -name "*.h" -not -path "*/OLD_IMPLEMENTATIONS/*" | wc -l | tr -d ' ')"
echo "  Python files (.py): $(find . -name "*.py" | wc -l | tr -d ' ')"
echo ""

echo "=== Cleanup Complete! ==="
echo ""
echo "Consider adding these to .gitignore to prevent future clutter:"
echo "  *.o"
echo "  test_*"
echo "  !test_*.cpp"
echo "  !test_*.h"
echo "  *_backup.*"
echo "  *_BACKUP.*"
echo "  *_old.*"
echo "  *_OLD.*"
echo "  *_Original.*"
echo "  *_broken.*"
echo "  .DS_Store"
echo "  __pycache__/"
echo "  *.pyc"
echo "  build/"