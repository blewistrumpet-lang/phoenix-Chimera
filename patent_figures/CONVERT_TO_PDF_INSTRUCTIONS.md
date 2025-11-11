# HOW TO CONVERT HTML FIGURES TO PDF

You have 11 HTML files in the `patent_figures/` directory. Here's how to convert them to PDFs:

## METHOD 1: Mac - Using Safari or Chrome (EASIEST - 5 minutes total)

### For Each Figure:

1. **Double-click** `Figure_1_System_Architecture.html`
   - Opens in your default browser (Safari/Chrome/Firefox)

2. **Print to PDF:**
   - **Mac:** Cmd+P (or File → Print)
   - In print dialog, click **"PDF"** button (bottom left)
   - Select **"Save as PDF"**
   - Save as: `Figure_1_System_Architecture.pdf`

3. **Repeat for all 11 figures** (or use script below)

**Time:** 30 seconds per figure = 5-6 minutes total

---

## METHOD 2: Automated Script (Mac - FASTEST - 30 seconds)

I'll create a script that converts all 11 HTML files to PDF automatically.

**Run this command:**

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/patent_figures

# Convert each HTML to PDF using Chrome headless
for file in Figure_*.html; do
    /Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome \
        --headless \
        --disable-gpu \
        --print-to-pdf="${file%.html}.pdf" \
        "$file"
    echo "Converted: $file → ${file%.html}.pdf"
done

echo "All figures converted to PDF!"
ls -lh Figure_*.pdf
```

**If you don't have Chrome, use Safari:**

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/patent_figures

# Convert using Safari (requires scripting - more complex)
# Easier to just do manually with Cmd+P method above
```

---

## METHOD 3: Manual Browser Method (Step-by-Step)

### Figure 1:

1. Open **Safari** (or Chrome)
2. File → Open File → Navigate to: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/patent_figures/Figure_1_System_Architecture.html`
3. **Press Cmd+P** (Print)
4. Click **"PDF"** dropdown (bottom left)
5. Select **"Save as PDF"**
6. Save as: `Figure_1_System_Architecture.pdf` (in same folder)
7. Click **"Save"**

### Repeat for Figures 2-11

**Files you're creating:**
- Figure_1_System_Architecture.pdf
- Figure_2_Three_Tier_Routing.pdf
- Figure_3_Trinity_Pipeline.pdf
- Figure_4_Hardware_Layout.pdf
- Figure_5_Signal_Flow.pdf
- Figure_6_AB_Bank_System.pdf
- Figure_7_Fallback_System.pdf
- Figure_8_Embedded_Platform.pdf
- Figure_9_Parameter_Nudging.pdf
- Figure_10_Chain_Optimization.pdf
- Figure_11_Voice_Workflow.pdf

---

## METHOD 4: Combine All PDFs into One (After Converting)

### Mac Preview:

1. Open `Figure_1_System_Architecture.pdf` in Preview
2. **View → Thumbnails** (sidebar appears on left)
3. **Drag** `Figure_2_Three_Tier_Routing.pdf` into sidebar (drop below Figure 1)
4. **Drag** `Figure_3_Trinity_Pipeline.pdf` into sidebar
5. Continue for all 11 figures
6. **File → Export as PDF**
7. Save as: `Chimera_Phoenix_Drawings.pdf`

### Online Tool:

1. Go to: https://www.ilovepdf.com/merge_pdf
2. Click **"Select PDF files"**
3. Upload all 11 PDF files
4. Arrange in order (1-11)
5. Click **"Merge PDF"**
6. Download: `Chimera_Phoenix_Drawings.pdf`

---

## VERIFY YOUR PDFs

Before filing, check each PDF:

- [ ] Opens correctly in Preview/Adobe Reader
- [ ] All text is readable
- [ ] Boxes and arrows are visible
- [ ] Figure title appears at bottom
- [ ] No password protection
- [ ] File size reasonable (<5 MB each, <25 MB combined)

---

## WHAT YOU'LL HAVE

After conversion:

```
patent_figures/
├── Figure_1_System_Architecture.html ✅ (created)
├── Figure_1_System_Architecture.pdf  📋 (you create from HTML)
├── Figure_2_Three_Tier_Routing.html ✅
├── Figure_2_Three_Tier_Routing.pdf  📋
... (all 11 figures)
├── Chimera_Phoenix_Drawings.pdf     📋 (combined, for USPTO filing)
```

---

## READY TO FILE

Once you have `Chimera_Phoenix_Drawings.pdf`, you're ready to file with USPTO!

**You'll upload 3 files total:**
1. Chimera_Phoenix_Specification.pdf (your 40+ page spec)
2. Chimera_Phoenix_Drawings.pdf (all 11 figures combined)
3. SB16_Cover_Sheet.pdf (form you fill out)

**That's it!**
