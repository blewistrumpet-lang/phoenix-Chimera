// Simple crash test - simulate what happens when plugin loads
#include <iostream>
#include <vector>
#include <array>

// Simulate the critical parts that might crash
class MockSlotComponent {
public:
    MockSlotComponent(int slotIndex) : slotNumber(slotIndex) {
        std::cout << "Creating SlotComponent " << slotIndex << std::endl;
        
        // This is what happens in the constructor
        visibleParamCount = 0;
        currentEngineId = -1;
        
        // Initialize arrays
        for (int i = 0; i < 15; ++i) {
            controlTypes[i] = 0; // CONTROL_ROTARY
            std::cout << "  Initialized control " << i << std::endl;
        }
    }
    
    void update(void* engine, int engineId) {
        std::cout << "Update called with engineId=" << engineId << std::endl;
        if (!engine) {
            std::cout << "  Engine is null - hiding controls" << std::endl;
            visibleParamCount = 0;
            return;
        }
        visibleParamCount = 8; // Simulate some params
    }
    
private:
    int slotNumber;
    int visibleParamCount;
    int currentEngineId;
    std::array<int, 15> controlTypes;
};

class MockEditor {
public:
    MockEditor() {
        std::cout << "Creating PluginEditorNexusStatic..." << std::endl;
        
        // Create 6 slots
        for (int i = 0; i < 6; ++i) {
            std::cout << "Creating slot " << i << "..." << std::endl;
            slots[i] = new MockSlotComponent(i);
            
            // This is what initializeSlot does
            std::cout << "Initializing slot " << i << "..." << std::endl;
            
            // This is what happens during initial update
            std::cout << "Initial update for slot " << i << "..." << std::endl;
            updateSlotEngine(i);
        }
        
        std::cout << "Editor created successfully!" << std::endl;
    }
    
    ~MockEditor() {
        for (int i = 0; i < 6; ++i) {
            delete slots[i];
        }
    }
    
    void updateSlotEngine(int slotIndex) {
        if (slotIndex < 0 || slotIndex >= 6) return;
        if (!slots[slotIndex]) {
            std::cerr << "ERROR: Slot " << slotIndex << " is null!" << std::endl;
            return;
        }
        
        // Simulate getting engine - might be null initially
        void* engine = nullptr; // Simulating no engine initially
        int engineId = 0;
        
        slots[slotIndex]->update(engine, engineId);
    }
    
private:
    MockSlotComponent* slots[6];
};

int main() {
    try {
        std::cout << "=== Starting crash test ===" << std::endl;
        
        MockEditor editor;
        
        std::cout << "=== Test completed successfully ===" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "CRASH: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "CRASH: Unknown error" << std::endl;
        return 1;
    }
    
    return 0;
}