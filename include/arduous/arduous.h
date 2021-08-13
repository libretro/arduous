#ifndef ARDUOUS_ARDUOUS_H
#define ARDUOUS_ARDUOUS_H

#include <bitset>
#include <cstdint>
#include <cstdio>

#include "sim_avr.h"
#include "ssd1306_virt.h"

constexpr int TIMING_FPS = 60;
constexpr int DISPLAY_WIDTH = 128;
constexpr int DISPLAY_HEIGHT = 64;

class Arduous {
   public:
    Arduous();
    Arduous(const Arduous&) = delete;
    Arduous(Arduous&&) = delete;
    Arduous& operator=(const Arduous&) = delete;
    Arduous& operator=(Arduous&&) = delete;
    ~Arduous() = default;

    void loadFirmware(std::string path);
    // void loadHex(const uint8_t* data, size_t size);
    // void loadBinaryRom(const uint8_t* data, size_t size);
    void reset();
    void emulateFrame();
    void update(int steps = 1);

    std::bitset<DISPLAY_WIDTH * DISPLAY_HEIGHT> getFrameBuffer();

   private:
    // Atcore cpu;
    avr_t* cpu;
    // SSD1306 screen;
    ssd1306_t screen;

    size_t cpuTicksPerFrame;
};

#endif
