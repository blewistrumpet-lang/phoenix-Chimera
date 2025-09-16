// Test Step 1: Bare minimum editor
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/TestEditorIncremental.h"
#include <iostream>

class TestApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Test Step 1"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        std::cout << "\n=== STEP 1: BARE MINIMUM EDITOR TEST ===" << std::endl;
        
        try {
            std::cout << "1. Creating processor..." << std::endl;
            processor = std::make_unique<ChimeraAudioProcessor>();
            
            std::cout << "2. Preparing processor..." << std::endl;
            processor->prepareToPlay(44100.0, 512);
            
            std::cout << "3. Creating MINIMAL editor..." << std::endl;
            auto* editor = new TestEditorIncremental(*processor);
            
            std::cout << "4. Creating window..." << std::endl;
            mainWindow = std::make_unique<juce::DocumentWindow>(
                "Test Editor", 
                juce::Colours::black,
                juce::DocumentWindow::allButtons
            );
            
            mainWindow->setContentOwned(editor, true);
            mainWindow->setResizable(false, false);
            mainWindow->centreWithSize(editor->getWidth(), editor->getHeight());
            mainWindow->setVisible(true);
            
            std::cout << "\n✓✓✓ STEP 1 PASSED: Minimal editor created successfully!" << std::endl;
            std::cout << "Window will close in 3 seconds..." << std::endl;
            
            juce::Timer::callAfterDelay(3000, [this]() {
                std::cout << "Test completed - closing." << std::endl;
                quit();
            });
        }
        catch (const std::exception& e) {
            std::cerr << "✗✗✗ STEP 1 FAILED: " << e.what() << std::endl;
            quit();
        }
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        processor = nullptr;
    }

private:
    std::unique_ptr<ChimeraAudioProcessor> processor;
    std::unique_ptr<juce::DocumentWindow> mainWindow;
};

START_JUCE_APPLICATION(TestApp)