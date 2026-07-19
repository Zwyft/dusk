#include <random>
#include <cstdint>

// Simple randomizer utility for Dusklight
namespace Randomizer {
    // Returns a random integer in [0, max)
    inline int getInt(int max) {
        static std::mt19937 engine{ std::random_device{}() };
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(engine);
    }

    // Returns a random float in [0.0f, 1.0f)
    inline float getFloat() {
        static std::mt19937 engine{ std::random_device{}() };
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(engine);
    }
}
