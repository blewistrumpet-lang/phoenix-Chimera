#include "PluginEditorNexusDynamicSafe.h"
#include "EngineFactory.h"

//==============================================================================
PluginEditorNexusDynamicSafe::PluginEditorNexusDynamicSafe(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set window size - no resizing for safety
    setSize(1200, 800);
    
    // Title
    titleLabel.setText("CHIMERA PHOENIX - DYNAMIC NEXUS UI", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ffcc));
    addAndMakeVisible(titleLabel);
    
    // Initialize slots
    for (int i = 0; i < 6; ++i)
    {
        initializeSlot(i);
    }
}

PluginEditorNexusDynamicSafe::~PluginEditorNexusDynamicSafe()
{
    // Clean destruction - components will auto-delete
}

void PluginEditorNexusDynamicSafe::initializeSlot(int slotIndex)
{
    auto& slot = slots[slotIndex];
    
    // Slot label
    slot.label.setText("SLOT " + juce::String(slotIndex + 1), juce::dontSendNotification);
    slot.label.setJustificationType(juce::Justification::centred);
    slot.label.setColour(juce::Label::textColourId, juce::Colour(0xff00ffcc));
    addAndMakeVisible(slot.label);
    
    // Engine selector
    slot.engineSelector.setTextWhenNothingSelected("Select Engine");
    
    // Get parameter safely
    juce::String paramName = "slot" + juce::String(slotIndex + 1) + "_engine";
    auto* param = audioProcessor.getValueTreeState().getParameter(paramName);
    
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
    {
        for (const auto& choice : choiceParam->choices)
        {
            slot.engineSelector.addItem(choice, slot.engineSelector.getNumItems() + 1);
        }
    }
    
    addAndMakeVisible(slot.engineSelector);
    
    // Create attachment
    slot.engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), paramName, slot.engineSelector
    );
    
    // Bypass button
    slot.bypassButton.setButtonText("BYPASS");
    slot.bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffff006e));
    addAndMakeVisible(slot.bypassButton);
    
    juce::String bypassParam = "slot" + juce::String(slotIndex + 1) + "_bypass";
    slot.bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), bypassParam, slot.bypassButton
    );
    
    // Engine change listener
    slot.engineSelector.onChange = [this, slotIndex]()
    {
        updateSlotParameters(slotIndex);
    };
    
    // Initial parameter update
    updateSlotParameters(slotIndex);
}

void PluginEditorNexusDynamicSafe::updateSlotParameters(int slotIndex)
{
    auto& slot = slots[slotIndex];
    
    // Clear old parameters
    for (auto& slider : slot.sliders)
    {
        removeChildComponent(slider.get());
    }
    for (auto& label : slot.labels)
    {
        removeChildComponent(label.get());
    }
    slot.sliders.clear();
    slot.labels.clear();
    slot.attachments.clear();
    
    // Get engine
    auto& engine = audioProcessor.getEngine(slotIndex);
    if (!engine) return;
    
    // Query live engine
    int numParams = engine->getNumParameters();
    
    // Create controls for up to 5 parameters (space limitation)
    for (int i = 0; i < numParams && i < 5; ++i)
    {
        // Get parameter name from live engine
        juce::String paramName = engine->getParameterName(i);
        
        // Create label
        auto label = std::make_unique<juce::Label>();
        label->setText(paramName, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, juce::Colour(0xffE5E7EB));
        label->setFont(juce::Font(10.0f));
        addAndMakeVisible(label.get());
        slot.labels.push_back(std::move(label));
        
        // Create slider
        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ffcc));
        slider->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1F2937));
        addAndMakeVisible(slider.get());
        
        // Create attachment
        juce::String apvtsParam = "slot" + juce::String(slotIndex + 1) + "_param" + juce::String(i + 1);
        auto attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getValueTreeState(), apvtsParam, *slider
        );
        
        slot.sliders.push_back(std::move(slider));
        slot.attachments.push_back(std::move(attachment));
    }
    
    // Trigger layout update
    resized();
}

void PluginEditorNexusDynamicSafe::paint(juce::Graphics& g)
{
    // Simple dark background
    g.fillAll(juce::Colour(0xff111827));
    
    // Draw grid lines for slots
    g.setColour(juce::Colour(0xff1F2937));
    
    // Vertical divider
    g.drawLine(getWidth() / 2.0f, 60, getWidth() / 2.0f, getHeight(), 2);
    
    // Horizontal dividers
    float slotHeight = (getHeight() - 60) / 3.0f;
    for (int i = 1; i < 3; ++i)
    {
        float y = 60 + i * slotHeight;
        g.drawLine(0, y, getWidth(), y, 2);
    }
    
    // Draw slot backgrounds
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        
        auto bounds = juce::Rectangle<float>(
            col * getWidth() / 2.0f + 5,
            60 + row * slotHeight + 5,
            getWidth() / 2.0f - 10,
            slotHeight - 10
        );
        
        g.setColour(juce::Colour(0xff1F2937).withAlpha(0.3f));
        g.fillRoundedRectangle(bounds, 5);
        
        g.setColour(juce::Colour(0xff00ffcc).withAlpha(0.2f));
        g.drawRoundedRectangle(bounds, 5, 1);
    }
}

void PluginEditorNexusDynamicSafe::resized()
{
    auto bounds = getLocalBounds();
    
    // Title at top
    titleLabel.setBounds(bounds.removeFromTop(60).reduced(10));
    
    // Layout slots in 2x3 grid
    float slotWidth = bounds.getWidth() / 2.0f;
    float slotHeight = bounds.getHeight() / 3.0f;
    
    for (int i = 0; i < 6; ++i)
    {
        auto& slot = slots[i];
        int col = i % 2;
        int row = i / 2;
        
        auto slotBounds = juce::Rectangle<int>(
            col * slotWidth + 10,
            bounds.getY() + row * slotHeight + 10,
            slotWidth - 20,
            slotHeight - 20
        );
        
        // Slot label
        slot.label.setBounds(slotBounds.removeFromTop(20));
        
        // Engine selector and bypass
        auto controlRow = slotBounds.removeFromTop(30);
        slot.engineSelector.setBounds(controlRow.removeFromLeft(controlRow.getWidth() * 0.7f));
        slot.bypassButton.setBounds(controlRow);
        
        // Parameters in a row
        if (!slot.sliders.empty())
        {
            auto paramArea = slotBounds.reduced(5);
            int paramWidth = paramArea.getWidth() / slot.sliders.size();
            
            for (size_t p = 0; p < slot.sliders.size(); ++p)
            {
                auto paramBounds = juce::Rectangle<int>(
                    paramArea.getX() + p * paramWidth,
                    paramArea.getY(),
                    paramWidth - 5,
                    paramArea.getHeight()
                );
                
                slot.labels[p]->setBounds(paramBounds.removeFromTop(15));
                slot.sliders[p]->setBounds(paramBounds.withHeight(60));
            }
        }
    }
}