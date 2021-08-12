#ifndef ARDUOUS_SSD1306_H
#define ARDUOUS_SSD1306_H

#include <array>
#include <bitset>
#include <map>
#include <vector>

constexpr int DISPLAY_WIDTH = 128;
constexpr int DISPLAY_HEIGHT = 64;
constexpr int DISPLAY_PIXELS = DISPLAY_WIDTH * DISPLAY_HEIGHT;
constexpr uint8_t PAGE_SIZE = 8;

enum AddressingMode : uint8_t {
    horizontalMode = 0x00,
    verticalMode = 0x01,
    pageMode = 0x02,
};

class SSD1306 {
   public:
    SSD1306();

    void pushCommand(uint8_t data);
    void pushData(uint8_t data);
    void tick();
    uint8_t getContrast();
    std::bitset<DISPLAY_PIXELS> getFrameBuffer();

   private:
    std::bitset<DISPLAY_PIXELS> ram;

    // registers
    uint8_t page = 0;
    uint8_t col = 0;
    uint8_t contrast = 0x7F;
    bool sleeping = false;
    bool ignoreRam = false;
    bool inverted = false;
    AddressingMode addressingMode = pageMode;
    uint8_t columnStart = 0;
    uint8_t columnEnd = 127;
    uint8_t pageStart = 0;
    uint8_t pageEnd = 7;
    uint8_t pageModeColumnStart = 0;
    uint8_t pageModePageStart = 0;
    uint8_t displayStartLine = 0;
    bool segmentRemap = false;
    uint8_t multiplexRatio = 63;
    bool comRemap = false;
    uint8_t verticalShift = 0;
    uint8_t comPinsConfiguration = 0x01;
    uint8_t divideRatio = 0;
    uint8_t oscillatorFrequency = 0x08;
    uint8_t prechargePeriod = 0x22;
    uint8_t vcomhDeselectLevel = 0x20;

    std::vector<uint8_t> commandBuffer{};

    void setPixel(uint8_t x, uint8_t y, bool value);

    void dispatchCommand(uint8_t command);

    // Fundamental commands
    void setContrastControl();
    void entireDisplayOn();
    void setNormalInverseDisplay();
    void setDisplayOnOff();

    // Scrolling commands
    // void continuousHorizontalScrollSetup();
    // void continuousVerticalAndHorizontalScrollSetup();
    // void deactivateScroll();
    // void activateScroll();
    // void setVerticalScrollArea();

    // Addressing Setting commands
    void setLowerColumnStartAddressForPageAddressingMode();
    void setHigherColumnStartAddressForPageAddressingMode();
    void setMemoryAddressingMode();
    void setColumnAddress();
    void setPageAddress();
    void setPageStartAddressForPageAddressingMode();

    // Hardware Configuration commands
    void setDisplayStartLine();
    void setSegmentRemap();
    void setMultiplexRatio();
    void setComOutputScanDirection();
    void setDisplayOffset();
    void setComPinsHardwareConfiguration();

    // Timing & Driving Scheme Setting commands
    void setDisplayClockDivideRatioOscillatorFrequency();
    void setPrechargePeriod();
    void setVcomhDeselectLevel();
    void nop();
};

/// Map of multi-byte commands -> number of extra bytes consumed
const std::map<uint8_t, uint8_t> COMMAND_EXTRA_BYTES{
    {0x81, 1}, {0x26, 6}, {0x27, 6}, {0x27, 6}, {0x29, 6}, {0x2A, 6}, {0xA3, 2}, {0x20, 1},
    {0x21, 2}, {0x22, 2}, {0xA8, 1}, {0xD3, 1}, {0xDA, 1}, {0xD5, 1}, {0xD9, 1}, {0xDB, 1},
};

#endif
