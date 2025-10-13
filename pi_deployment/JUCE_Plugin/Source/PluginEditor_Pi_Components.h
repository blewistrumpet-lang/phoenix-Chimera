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
 * GradientButton - Modern button with gradient fill
 */
class GradientButton : public juce::Component {
public:
    GradientButton(const juce::String& text) : buttonText(text) {}
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Gradient fill (cyan to teal)
        juce::ColourGradient gradient(
            juce::Colour(0xff06b6d4), bounds.getTopLeft(),
            juce::Colour(0xff0891b2), bounds.getBottomRight(),
            false
        );
        
        if (isPressed) {
            gradient = juce::ColourGradient(
                juce::Colour(0xff0891b2), bounds.getTopLeft(),
                juce::Colour(0xff06b6d4), bounds.getBottomRight(),
                false
            );
        }
        
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, 8.0f);
        
        // Text
        g.setColour(juce::Colours::black);
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText(buttonText, bounds, juce::Justification::centred);
    }
    
    void mouseDown(const juce::MouseEvent&) override {
        isPressed = true;
        repaint();
        if (onPress) onPress();
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        isPressed = false;
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
        
        // Background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds, 3.0f);
        
        // Border
        g.setColour(juce::Colour(0xff404040));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
        
        // Level bar with gradient
        float dbLevel = juce::Decibels::gainToDecibels(level);
        float normalizedLevel = juce::jmap(dbLevel, -60.0f, 0.0f, 0.0f, 1.0f);
        normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);
        
        if (normalizedLevel > 0.01f) {
            auto levelBounds = bounds.reduced(2.0f);
            levelBounds = levelBounds.removeFromBottom(levelBounds.getHeight() * normalizedLevel);
            
            // Create gradient green → yellow → red
            juce::ColourGradient gradient(
                juce::Colour(0xff4ade80), levelBounds.getBottom(), 0.0f,
                juce::Colour(0xffef4444), levelBounds.getY(), 0.0f,
                false
            );
            gradient.addColour(0.6, juce::Colour(0xfffbbf24)); // Yellow at 60%
            
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(levelBounds, 2.0f);
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
        engineName = "EMPTY";
        categoryColor = juce::Colour(0xff3a3a3a);
        repaint();
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Drop shadow
        if (currentEngineID != 0) {
            juce::DropShadow shadow(juce::Colour(0x30000000), 4, juce::Point<int>(0, 1));
            shadow.drawForRectangle(g, bounds.toNearestInt());
        }
        
        // Gradient background
        juce::ColourGradient gradient(
            categoryColor, bounds.getTopLeft(),
            categoryColor.darker(0.3f), bounds.getBottomRight(),
            false
        );
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, 6.0f);
        
        // Border
        g.setColour(categoryColor.brighter(0.2f));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 6.0f, 1.5f);
        
        // Engine name
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        auto textBounds = bounds.reduced(4.0f);
        g.drawFittedText(engineName, textBounds.toNearestInt(), juce::Justification::centred, 2);
        
        // Slot number (small, top-left)
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(juce::Font(8.0f));
        g.drawText(juce::String(slotNumber), bounds.reduced(3.0f).toNearestInt(), 
                   juce::Justification::topLeft);
    }
    
private:
    int slotNumber;
    int currentEngineID = 0;
    juce::String engineName = "EMPTY";
    juce::Colour categoryColor = juce::Colour(0xff3a3a3a);
    
    juce::Colour getColorForEngine(int engineID) {
        // Map engine ID to category color
        if (engineID == 0) return juce::Colour(0xff3a3a3a); // Empty - gray
        
        // Dynamics (1-6): Blue
        if (engineID >= 1 && engineID <= 6) return juce::Colour(0xff4a7c9e);
        
        // EQ/Filters (7-14): Green
        if (engineID >= 7 && engineID <= 14) return juce::Colour(0xff4a9e6f);
        
        // Distortion (15-22): Red/Orange
        if (engineID >= 15 && engineID <= 22) return juce::Colour(0xffd97757);
        
        // Modulation (23-30): Purple
        if (engineID >= 23 && engineID <= 30) return juce::Colour(0xff8b7ac7);
        
        // Pitch/Harmony (31-33): Yellow
        if (engineID >= 31 && engineID <= 33) return juce::Colour(0xffc7a84a);
        
        // Delay/Echo (34-38): Amber
        if (engineID >= 34 && engineID <= 38) return juce::Colour(0xffc78b4a);
        
        // Reverb (39-43): Cyan
        if (engineID >= 39 && engineID <= 43) return juce::Colour(0xff4ac7c7);
        
        // Spatial (44-46): Magenta
        if (engineID >= 44 && engineID <= 46) return juce::Colour(0xffc74ac7);
        
        // Spectral/Special (47-52): Teal
        if (engineID >= 47 && engineID <= 52) return juce::Colour(0xff4ac79e);
        
        // Utility (53-56): Gray
        if (engineID >= 53 && engineID <= 56) return juce::Colour(0xff6b6b6b);
        
        return juce::Colour(0xff3a3a3a); // Default gray
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
        int slotWidth = bounds.getWidth() / 6;
        int gap = 6;
        int actualSlotWidth = slotWidth - gap;
        
        for (int i = 0; i < 6; ++i) {
            slots[i]->setBounds(i * slotWidth + gap/2, 0, actualSlotWidth, bounds.getHeight());
        }
    }
    
private:
    std::array<std::unique_ptr<EngineSlot>, 6> slots;
};

