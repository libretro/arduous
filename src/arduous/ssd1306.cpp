#include "arduous/ssd1306.h"

#include <bitset>
#include <iostream>

#include "stdext/map.h"

SSD1306::SSD1306() = default;

void SSD1306::pushCommand(uint8_t data) {
    commandBuffer.push_back(data);
    uint8_t command = commandBuffer.front();

    if ((uint8_t)(commandBuffer.size()) > get_default(COMMAND_EXTRA_BYTES, command, (uint8_t)0)) {
        dispatchCommand(command);
        commandBuffer.clear();
    }
}

void SSD1306::pushData(uint8_t data) {
    int x = col;

    for (int y = PAGE_SIZE * (page + 1) - 1, i = 0; i < PAGE_SIZE; i++, y--) {
        setPixel(x, y, (data >> i) & 0x01);
    }

    switch (addressingMode) {
        case pageMode:
            col++;
            if (col > DISPLAY_WIDTH - 1) {
                col = pageModeColumnStart;
            }
            break;
        case horizontalMode:
            col++;
            if (col > columnEnd) {
                col = columnStart;
                page++;
                if (page > pageEnd) {
                    page = pageStart;
                }
            }
            break;
        case verticalMode:
            page++;
            if (page > pageEnd) {
                page = pageStart;
                col++;
                if (col > columnEnd) {
                    col = columnStart;
                }
            }
            break;
    }
}

void SSD1306::tick() {}

uint8_t SSD1306::getContrast() { return contrast; }

std::bitset<DISPLAY_PIXELS> SSD1306::getFrameBuffer() {
    if (sleeping) {
        return std::bitset<DISPLAY_PIXELS>();
    }
    if (ignoreRam) {
        return std::bitset<DISPLAY_PIXELS>().set();
    }
    if (inverted) {
        return ~ram;
    }
    return ram;
}

void SSD1306::setPixel(uint8_t x, uint8_t y, bool value) { ram[y * DISPLAY_WIDTH + x] = value; }

void SSD1306::dispatchCommand(uint8_t command) {
    switch (command) {
        case 0x81:
            setContrastControl();
            break;
        case 0xA4:
        case 0xA5:
            entireDisplayOn();
            break;
        case 0xA6:
        case 0xA7:
            setNormalInverseDisplay();
            break;
        case 0xAE:
        case 0xAF:
            setDisplayOnOff();
        case 0x26:
        case 0x27:
        case 0x29:
        case 0x2A:
        case 0x2E:
        case 0x2F:
        case 0xA3:
            // TODO(jmaroeder): implement scrolling commands
            break;
        case 0x20:
            setMemoryAddressingMode();
            break;
        case 0x21:
            setColumnAddress();
            break;
        case 0x22:
            setPageAddress();
            break;
        case 0xA0:
        case 0xA1:
            setSegmentRemap();
            break;
        case 0xA8:
            setMultiplexRatio();
            break;
        case 0xC0:
        case 0xC8:
            setComOutputScanDirection();
            break;
        case 0xD3:
            setDisplayOffset();
            break;
        case 0xDA:
            setComPinsHardwareConfiguration();
            break;
        case 0xD5:
            setDisplayClockDivideRatioOscillatorFrequency();
            break;
        case 0xD9:
            setPrechargePeriod();
            break;
        case 0xDB:
            setVcomhDeselectLevel();
            break;
        case 0xE3:
            nop();
            break;
        default:
            if (0x00 <= command && command <= 0x0F) {
                setLowerColumnStartAddressForPageAddressingMode();
            } else if (0x10 <= command && command <= 0x1F) {
                setHigherColumnStartAddressForPageAddressingMode();
            } else if (0xB0 <= command && command <= 0xB7) {
                setPageStartAddressForPageAddressingMode();
            } else if (0x40 <= command && command <= 0x7F) {
                setDisplayStartLine();
            } else {
                std::cerr << "Unknown command: " << command;
            }
    }
}

void SSD1306::setContrastControl() { contrast = commandBuffer[1]; }

void SSD1306::entireDisplayOn() { ignoreRam = commandBuffer[0] & 0x01; }

void SSD1306::setNormalInverseDisplay() { inverted = commandBuffer[0] & 0x01; }

void SSD1306::setDisplayOnOff() { sleeping = commandBuffer[0] & 0x01; }

void SSD1306::setLowerColumnStartAddressForPageAddressingMode() {
    col = pageModeColumnStart = (pageModeColumnStart & 0xF0) | (commandBuffer[0] & 0x0F);
}

void SSD1306::setHigherColumnStartAddressForPageAddressingMode() {
    col = pageModeColumnStart = ((commandBuffer[0] & 0x0F) << 4) | (pageModeColumnStart & 0x0F);
}

void SSD1306::setMemoryAddressingMode() {
    uint8_t a = commandBuffer[0] & 0x03;
    if (a != horizontalMode && a != verticalMode && a != pageMode) {
        std::cerr << "Invalid addressing mode: " << a;
        return;
    }
    addressingMode = static_cast<AddressingMode>(a);
}

void SSD1306::setColumnAddress() {
    col = columnStart = commandBuffer[1] & 0x7F;
    columnEnd = commandBuffer[2] & 0x7F;
}

void SSD1306::setPageAddress() {
    page = pageStart = commandBuffer[1] & 0x07;
    pageEnd = commandBuffer[2] & 0x07;
}

void SSD1306::setPageStartAddressForPageAddressingMode() { page = pageModePageStart = commandBuffer[0] & 0x07; }

void SSD1306::setDisplayStartLine() { displayStartLine = commandBuffer[0] & 0x3F; }

void SSD1306::setSegmentRemap() { segmentRemap = commandBuffer[0] & 0x01; }

void SSD1306::setMultiplexRatio() { multiplexRatio = commandBuffer[0] & 0x3F; }

void SSD1306::setComOutputScanDirection() { comRemap = commandBuffer[0] & 0x08; }

void SSD1306::setDisplayOffset() { verticalShift = commandBuffer[1] & 0x3F; }

void SSD1306::setComPinsHardwareConfiguration() { comPinsConfiguration = (commandBuffer[1] & 0x30) >> 4; }

void SSD1306::setDisplayClockDivideRatioOscillatorFrequency() {
    divideRatio = commandBuffer[1] & 0x0F;
    oscillatorFrequency = (commandBuffer[1] & 0xF0) >> 4;
}

void SSD1306::setPrechargePeriod() { prechargePeriod = commandBuffer[1]; }

void SSD1306::setVcomhDeselectLevel() { vcomhDeselectLevel = commandBuffer[1]; }

void SSD1306::nop() {}
