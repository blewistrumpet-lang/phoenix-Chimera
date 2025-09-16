// Debug the Chaos Generator to see actual values
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// Simplified Lorenz system test
struct SimpleLorenz {
    double x = 0.1, y = 0.0, z = 0.0;
    const double sigma = 10.0;
    const double rho = 28.0;
    const double beta = 8.0 / 3.0;
    
    float iterate(double dt) {
        double dx = sigma * (y - x);
        double dy = x * (rho - z) - y;
        double dz = x * y - beta * z;
        
        x += dx * dt;
        y += dy * dt;
        z += dz * dt;
        
        return static_cast<float>(x); // Raw value, not tanh
    }
    
    float iterateWithTanh(double dt) {
        iterate(dt);
        return static_cast<float>(std::tanh(x / 30.0));
    }
};

int main() {
    std::cout << "Testing Lorenz System Output\n";
    std::cout << "=============================\n\n";
    
    SimpleLorenz lorenz1, lorenz2;
    
    std::cout << "Iteration | Raw X Value | tanh(x/30) Output\n";
    std::cout << "----------|-------------|------------------\n";
    
    for (int i = 0; i < 1000; ++i) {
        float raw = lorenz1.iterate(0.01);
        float withTanh = lorenz2.iterateWithTanh(0.01);
        
        if (i % 50 == 0) { // Print every 50th iteration
            std::cout << std::setw(9) << i << " | ";
            std::cout << std::setw(11) << std::fixed << std::setprecision(3) << raw << " | ";
            std::cout << std::setw(17) << std::fixed << std::setprecision(6) << withTanh << "\n";
        }
    }
    
    std::cout << "\nTesting modulation effect:\n";
    std::cout << "Input signal = 0.5\n";
    
    SimpleLorenz lorenz3;
    float totalChange = 0;
    
    for (int i = 0; i < 100; ++i) {
        float chaos = lorenz3.iterateWithTanh(0.01);
        float depth = 1.0f; // Max depth
        float scaledChaos = chaos * depth * 2.0f; // As in the fixed code
        
        // Amplitude modulation as in ChaosGenerator
        float input = 0.5f;
        float gain = 1.0f + scaledChaos * 2.0f;
        gain = std::max(0.0f, gain);
        float output = input * gain;
        
        float change = std::abs(output - input);
        totalChange += change;
        
        if (i % 20 == 0) {
            std::cout << "  Iter " << i << ": chaos=" << chaos 
                      << ", gain=" << gain << ", output=" << output 
                      << ", change=" << change << "\n";
        }
    }
    
    std::cout << "\nTotal change over 100 iterations: " << totalChange << "\n";
    
    if (totalChange < 0.1f) {
        std::cout << "❌ Effect is too subtle!\n";
    } else {
        std::cout << "✅ Effect should be noticeable\n";
    }
    
    return 0;
}