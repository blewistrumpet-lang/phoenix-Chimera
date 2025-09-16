#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/SlotComponent.h"
#include "JUCE_Plugin/Source/EngineFactory.h"

class TestWindow : public juce::DocumentWindow
{
public:
    TestWindow() : DocumentWindow("Slot Component Test", 
                                  juce::Colours::black,
                                  DocumentWindow::allButtons)
    {
        slotComponent = std::make_unique<SlotComponent>(0);
        setContentOwned(slotComponent.get(), false);
        
        // Create a test engine
        auto engine = EngineFactory::createEngine(1); // ClassicCompressor
        
        std::cout << "\n=== Testing SlotComponent ===" << std::endl;
        
        if (engine) {
            std::cout << "Engine created: " << engine->getName().toStdString() << std::endl;
            std::cout << "Parameter count: " << engine->getNumParameters() << std::endl;
            
            // Update the slot component
            slotComponent->update(engine.get(), 1);
            
            // Force size and layout
            slotComponent->setSize(400, 600);
            
            // Check slider visibility
            for (int i = 0; i < 15; ++i) {
                auto* slider = slotComponent->getSlider(i);
                if (slider) {
                    std::cout << "Slider " << i << ": "
                              << "visible=" << (slider->isVisible() ? "YES" : "NO")
                              << ", bounds=" << slider->getBounds().toString().toStdString()
                              << ", value=" << slider->getValue()
                              << std::endl;
                }
            }
        }
        
        setSize(400, 600);
        setVisible(true);
    }
    
    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    
private:
    std::unique_ptr<SlotComponent> slotComponent;
};

class TestApp : public juce::JUCEApplication
{
public:
    TestApp() {}
    
    const juce::String getApplicationName() override { return "SlotTest"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    
    void initialise(const juce::String&) override
    {
        window = std::make_unique<TestWindow>();
    }
    
    void shutdown() override
    {
        window = nullptr;
    }
    
private:
    std::unique_ptr<TestWindow> window;
};

START_JUCE_APPLICATION(TestApp)