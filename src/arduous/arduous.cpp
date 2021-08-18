#include "arduous/arduous.h"

#include <stdio.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>

#include "avr_ioport.h"
#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_hex.h"
#include "ssd1306_virt.h"

void noOpSleep(avr_t* avr, avr_cycle_count_t how_long) {}

Arduous::Arduous() = default;

// void Arduous::loadFirmware(std::string path) {
//     elf_firmware_t firmware;
//     elf_read_firmware(path.c_str(), &firmware);
//     fprintf(stderr, "Using mcu %s at frequency %d", firmware.mmcu, firmware.frequency);
//     if (strlen(firmware.mmcu) > 0) {
//         mmcu = firmware.mmcu;
//     }
// }

void Arduous::loadHexFile(std::string path) {
    uint32_t bootSize;
    uint32_t bootBase;
    uint8_t* boot = read_ihex_file(path.c_str(), &bootSize, &bootBase);
    if (!boot) {
        fprintf(stderr, "Unable to load %s\n", path.c_str());
        exit(1);
    }

    init(boot, bootBase, bootSize);
    free(boot);
}

void Arduous::init(uint8_t* boot, uint32_t bootBase, uint32_t bootSize) {
    cpu = avr_make_mcu_by_name(mmcu.c_str());

    fprintf(stderr, "%s booloader 0x%05x: %d bytes\n", mmcu.c_str(), bootBase, bootSize);

    avr_init(cpu);

    memcpy(cpu->flash + bootBase, boot, bootSize);
    cpu->frequency = freq;
    cpu->sleep = noOpSleep;
    cpu->pc = bootBase;
    cpu->codeend = cpu->flashend;

    ssd1306_init(cpu, &screen, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    ssd1306_wiring_t wiring = {
        .chip_select.port = 'D',
        .chip_select.pin = 6,
        .data_instruction.port = 'D',
        .data_instruction.pin = 4,
        .reset.port = 'D',
        .reset.pin = 7,
    };
    ssd1306_connect(&screen, &wiring);

    pinCallbackParamTs = {PinCallbackParamT{.self = this, .speakerPin = 0},
                          PinCallbackParamT{.self = this, .speakerPin = 1}};

    avr_irq_register_notify(avr_io_getirq(cpu, AVR_IOCTL_IOPORT_GETIRQ('C'), 6), Arduous::soundPinCallback,
                            &pinCallbackParamTs[0]);
    avr_irq_register_notify(avr_io_getirq(cpu, AVR_IOCTL_IOPORT_GETIRQ('C'), 7), Arduous::soundPinCallback,
                            &pinCallbackParamTs[1]);

    cyclesPerVideoFrame = freq / TIMING_FPS;
    cyclesPerAudioSample = freq / TIMING_SAMPLE_RATE;
    audioSamplesPerVideoFrame = TIMING_SAMPLE_RATE / TIMING_FPS;
    audioBuffer.reserve(audioSamplesPerVideoFrame * 2);
}

void Arduous::reset() {
    // avr_load_firmware(cpu, &firmware);
}

void Arduous::emulateFrame() {
    frameStartCycle = cpu->cycle;
    frameEndCycle = frameStartCycle + cyclesPerVideoFrame;
    audioBuffer.clear();

    while (cpu->cycle < frameEndCycle) {
        int state = avr_run(cpu);
        switch (state) {
            case cpu_Done:
                // TODO(jmaroeder): exit cleanly
                std::cerr << "cpu state is done\n";
                break;
            case cpu_Crashed:
                // TODO(jmaroeder): exit cleanly
                std::cerr << "cpu state is crashed\n";
                break;
            default:
                break;
        }
    }
}

void Arduous::update(int steps) {
    for (int i = 0; i < steps; i++) {
        emulateFrame();
    }
}

#define PINB 0x23
#define PINE 0x2c
#define PINF 0x2f

void Arduous::setButtonState(ArduousButtonState newButtonState) {
    cpu->data[PINB] = (!newButtonState.buttonB << 4) | (cpu->data[PINB] & 0xEF);
    cpu->data[PINE] = (!newButtonState.buttonA << 6) | (cpu->data[PINE] & 0xBF);
    cpu->data[PINF] = (!newButtonState.buttonUp << 7) | (!newButtonState.buttonRight << 6) |
                      (!newButtonState.buttonLeft << 5) | (!newButtonState.buttonDown << 4) | (cpu->data[PINF] & 0x0F);
}

std::bitset<DISPLAY_WIDTH * DISPLAY_HEIGHT> Arduous::getVideoFrameBuffer() {
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

std::vector<int16_t> Arduous::getAudioBuffer() {
    if (audioBuffer.size() < audioSamplesPerVideoFrame * 2) {
        extendAudioBuffer();
    }
    if (audioBuffer.size() != audioSamplesPerVideoFrame * 2) {
        fprintf(stderr, "audioBuffer.size() == %d, expected %d", audioBuffer.size(), audioSamplesPerVideoFrame * 2);
    }
    return audioBuffer;
}

int16_t Arduous::getCurrentSpeakerSample() { return (speakerPins[0] + speakerPins[1]) << 14; }

void Arduous::extendAudioBuffer() {
    // multiply by 2 to get interleaved stereo
    int currentSampleIndex = 2 * std::min((cpu->cycle - frameStartCycle) / cyclesPerAudioSample,
                                          static_cast<uint64_t>(audioSamplesPerVideoFrame));
    uint16_t currentSample = getCurrentSpeakerSample();
    for (uint i = audioBuffer.size(); i < currentSampleIndex; i++) {
        audioBuffer.push_back(currentSample);
    }
}

void Arduous::soundPinCallback(struct avr_irq_t* irq, uint32_t value, void* param) {
    auto* pinCallbackParamT = static_cast<PinCallbackParamT*>(param);
    Arduous* self = pinCallbackParamT->self;
    self->extendAudioBuffer();
    self->speakerPins[pinCallbackParamT->speakerPin] = value & 0x1;

    // fprintf(stderr, "[%lld]: [%d]: %d\n", self->cpu->cycle, pinCallbackParamT->speakerPin, value & 0x1);
}
