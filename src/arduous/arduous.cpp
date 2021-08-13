#include "arduous/arduous.h"

#include <stdio.h>

#include <iostream>
#include <string>

#include "sim_avr.h"
#include "sim_elf.h"
#include "ssd1306_virt.h"

Arduous::Arduous() = default;

void Arduous::loadFirmware(std::string path) {
    elf_firmware_t firmware;
    elf_read_firmware(path.c_str(), &firmware);
    fprintf(stderr, "Using mcu %s at frequency %d", firmware.mmcu, firmware.frequency);
    cpu = avr_make_mcu_by_name(firmware.mmcu);
    avr_init(cpu);
    // screen = SSD1306();
    ssd1306_init(cpu, &screen, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    ssd1306_wiring_t wiring = {
        .chip_select.port = 'B',
        .data_instruction.port = 'B',
        .data_instruction.pin = 1,
        .reset.port = 'B',
        .reset.pin = 3,
    };
    ssd1306_connect(&screen, &wiring);

    cpuTicksPerFrame = firmware.frequency * 10 / TIMING_FPS;
    avr_load_firmware(cpu, &firmware);
}

void Arduous::emulateFrame() {
    for (int i = 0; i < cpuTicksPerFrame; i++) {
        avr_run(cpu);
    }
}

void Arduous::update(int steps) {
    for (int i = 0; i < steps; i++) {
        emulateFrame();
    }
}

std::bitset<DISPLAY_WIDTH * DISPLAY_HEIGHT> Arduous::getFrameBuffer() {
    std::bitset<DISPLAY_WIDTH * DISPLAY_HEIGHT> fb;

    for (int p = 0; p < screen.pages; p++) {
        for (int c = 0; c < screen.columns; c++) {
            uint8_t vram_byte = screen.vram[p][c];

            for (int i = 0; i < 8; i++) {
                if (vram_byte & (1 << i)) {
                    fb[(p * 8 + i) * DISPLAY_WIDTH + c] = true;
                }
            }
        }
    }

    return fb;
}
