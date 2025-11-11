#!/bin/bash

# Script to convert all HTML figures to PDF using Chrome headless
# Run from patent_figures directory

echo "Converting HTML figures to PDF..."
echo ""

# Check if Chrome is installed
if [ ! -f "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" ]; then
    echo "Google Chrome not found. Please install Chrome or use manual method."
    echo "See CONVERT_TO_PDF_INSTRUCTIONS.md for manual instructions."
    exit 1
fi

# Convert each HTML file
for file in Figure_*.html; do
    if [ -f "$file" ]; then
        output="${file%.html}.pdf"

        /Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome \
            --headless \
            --disable-gpu \
            --print-to-pdf="$output" \
            "$file" 2>/dev/null

        if [ -f "$output" ]; then
            echo "✓ Converted: $file → $output"
        else
            echo "✗ Failed: $file"
        fi
    fi
done

echo ""
echo "Conversion complete!"
echo ""
echo "PDF files created:"
ls -lh Figure_*.pdf 2>/dev/null || echo "No PDF files found - conversion may have failed"

echo ""
echo "Next step: Combine all PDFs into Chimera_Phoenix_Drawings.pdf"
echo "Use Preview (View → Thumbnails → drag PDFs) or https://www.ilovepdf.com/merge_pdf"
