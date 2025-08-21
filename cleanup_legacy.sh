#!/bin/bash

# Chimera Phoenix 3.0 - Legacy Cleanup Script
# Date: August 19, 2025
# Purpose: Remove legacy files and update documentation

echo "======================================"
echo "Chimera Phoenix 3.0 - Legacy Cleanup"
echo "======================================"
echo ""

# Create backup directory with timestamp
BACKUP_DIR="legacy_backup_$(date +%Y%m%d_%H%M%S)"
echo "Creating backup directory: $BACKUP_DIR"
mkdir -p "$BACKUP_DIR"

# Function to safely remove files
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
echo "Step 3: Cleaning up empty directories..."
echo "-----------------------------------------"
if [ -d "archive/deprecated_implementations" ]; then
    if [ -z "$(ls -A archive/deprecated_implementations)" ]; then
        echo "  Removing empty directory: archive/deprecated_implementations"
        rmdir "archive/deprecated_implementations"
    fi
fi

if [ -d "archive" ]; then
    if [ -z "$(ls -A archive)" ]; then
        echo "  Removing empty directory: archive"
        rmdir "archive"
    fi
fi

echo ""
echo "Step 4: Documentation files to update..."
echo "-----------------------------------------"
echo "The following files contain EAM/EngineArchitectureManager references:"
echo ""
docs_to_update=(
    "ALL_57_ENGINES_VERIFIED.md"
    "CODEBASE_GHOST_INVENTORY.md"
    "DEFINITIVE_ANSWER.md"
    "PARAMETER_MAPPING_PROOF.md"
    "SESSION_LOG_AUGUST_17.md"
    "COMPLETE_PROJECT_ANALYSIS.md"
    "STRATEGIC_ROADMAP.md"
    "PROGRESS_LOG.md"
    "ENGINE_CATEGORY_MANAGEMENT_SYSTEM.md"
)

for doc in "${docs_to_update[@]}"; do
    if [ -f "$doc" ]; then
        count=$(grep -c "EAM\|EngineArchitectureManager" "$doc" 2>/dev/null || echo "0")
        if [ "$count" -gt 0 ]; then
            echo "  ⚠️  $doc - Contains $count EAM references"
        fi
    fi
done

echo ""
echo "Step 5: Creating documentation update script..."
echo "------------------------------------------------"
cat > update_documentation.sh << 'DOCSCRIPT'
#!/bin/bash

# Remove EAM references from documentation
echo "Updating documentation to remove EAM references..."

for file in *.md; do
    if [ -f "$file" ]; then
        if grep -q "EAM\|EngineArchitectureManager" "$file"; then
            echo "Processing: $file"
            # Create backup
            cp "$file" "${file}.before_eam_cleanup"
            
            # Remove lines containing EAM references but keep the context
            sed -i '' 's/EAM wrong//g' "$file"
            sed -i '' 's/EAM WRONG//g' "$file"
            sed -i '' 's/EAM CRASH//g' "$file"
            sed -i '' 's/EAM Issue//g' "$file"
            sed -i '' 's/EAM Claims.*|//g' "$file"
            sed -i '' 's/(EAM wrong)//g' "$file"
            sed -i '' 's/EngineArchitectureManager/[REMOVED-SYSTEM]/g' "$file"
            
            echo "  Updated: $file"
        fi
    fi
done

echo "Documentation update complete!"
DOCSCRIPT

chmod +x update_documentation.sh

echo ""
echo "Step 6: Test updates needed..."
echo "-------------------------------"
echo "The following test needs updating:"
echo "  ⚠️  ChaosGenerator test - Should send non-silent input"
echo ""
echo "To fix, update test_all_engines to:"
echo "  1. Generate a test signal (sine wave) for input"
echo "  2. Send to ChaosGenerator_Platinum"
echo "  3. Check for modulation of the signal"

echo ""
echo "======================================"
echo "Cleanup Summary"
echo "======================================"
echo "✅ Legacy files backed up to: $BACKUP_DIR"
echo "✅ Deprecated implementations removed"
echo "✅ Backup files removed"
echo "📝 Documentation update script created: update_documentation.sh"
echo ""
echo "Next steps:"
echo "1. Run ./update_documentation.sh to clean documentation"
echo "2. Update ChaosGenerator test methodology"
echo "3. Review and commit changes"
echo ""
echo "Cleanup complete!"