#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/PluginEditorNexusStatic.h"

class TestWindow : public juce::DocumentWindow
{
public:
    TestWindow() : DocumentWindow("Chimera Phoenix UI Analysis", 
                                   juce::Colours::black,
                                   DocumentWindow::allButtons)
    {
        processor = std::make_unique<ChimeraAudioProcessor>();
        editor = std::make_unique<PluginEditorNexusStatic>(*processor);
        
        setContentOwned(editor.release(), true);
        setResizable(false, false);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
        
        // Load different engines for testing
        startTimer(3000);
        engineIndex = 0;
    }
    
    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    
    void timerTick()
    {
        // Cycle through different engines to see UI issues
        int testEngines[] = {18, 20, 2, 52, 45, 15};  // BitCrusher, WaveFolder, Compressor, Harmonizer, DynamicEQ, VintagePreamp
        
        if (engineIndex < 6) {
            int engineId = testEngines[engineIndex];
            
            // Set slot 1 to this engine
            if (auto* param = processor->getValueTreeState().getParameter("slot1_engine")) {
                param->setValueNotifyingHost(engineId / 56.0f);
                
                std::cout << "\n=== Loading Engine " << engineId << " ===" << std::endl;
            }
            
            engineIndex++;
        }
    }
    
private:
    std::unique_ptr<ChimeraAudioProcessor> processor;
    std::unique_ptr<PluginEditorNexusStatic> editor;
    int engineIndex;
    
    class UITimer : public juce::Timer {
        TestWindow* parent;
    public:
        void setParent(TestWindow* p) { parent = p; }
        void timerCallback() override { if (parent) parent->timerTick(); }
    } timer;
    
    void startTimer(int ms) { timer.setParent(this); timer.startTimer(ms); }
};

class TestApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "UI Analysis"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    
    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<TestWindow>();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "CHIMERA PHOENIX UI ANALYSIS" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\nObserving UI for issues..." << std::endl;
        std::cout << "Window should be open. Testing engines:" << std::endl;
        std::cout << "- BitCrusher (3 params)" << std::endl;
        std::cout << "- Wave Folder (4 params)" << std::endl;
        std::cout << "- Classic Compressor (10 params)" << std::endl;
        std::cout << "- Intelligent Harmonizer (stepped encoders)" << std::endl;
        std::cout << "- Dynamic EQ (time sliders)" << std::endl;
        std::cout << "- Vintage Preamp (14 params)" << std::endl;
    }
    
    void shutdown() override
    {
        mainWindow = nullptr;
    }
    
private:
    std::unique_ptr<TestWindow> mainWindow;
};

START_JUCE_APPLICATION(TestApp)