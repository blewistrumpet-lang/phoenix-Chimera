#include <iostream>
#include <JuceHeader.h>

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "Testing JUCE Slider visibility\n";
    std::cout << "================================\n\n";
    
    // Create a simple slider
    juce::Slider testSlider;
    
    std::cout << "Initial state:\n";
    std::cout << "  Visible: " << (testSlider.isVisible() ? "YES" : "NO") << "\n";
    std::cout << "  Bounds: " << testSlider.getBounds().toString().toStdString() << "\n\n";
    
    // Configure it like we do in SlotComponent
    testSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    testSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
    testSlider.setRange(0.0, 1.0, 0.001);
    testSlider.setValue(0.5);
    
    std::cout << "After configuration:\n";
    std::cout << "  Visible: " << (testSlider.isVisible() ? "YES" : "NO") << "\n";
    std::cout << "  Range: " << testSlider.getMinimum() << " to " << testSlider.getMaximum() << "\n";
    std::cout << "  Value: " << testSlider.getValue() << "\n\n";
    
    // Set visible
    testSlider.setVisible(true);
    std::cout << "After setVisible(true):\n";
    std::cout << "  Visible: " << (testSlider.isVisible() ? "YES" : "NO") << "\n\n";
    
    // Set bounds
    testSlider.setBounds(10, 10, 50, 50);
    std::cout << "After setBounds(10,10,50,50):\n";
    std::cout << "  Visible: " << (testSlider.isVisible() ? "YES" : "NO") << "\n";
    std::cout << "  Bounds: " << testSlider.getBounds().toString().toStdString() << "\n\n";
    
    // Test with colors
    testSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ffcc));
    testSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xffffffff));
    
    std::cout << "Slider configuration complete.\n";
    std::cout << "Final visibility: " << (testSlider.isVisible() ? "YES" : "NO") << "\n";
    
    return 0;
}