#ifndef ARDUOUS_ARDUOUS_H
#define ARDUOUS_ARDUOUS_H

#include <bitset>
#include <cstdint>
#include <cstdio>

#include "arduous/atcore.h"
#include "arduous/ssd1306.h"

constexpr int TIMING_FPS = 60;

class Arduous {
   public:
    Arduous();
    Arduous(const Arduous&) = delete;
    Arduous(Arduous&&) = delete;
    Arduous& operator=(const Arduous&) = delete;
    Arduous& operator=(Arduous&&) = delete;
    ~Arduous() = default;

    void loadHex(const uint8_t* data, size_t size);
    void loadBinaryRom(const uint8_t* data, size_t size);
    void reset();
    void emulateFrame();
    void update(int steps = 1);

    std::bitset<DISPLAY_WIDTH * DISPLAY_HEIGHT> getFrameBuffer();

   private:
    Atcore cpu;
    SSD1306 screen;

    size_t cpuTicksPerFrame;
};

#endif
