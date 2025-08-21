#!/bin/bash
echo "======================================"
echo "Chimera Phoenix 3.0 - Legacy Cleanup"
echo "======================================"
echo ""

BACKUP_DIR="legacy_backup_$(date +%Y%m%d_%H%M%S)"
echo "Creating backup directory: $BACKUP_DIR"
mkdir -p "$BACKUP_DIR"

safe_remove() {
    local file=$1
    if [ -f "$file" ]; then
        echo "  Backing up and removing: $file"
        cp "$file" "$BACKUP_DIR/" 2>/dev/null
        rm "$file"
    else
        echo "  File not found (already removed?): $file"
    fi
}

echo ""
echo "Step 1: Removing deprecated implementations..."
echo "----------------------------------------------"
safe_remove "archive/deprecated_implementations/PitchShifter_OLD.h"
safe_remove "archive/deprecated_implementations/PitchShifter_OLD.cpp"
safe_remove "archive/deprecated_implementations/TapeEcho_OLD.h"
safe_remove "archive/deprecated_implementations/TapeEcho_OLD.cpp"
safe_remove "archive/deprecated_implementations/CombResonator_OLD.cpp"
safe_remove "archive/deprecated_implementations/CombResonator_OLD.h"
safe_remove "archive/deprecated_implementations/PhasedVocoder_OLD.h"
safe_remove "archive/deprecated_implementations/PhasedVocoder_OLD.cpp"

echo ""
echo "Step 2: Removing backup files..."
echo "---------------------------------"
safe_remove "JUCE_Plugin/ChimeraPhoenix.jucer.bak"
safe_remove "JUCE_Plugin/ChimeraPhoenix.jucer.backup"
safe_remove "AI_Server/engine_defaults.py.backup"

echo ""
echo "Step 3: Checking for EAM references in documentation..."
echo "-------------------------------------------------------"
docs_to_update=(
    "ALL_57_ENGINES_VERIFIED.md"
    "ENGINE_CATEGORY_MANAGEMENT_SYSTEM.md"
    "CODEBASE_GHOST_INVENTORY.md"
    "DEFINITIVE_ANSWER.md"
    "PARAMETER_MAPPING_PROOF.md"
)

for doc in "${docs_to_update[@]}"; do
    if [ -f "$doc" ]; then
        count=$(grep -c "EAM\|EngineArchitectureManager" "$doc" 2>/dev/null || echo "0")
        if [ "$count" -gt 0 ]; then
            echo "  Found $count EAM references in: $doc"
        fi
    fi
done

echo ""
echo "======================================"
echo "Cleanup Summary"
echo "======================================"
echo "Legacy files backed up to: $BACKUP_DIR"
echo "Run update_documentation.sh to clean docs"
echo "Cleanup complete!"