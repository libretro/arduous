#include "arduous/arduous.h"

#include "arduous/atcore.h"
#include "arduous/ssd1306.h"

std::vector<uint8_t> SCREEN_TEST_COMMANDS = {
    0x20, 0x00  // horizontal addressing mode
};

std::vector<std::vector<uint8_t>> SCREEN_TEST_DATA = {
    {0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF},
};

size_t SCREEN_TEST_DATA_PTR = 0;

Arduous::Arduous() {
    cpu = Atcore();
    screen = SSD1306();
    for (uint8_t command : SCREEN_TEST_COMMANDS) {
        screen.pushCommand(command);
    }
    cpuTicksPerFrame = cpu.getDesc().clock / TIMING_FPS;
}

void Arduous::emulateFrame() {
    for (int i = 0; i < cpuTicksPerFrame; i++) {
        cpu.tick();
    }

    for (uint8_t data : SCREEN_TEST_DATA[SCREEN_TEST_DATA_PTR]) {
        screen.pushData(data);
    }
    SCREEN_TEST_DATA_PTR++;
    SCREEN_TEST_DATA_PTR %= SCREEN_TEST_DATA.size();

    screen.tick();
}

void Arduous::update(int steps) {
    for (int i = 0; i < steps; i++) {
        emulateFrame();
    }
}

std::bitset<DISPLAY_WIDTH * DISPLAY_HEIGHT> Arduous::getFrameBuffer() { return screen.getFrameBuffer(); }
