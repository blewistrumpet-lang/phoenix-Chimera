#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/NexusLookAndFeel.h"
#include "JUCE_Plugin/Source/SlotComponent.h"
#include "JUCE_Plugin/Source/EngineFactory.h"

class TestWindow : public juce::DocumentWindow
{
public:
    TestWindow(const juce::String& name)
        : DocumentWindow(name, juce::Colours::black, DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(), true);
        setResizable(true, true);
        centreWithSize(800, 600);
        setVisible(true);
    }
    
    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    
private:
    class MainComponent : public juce::Component
    {
    public:
        MainComponent()
        {
            nexusLook = std::make_unique<NexusLookAndFeel>();
            setLookAndFeel(nexusLook.get());
            
            // Create a slot component
            slot = std::make_unique<SlotComponent>(0);
            slot->setLookAndFeel(nexusLook.get());
            addAndMakeVisible(slot.get());
            
            // Test with different engines
            testEngineIndex = 0;
            
            setSize(800, 600);
            startTimer(3000); // Switch engines every 3 seconds
        }
        
        ~MainComponent()
        {
            if (slot)
                slot->setLookAndFeel(nullptr);
            setLookAndFeel(nullptr);
        }
        
        void paint(juce::Graphics& g) override
        {
            // Carbon fiber background
            NexusLookAndFeel::drawCarbonFiberBackground(g, getLocalBounds().toFloat());
            
            // Title
            g.setColour(juce::Colour(0xff00ffcc));
            g.setFont(juce::Font(28.0f));
            g.drawText("CHIMERA PHOENIX - NEXUS ENGINE", 
                      getLocalBounds().removeFromTop(50), 
                      juce::Justification::centred);
        }
        
        void resized() override
        {
            auto bounds = getLocalBounds();
            bounds.removeFromTop(60);
            bounds = bounds.reduced(20);
            
            if (slot)
                slot->setBounds(bounds.removeFromLeft(bounds.getWidth() / 2).reduced(10));
        }
        
        void timerCallback()
        {
            // Test engines
            int testEngines[] = {0, 18, 1, 2, 15, 22, 3};  // Empty, BitCrusher, Opto, Classic, etc
            
            int engineId = testEngines[testEngineIndex % 7];
            testEngineIndex++;
            
            if (engineId == 0) {
                std::cout << "Testing: Empty slot\\n";
                slot->update(nullptr, 0);
            } else {
                auto engine = EngineFactory::createEngine(engineId);
                if (engine) {
                    std::cout << "Testing: " << engine->getName().toStdString() 
                             << " (" << engine->getNumParameters() << " params)\\n";
                    slot->update(engine.get(), engineId);
                }
            }
        }
        
    private:
        std::unique_ptr<NexusLookAndFeel> nexusLook;
        std::unique_ptr<SlotComponent> slot;
        int testEngineIndex;
        
        class ComponentTimer : public juce::Timer {
            MainComponent* parent;
        public:
            void setParent(MainComponent* p) { parent = p; }
            void timerCallback() override { 
                if (parent) parent->onTimerTick(); 
            }
        } timer;
        
        void startTimer(int ms) { timer.setParent(this); timer.startTimer(ms); }
        void onTimerTick() { timerCallback(); }
    };
};

class TestApp : public juce::JUCEApplication
{
public:
    TestApp() {}
    
    const juce::String getApplicationName() override { return "UI Visual Test"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    
    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<TestWindow>("Nexus UI Visual Test");
    }
    
    void shutdown() override
    {
        mainWindow = nullptr;
    }
    
private:
    std::unique_ptr<TestWindow> mainWindow;
};

START_JUCE_APPLICATION(TestApp)