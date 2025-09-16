#!/bin/bash

# Deep cleanup script for Chimera Phoenix
# Removes old implementations and consolidates tests

echo "=== Deep Repository Cleanup ==="
echo ""

# Move important test files to a Tests directory
echo "Creating organized Tests directory..."
mkdir -p Tests/UI
mkdir -p Tests/Engines
mkdir -p Tests/Trinity
mkdir -p Tests/Parameters

# Move recent/important test files
echo "Organizing test files..."
[ -f test_trinity_engine_fix.cpp ] && mv test_trinity_engine_fix.cpp Tests/Trinity/
[ -f test_parameter_normalization.cpp ] && mv test_parameter_normalization.cpp Tests/Parameters/
[ -f test_all_engines_comprehensive.cpp ] && mv test_all_engines_comprehensive.cpp Tests/Engines/
[ -f test_ui_visual.cpp ] && mv test_ui_visual.cpp Tests/UI/
[ -f test_ui_summary.cpp ] && mv test_ui_summary.cpp Tests/UI/
[ -f test_engine_comparison.cpp ] && mv test_engine_comparison.cpp Tests/Engines/

# Remove all remaining test cpp files in root
echo "Removing old test files from root..."
rm -f test_*.cpp
echo "  ✓ Removed old test source files"

# Remove OLD implementations
echo "Removing OLD_IMPLEMENTATIONS directory..."
rm -rf JUCE_Plugin/Source/OLD_IMPLEMENTATIONS/
rm -f JUCE_Plugin/Source/*_OLD.*
echo "  ✓ Removed"

# Remove duplicate/variant implementations
echo "Removing duplicate engine implementations..."
rm -f JUCE_Plugin/Source/ConvolutionReverb_Algorithmic.cpp
rm -f JUCE_Plugin/Source/ConvolutionReverb_Fixed.cpp
rm -f JUCE_Plugin/Source/ConvolutionReverb_WAV.cpp
rm -f JUCE_Plugin/Source/PlateReverb_Freeverb.cpp
rm -f JUCE_Plugin/Source/PlateReverb_old.cpp
rm -f JUCE_Plugin/Source/SpringReverb_old.cpp
rm -f JUCE_Plugin/Source/SpringReverb_Proven.cpp
rm -f JUCE_Plugin/Source/GatedReverb_old.cpp
rm -f JUCE_Plugin/Source/GatedReverb_Proven.cpp
rm -f JUCE_Plugin/Source/ShimmerReverb_old.cpp
rm -f JUCE_Plugin/Source/ShimmerReverb_Proven.cpp
rm -f JUCE_Plugin/Source/ChaosGenerator_Complex.cpp
rm -f JUCE_Plugin/Source/BucketBrigadeDelay_Original.cpp
rm -f JUCE_Plugin/Source/BucketBrigadeDelay_Fixed.cpp
echo "  ✓ Removed duplicate implementations"

# Remove test/experimental UI editors
echo "Removing experimental UI editors..."
rm -f JUCE_Plugin/Source/PluginEditorRefined.*
rm -f JUCE_Plugin/Source/PluginEditorStaticWithDynamic.*
rm -f JUCE_Plugin/Source/PluginEditorWithAllAttachments.*
rm -f JUCE_Plugin/Source/PluginEditorWithOneAttachment.*
rm -f JUCE_Plugin/Source/TestEditorIncremental.*
rm -f JUCE_Plugin/Source/PluginEditorNexus_Final.*
echo "  ✓ Removed experimental editors"

# Remove unused pitch shift strategies
echo "Removing unused pitch shift implementations..."
rm -f JUCE_Plugin/Source/SMBPitchShift*.h
rm -f JUCE_Plugin/Source/PsolaEngine*.h
rm -f JUCE_Plugin/Source/PhaseVocoder*.h
rm -f JUCE_Plugin/Source/PhaseVocoder*.cpp
echo "  ✓ Removed unused pitch implementations"

# Remove test executables from JUCE_Plugin/Source
echo "Removing test executables from Source..."
rm -f JUCE_Plugin/Source/*_test
rm -f JUCE_Plugin/Source/*_test_*
echo "  ✓ Removed"

# Remove various test implementations
echo "Removing test implementations..."
rm -f JUCE_Plugin/Source/*Test.cpp
rm -f JUCE_Plugin/Source/Comprehensive*Test.cpp
rm -f JUCE_Plugin/Source/Simple*Test.cpp
rm -f JUCE_Plugin/Source/Minimal*Test.cpp
echo "  ✓ Removed"

# Clean up root directory reports and analysis files
echo "Cleaning up report files..."
rm -f *_IMPROVEMENTS.md
rm -f *_ANALYSIS.md
rm -f *_GUIDE.md
rm -f *_PLAN.md
rm -f *_ASSESSMENT.md
rm -f *_DELIVERABLE.md
rm -f *_SUMMARY.md
rm -f DSP_*_RESEARCH_REPORT.md
rm -f PROGRESS_*.md
# Keep the main status report
echo "  ✓ Removed analysis and report files (keeping STATUS_REPORT_2025.md)"

# Remove empty directories
echo "Removing empty directories..."
find . -type d -empty -delete 2>/dev/null
echo "  ✓ Removed empty directories"

# Final stats
echo ""
echo "=== Final Repository Stats ==="
echo "Size: $(du -sh . | cut -f1)"
echo ""
echo "Core source files:"
echo "  Engines: $(ls JUCE_Plugin/Source/*Engine.cpp 2>/dev/null | wc -l | tr -d ' ') implementations"
echo "  UI Components: $(ls JUCE_Plugin/Source/*Component*.cpp 2>/dev/null | wc -l | tr -d ' ') files"
echo "  Main Plugin: $(ls JUCE_Plugin/Source/Plugin*.cpp 2>/dev/null | wc -l | tr -d ' ') files"
echo ""
echo "Test files organized in Tests/:"
ls -la Tests/ 2>/dev/null
echo ""
echo "=== Deep cleanup complete! ==="