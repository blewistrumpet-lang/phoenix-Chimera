#pragma once
#include <JuceHeader.h>

// =====================================================================
// POLISHED UI COMPONENTS FOR PI - Plan A Full Component Overhaul
// =====================================================================

/**
 * GradientCard - Reusable card with gradient border, shadow, rounded corners
 */
class GradientCard : public juce::Component {
public:
    GradientCard() = default;
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Drop shadow
        juce::DropShadow shadow(juce::Colour(0x40000000), 8, juce::Point<int>(0, 2));
        shadow.drawForRectangle(g, bounds.toNearestInt());
        
        // Card background
        g.setColour(juce::Colour(0xff252525));
        g.fillRoundedRectangle(bounds, 8.0f);
        
        // Gradient border
        juce::ColourGradient gradient(
            juce::Colour(0xff06b6d4), bounds.getTopLeft(),
            juce::Colour(0xff8b7ac7), bounds.getBottomRight(),
            false
        );
        g.setGradientFill(gradient);
        g.drawRoundedRectangle(bounds.reduced(1.0f), 8.0f, 2.0f);
    }
};

/**
 * GradientButton - Premium button with blue-to-purple gradient and press feedback
 */
class GradientButton : public juce::Component {
public:
    GradientButton(const juce::String& text) : buttonText(text) {}

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Premium brand colors
        const juce::Colour brandBlue(0xff0a84ff);    // #0A84FF
        const juce::Colour brandPurple(0xff5e5ce6);  // #5E5CE6
        const juce::Colour midColor(0xff3462f7);     // Interpolated middle

        // Gradient fill (blue to purple)
        juce::ColourGradient gradient(
            brandBlue, bounds.getX(), bounds.getCentreY(),
            brandPurple, bounds.getRight(), bounds.getCentreY(),
            false
        );

        // Add middle color for smoother gradient
        gradient.addColour(0.5, midColor);

        if (isPressed) {
            // Slightly darker when pressed
            gradient = juce::ColourGradient(
                brandBlue.darker(0.15f), bounds.getX(), bounds.getCentreY(),
                brandPurple.darker(0.15f), bounds.getRight(), bounds.getCentreY(),
                false
            );
            gradient.addColour(0.5, midColor.darker(0.15f));
        }

        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, 16.0f);  // 16px radius for premium look

        // Subtle top highlight for depth
        g.setColour(juce::Colour(0x20ffffff));
        g.drawHorizontalLine(bounds.getY() + 2, bounds.getX() + 16, bounds.getRight() - 16);

        // Text with proper font
        g.setColour(juce::Colours::white);
        auto buttonFont = juce::Font(juce::Font::getDefaultSansSerifFontName(), 18.0f, juce::Font::bold);
        buttonFont.setExtraKerningFactor(0.03f);  // Slight letter spacing
        g.setFont(buttonFont);
        g.drawText(buttonText, bounds, juce::Justification::centred);
    }
    
    void mouseDown(const juce::MouseEvent&) override {
        isPressed = true;
        // Scale down slightly on press (premium feedback)
        setTransform(juce::AffineTransform::scale(0.98f, 0.98f,
                                                  getWidth() / 2.0f, getHeight() / 2.0f));
        repaint();
        if (onPress) onPress();
    }

    void mouseUp(const juce::MouseEvent&) override {
        isPressed = false;
        // Return to normal scale
        setTransform(juce::AffineTransform());
        repaint();
        if (onRelease) onRelease();
    }
    
    void setButtonText(const juce::String& newText) {
        buttonText = newText;
        repaint();
    }

    std::function<void()> onPress;
    std::function<void()> onRelease;

private:
    juce::String buttonText;
    bool isPressed = false;
};

/**
 * GradientMeter - Larger meter with green→yellow→red gradient
 */
class GradientMeter : public juce::Component {
public:
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Premium colors
        const juce::Colour surfaceDark(0xff1c1c1e);      // #1C1C1E
        const juce::Colour statusSuccess(0xff30d158);    // #30D158 Green
        const juce::Colour statusWarning(0xffffd60a);    // #FFD60A Yellow
        const juce::Colour statusError(0xffff453a);      // #FF453A Red

        // Dark inset background
        g.setColour(surfaceDark);
        g.fillRoundedRectangle(bounds, 6.0f);

        // Subtle inner shadow for depth
        g.setColour(juce::Colour(0, 0, 0).withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

        // Level bar with gradient
        float dbLevel = juce::Decibels::gainToDecibels(level);
        float normalizedLevel = juce::jmap(dbLevel, -60.0f, 0.0f, 0.0f, 1.0f);
        normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);

        if (normalizedLevel > 0.01f) {
            auto levelBounds = bounds.reduced(3.0f);
            levelBounds = levelBounds.removeFromBottom(levelBounds.getHeight() * normalizedLevel);

            // Premium gradient: Green → Yellow → Red
            juce::ColourGradient gradient(
                statusSuccess, 0.0f, levelBounds.getBottom(),
                statusError, 0.0f, levelBounds.getY(),
                false
            );
            gradient.addColour(0.7, statusWarning);  // Yellow at 70%

            g.setGradientFill(gradient);
            g.fillRoundedRectangle(levelBounds, 4.0f);

            // Subtle top highlight for visual depth
            g.setColour(juce::Colour(0x20ffffff));
            g.drawHorizontalLine(levelBounds.getY() + 1, levelBounds.getX() + 4, levelBounds.getRight() - 4);
        }
    }
    
    void setLevel(float newLevel) {
        level = newLevel * 0.9f + level * 0.1f;  // Smooth
        repaint();
    }
    
private:
    float level = 0.0f;
};

/**
 * EngineSlot - Single colored slot box with engine name
 */
class EngineSlot : public juce::Component {
public:
    EngineSlot(int slotNum) : slotNumber(slotNum) {}
    
    void setEngine(int engineID, const juce::String& name) {
        currentEngineID = engineID;
        engineName = name;
        categoryColor = getColorForEngine(engineID);
        repaint();
    }
    
    void clearEngine() {
        currentEngineID = 0;
        engineName = "";  // Empty string - don't draw anything
        categoryColor = juce::Colour(0xff1c1c1e);
        repaint();
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Premium colors
        const juce::Colour surfaceDark(0xff1c1c1e);   // #1C1C1E - Empty slots
        const juce::Colour textPrimary = juce::Colours::white;
        const juce::Colour textTertiary = juce::Colours::white.withAlpha(0.3f);

        const float cornerRadius = 12.0f;

        if (currentEngineID == 0) {
            // ========== EMPTY SLOT STYLING ==========
            // Subtle dark gray background (#1C1C1E - not pure black)
            g.setColour(juce::Colour(28, 28, 30));  // #1C1C1E
            g.fillRoundedRectangle(bounds, cornerRadius);

            // Subtle border for definition (5% white)
            g.setColour(juce::Colour(255, 255, 255).withAlpha(0.05f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);

            // Slot number (more visible - 40% white instead of 30%)
            g.setColour(juce::Colours::white.withAlpha(0.4f));  // 40% white
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::bold));
            g.drawText(juce::String(slotNumber),
                      bounds.withWidth(24).withHeight(24).translated(8, 6).toNearestInt(),
                      juce::Justification::topLeft);

            // No center text - keep it minimal and clean

        } else {
            // ========== ACTIVE SLOT STYLING ==========
            // Subtle shadow below (simple offset for performance)
            g.setColour(juce::Colour(0, 0, 0).withAlpha(0.25f));
            g.fillRoundedRectangle(bounds.translated(0, 2), cornerRadius);

            // Gradient background with category color (very subtle)
            juce::ColourGradient gradient(
                categoryColor.withAlpha(0.15f), bounds.getX(), bounds.getY(),
                categoryColor.withAlpha(0.05f), bounds.getRight(), bounds.getBottom(),
                false
            );
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(bounds, cornerRadius);

            // Border with category color
            g.setColour(categoryColor.withAlpha(0.3f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);

            // Slot number (subtle, top-left corner)
            g.setColour(textTertiary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::bold));
            g.drawText(juce::String(slotNumber),
                      bounds.withWidth(24).withHeight(24).translated(8, 6).toNearestInt(),
                      juce::Justification::topLeft);

            // Engine name (bright white, prominent) - only draw if not empty
            // Larger font size for taller slots
            if (engineName.isNotEmpty()) {
                g.setColour(textPrimary);
                g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::bold));
                auto textBounds = bounds.reduced(8.0f);
                g.drawFittedText(engineName, textBounds.toNearestInt(), juce::Justification::centred, 3);
            }
        }
    }
    
private:
    int slotNumber;
    int currentEngineID = 0;
    juce::String engineName = "";  // Empty by default
    juce::Colour categoryColor = juce::Colour(0xff1c1c1e);
    
    juce::Colour getColorForEngine(int engineID) {
        // Premium Apple-esque category colors
        if (engineID == 0) return juce::Colour(0xff1c1c1e); // Empty - dark gray

        // Dynamics (1-6): Purple
        if (engineID >= 1 && engineID <= 6) return juce::Colour(0xffbf5af2);  // #BF5AF2

        // EQ/Filters (7-14): Green
        if (engineID >= 7 && engineID <= 14) return juce::Colour(0xff30d158);  // #30D158

        // Distortion (15-22): Red
        if (engineID >= 15 && engineID <= 22) return juce::Colour(0xffff453a);  // #FF453A

        // Modulation (23-30): Blue
        if (engineID >= 23 && engineID <= 30) return juce::Colour(0xff0a84ff);  // #0A84FF

        // Pitch/Harmony (31-33): Yellow (using same as filters for now)
        if (engineID >= 31 && engineID <= 33) return juce::Colour(0xff30d158);  // #30D158

        // Delay/Echo (34-38): Blue (modulation color)
        if (engineID >= 34 && engineID <= 38) return juce::Colour(0xff0a84ff);  // #0A84FF

        // Reverb (39-43): Cyan
        if (engineID >= 39 && engineID <= 43) return juce::Colour(0xff64d2ff);  // #64D2FF

        // Spatial (44-46): Orange
        if (engineID >= 44 && engineID <= 46) return juce::Colour(0xffff9f0a);  // #FF9F0A

        // Spectral/Special (47-52): Cyan (reverb color)
        if (engineID >= 47 && engineID <= 52) return juce::Colour(0xff64d2ff);  // #64D2FF

        // Utility (53-56): Gray
        if (engineID >= 53 && engineID <= 56) return juce::Colour(0xff98989d);  // #98989D

        return juce::Colour(0xff1c1c1e); // Default - dark gray
    }
};

/**
 * EngineSlotGrid - Container for 6 slots in horizontal row
 */
class EngineSlotGrid : public juce::Component {
public:
    EngineSlotGrid() {
        for (int i = 0; i < 6; ++i) {
            slots[i] = std::make_unique<EngineSlot>(i + 1);
            addAndMakeVisible(slots[i].get());
        }
    }
    
    void updateSlot(int slotIndex, int engineID, const juce::String& engineName) {
        if (slotIndex >= 0 && slotIndex < 6) {
            if (engineID == 0) {
                slots[slotIndex]->clearEngine();
            } else {
                slots[slotIndex]->setEngine(engineID, engineName);
            }
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        const int gap = 8;  // Tight spacing between slots (8px)
        const int totalGap = gap * 5;  // 5 gaps between 6 slots
        const int slotWidth = (bounds.getWidth() - totalGap) / 6;

        for (int i = 0; i < 6; ++i) {
            int x = i * (slotWidth + gap);
            slots[i]->setBounds(x, 0, slotWidth, bounds.getHeight());
        }
    }
    
private:
    std::array<std::unique_ptr<EngineSlot>, 6> slots;
};

