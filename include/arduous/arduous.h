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

typedef struct {
    bool buttonUp = false;
    bool buttonRight = false;
    bool buttonLeft = false;
    bool buttonDown = false;
    bool buttonA = false;
    bool buttonB = false;
} ArduousButtonState;

class Arduous {
   public:
    Arduous();
    Arduous(const Arduous&) = delete;
    Arduous(Arduous&&) = delete;
    Arduous& operator=(const Arduous&) = delete;
    Arduous& operator=(Arduous&&) = delete;
    ~Arduous() = default;

    void loadFirmware(std::string path);
    void loadHex(std::string hexString);
    void loadHexFile(std::string path);
    void init(uint8_t* boot, uint32_t bootSize, uint32_t bootBase);
    void reset();
    void emulateFrame();
    void update(int steps = 1);
    void setButtonState(ArduousButtonState newButtonState);

    std::bitset<DISPLAY_WIDTH * DISPLAY_HEIGHT> getFrameBuffer();

   private:
    // Atcore cpu;
    avr_t* cpu;
    // SSD1306 screen;
    ssd1306_t screen;

    std::string mmcu = "atmega32u4";
    size_t freq = 16000000;
    size_t cpuTicksPerFrame;
    ArduousButtonState buttonState = {};
};

#endif
