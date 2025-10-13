#!/bin/bash

echo "============================================================"
echo "DISTORTION ENGINES (15-22) - RESULT EXTRACTOR"
echo "============================================================"
echo ""

# Check if report exists
if [ ! -f "REAL_WORLD_AUDIO_TESTING_REPORT.md" ]; then
    echo "ERROR: REAL_WORLD_AUDIO_TESTING_REPORT.md not found"
    echo ""
    echo "Please run first:"
    echo "  ./run_real_world_tests.sh"
    echo ""
    exit 1
fi

OUTPUT="DISTORTION_REALWORLD_EXTRACTED_REPORT.md"

echo "Extracting distortion engine results..."

# Create report header
cat > $OUTPUT << 'EOF'
# DISTORTION ENGINES REAL-WORLD TESTING REPORT (Extracted)

**Engines Tested**: 15-22 (8 distortion engines)
**Source**: Full real-world audio testing suite
**Test Materials**: Guitar, Bass, Drums, Vocals, Piano, Noise

---

## EXECUTIVE SUMMARY

### Distortion Engines (15-22)

| ID | Engine Name | Grade | Status |
|----|-------------|-------|--------|
EOF

# Extract engine 15-22 data
for i in {15..22}; do
    echo "  Processing Engine $i..."

    # Extract section for this engine
    sed -n "/^### Engine $i:/,/^### Engine $((i+1)):/p" REAL_WORLD_AUDIO_TESTING_REPORT.md | \
        head -n -1 >> temp_engine_$i.txt

    # If it's engine 22, go to end of detailed section
    if [ $i -eq 22 ]; then
        sed -n "/^### Engine 22:/,/^## RECOMMENDATIONS/p" REAL_WORLD_AUDIO_TESTING_REPORT.md | \
            head -n -1 > temp_engine_22.txt
    fi
done

# Parse summary data
for i in {15..22}; do
    if [ -f "temp_engine_$i.txt" ]; then
        ENGINE_NAME=$(grep "^### Engine $i:" temp_engine_$i.txt | sed 's/### Engine [0-9]*: //')
        GRADE=$(grep "**Overall Grade**:" temp_engine_$i.txt | sed 's/.*: //')

        # Determine status
        if [[ "$GRADE" == "A" || "$GRADE" == "B" ]]; then
            STATUS="✅ Production Ready"
        elif [[ "$GRADE" == "C" ]]; then
            STATUS="⚠️  Acceptable"
        else
            STATUS="🚨 Needs Work"
        fi

        echo "| $i | $ENGINE_NAME | $GRADE | $STATUS |" >> $OUTPUT
    fi
done

cat >> $OUTPUT << 'EOF'

---

## DETAILED RESULTS BY ENGINE

EOF

# Append full details for each engine
for i in {15..22}; do
    if [ -f "temp_engine_$i.txt" ]; then
        cat temp_engine_$i.txt >> $OUTPUT
        echo "" >> $OUTPUT
        echo "---" >> $OUTPUT
        echo "" >> $OUTPUT
    fi
done

# Add analysis section
cat >> $OUTPUT << 'EOF'

## DISTORTION CHARACTER ANALYSIS

### Warm Distortions (Even Harmonic Dominant)
Best for: Vocals, guitars, smooth overdrive

EOF

echo "**Candidates:** (Check detailed results for even/odd harmonic balance)" >> $OUTPUT
echo "" >> $OUTPUT

cat >> $OUTPUT << 'EOF'

### Aggressive Distortions (Odd Harmonic Dominant)
Best for: Heavy guitars, bass, fuzz tones

EOF

echo "**Candidates:** (Check detailed results for odd harmonic dominance)" >> $OUTPUT
echo "" >> $OUTPUT

cat >> $OUTPUT << 'EOF'

### Digital/Bitcrushed
Best for: Lo-fi, creative effects

- Engine 18: Bit Crusher

---

## RECOMMENDATIONS

### Production-Ready Distortions
Engines with Grade B or better and no critical issues:

EOF

# Extract production-ready engines (15-22 only)
grep -A 1 "^- \*\*Engine 1[5-9]\|^- \*\*Engine 2[0-2]" REAL_WORLD_AUDIO_TESTING_REPORT.md | \
    grep "Production-Ready" -B 1 | grep "^- " >> $OUTPUT 2>/dev/null || echo "_Check detailed results above_" >> $OUTPUT

cat >> $OUTPUT << 'EOF'

### Needs Improvement
Engines requiring attention:

EOF

# Extract problem engines (15-22 only)
grep "^- \*\*Engine 1[5-9]\|^- \*\*Engine 2[0-2]" REAL_WORLD_AUDIO_TESTING_REPORT.md | \
    grep -v "Production-Ready" >> $OUTPUT 2>/dev/null || echo "_No major issues detected_" >> $OUTPUT

cat >> $OUTPUT << 'EOF'

---

## TESTING METHODOLOGY

**Drive Levels Tested**: Moderate settings (parameter = 0.5)
**Materials**: 7 different real-world audio sources
**Quality Criteria**:
- Dynamic range preservation
- Artifact detection
- Clipping analysis
- Noise floor measurement
- DC offset detection

**Grading**:
- A: Excellent transparency or musical coloration
- B: Good quality, minor issues
- C: Acceptable for production
- D: Significant degradation
- F: Critical failures

---

## USAGE RECOMMENDATIONS

### For Vocals/Acoustic
Use warm, even-harmonic distortions (check individual results)

### For Electric Guitar
Engines 20-22 (Muff Fuzz, Rodent, K-Style) - classic tones

### For Bass
Lower-order distortions with good low-end preservation

### For Drums
Subtle saturation or aggressive fuzz depending on style

### For Creative Effects
Bit Crusher (18), WaveFolder (16) for experimental sounds

---

**Report Generated**: ` + "`date`" + `

EOF

# Cleanup
rm -f temp_engine_*.txt

echo ""
echo "============================================================"
echo "✅ EXTRACTION COMPLETE"
echo "============================================================"
echo ""
echo "Report saved to: $OUTPUT"
echo ""
echo "To view:"
echo "  open $OUTPUT"
echo ""
