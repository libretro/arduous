#include "arduous/arduous.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

#include "avr_ioport.h"
#include "avr_eeprom.h"
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

    // decode line text hex to binary
static int read_hex_string_buf(const char * src, const char *end, uint8_t * buffer, int maxlen)
{
    uint8_t * dst = buffer;
    int ls = 0;
    uint8_t b = 0;
    while (src < end && maxlen) {
        char c = *src++;
        switch (c) {
            case 'a' ... 'f':   b = (b << 4) | (c - 'a' + 0xa); break;
            case 'A' ... 'F':   b = (b << 4) | (c - 'A' + 0xa); break;
            case '0' ... '9':   b = (b << 4) | (c - '0'); break;
            default:
                if (c > ' ') {
                    fprintf(stderr, "%s: huh '%c' (%s)\n", __FUNCTION__, c, src);
                    return -1;
                }
                continue;
        }
        if (ls & 1) {
            *dst++ = b; b = 0;
            maxlen--;
        }
        ls++;
    }

    return dst - buffer;
}

static int
read_ihex_chunks_buffer(const char *buf, const char *end,
		ihex_chunk_p * chunks )
{
    if (!buf || !chunks)
	return -1;
    uint32_t segment = 0;	// segment address
    int chunk = 0, max_chunks = 0;
    const char *nextlineptr = buf;
    *chunks = NULL;

    while (nextlineptr < end) {
	const char *lineptr, *lineend;
	lineptr = nextlineptr;

	// Find end of the line
	for (lineend = lineptr;
	     lineend < end && !isspace(*lineend);
	     lineend++);

	// Find beginning of the next line. Skip all the whitespaces 
	for (nextlineptr = lineend;
	     nextlineptr < end && isspace(*nextlineptr);
	     nextlineptr++);
	    
	if (lineptr[0] != ':') {
	    fprintf(stderr, "AVR: invalid ihex format (%.4s)\n", lineptr);
	    break;
	}
	uint8_t bline[64];

	int len = read_hex_string_buf(lineptr + 1, lineend,
				      bline, sizeof(bline));
	if (len <= 0)
	    continue;

	uint8_t chk = 0;
	{	// calculate checksum
	    uint8_t * src = bline;
	    int tlen = len-1;
	    while (tlen--)
		chk += *src++;
	    chk = 0x100 - chk;
	}
	if (chk != bline[len-1]) {
	    fprintf(stderr, "%s: invalid checksum %02x/%02x\n", __FUNCTION__, chk, bline[len-1]);
	    break;
	}
	uint32_t addr = 0;
	switch (bline[3]) {
	case 0: // normal data
	    addr = segment | (bline[1] << 8) | bline[2];
	    break;
	case 1: // end of file
	    continue;
	case 2: // extended address 2 bytes
	    segment = ((bline[4] << 8) | bline[5]) << 4;
	    continue;
	case 4:
	    segment = ((bline[4] << 8) | bline[5]) << 16;
	    continue;
	default:
	    fprintf(stderr, "%s: unsupported check type %02x\n", __FUNCTION__, bline[3]);
	    continue;
	}
	if (chunk < max_chunks && addr != ((*chunks)[chunk].baseaddr + (*chunks)[chunk].size)) {
	    if ((*chunks)[chunk].size)
		chunk++;
	}
	if (chunk >= max_chunks) {
	    max_chunks++;
	    /* Here we allocate and zero an extra chunk, to act as terminator */
	    *chunks = (ihex_chunk_p)realloc(*chunks, (1 + max_chunks) * sizeof(ihex_chunk_t));
	    memset(*chunks + chunk, 0,
		   (1 + (max_chunks - chunk)) * sizeof(ihex_chunk_t));
	    (*chunks)[chunk].baseaddr = addr;
	}
	(*chunks)[chunk].data = (uint8_t *) realloc((*chunks)[chunk].data,
					(*chunks)[chunk].size + bline[0]);
	memcpy((*chunks)[chunk].data + (*chunks)[chunk].size,
	       bline + 4, bline[0]);
	(*chunks)[chunk].size += bline[0];
    }
    return max_chunks;
}


static uint8_t *
read_ihex_buffer(const char *data, size_t sz, uint32_t * dsize, uint32_t * start)
{
	ihex_chunk_p chunks = NULL;
	int count = read_ihex_chunks_buffer(data, data + sz, &chunks);
	uint8_t * res = NULL;

	if (count > 0) {
		*dsize = chunks[0].size;
		*start = chunks[0].baseaddr;
		res = chunks[0].data;
		chunks[0].data = NULL;
	}
	if (count > 1) {
		fprintf(stderr, "AVR: ihex contains more chunks than loaded (%d)\n",
				count);
	}
	free_ihex_chunks(chunks);
	return res;
}


void Arduous::loadHexBuffer(const char *buf, size_t sz) {
    uint32_t bootSize;
    uint32_t bootBase;
    uint8_t* boot = read_ihex_buffer(buf, sz, &bootSize, &bootBase);
    if (!boot) {
        fprintf(stderr, "Unable to load buffer\n");
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
        .chip_select = {.port = 'D', .pin = 6},
        .data_instruction = {.port = 'D', .pin = 4},
        .reset = {.port = 'D', .pin = 7},
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
    cpu->pc = cpu->reset_pc;
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
    return audioBuffer;
}

size_t Arduous::getSaveSize() {
    size_t size = sizeof(int)                  // cpu->state
                  + sizeof(avr_cycle_count_t)  // cpu->cycle
                  + sizeof(avr_cycle_count_t)  // cpu->run_cycle_count
                  + sizeof(avr_cycle_count_t)  // cpu->run_cycle_limit
                  + sizeof(uint8_t) * 8        // cpu->sreg
                  + sizeof(int8_t)             // cpu->interrupt_state
                  + sizeof(avr_flashaddr_t)    // cpu->pc
                  + sizeof(avr_flashaddr_t)    // cpu->reset_pc
                  + cpu->ramend + 1            // cpu->data

                  + sizeof(ssd1306_virt_cursor_t)                                // screen->cursor
                  + sizeof(uint8_t) * SSD1306_VIRT_PAGES * SSD1306_VIRT_COLUMNS  // screen->vram
                  + sizeof(uint16_t)                                             // screen->flags
                  + sizeof(uint8_t)                                              // screen->command_register
                  + sizeof(uint8_t)                                              // screen->contrast_register
                  + sizeof(uint8_t)                                              // screen->cs_pin
                  + sizeof(uint8_t)                                              // screen->di_pin
                  + sizeof(uint8_t)                                              // screen->spi_data
                  + sizeof(uint8_t)                                              // screen->reg_write_sz
                  + sizeof(ssd1306_addressing_mode_t)                            // screen->addr_mode
                  + sizeof(uint8_t)                                              // screen->twi_selected
                  + sizeof(uint8_t)                                              // screen->twi_index
	          + getEEPROMSize()
	;
    return size;
}

bool Arduous::save(void* data, size_t size) {
    auto* buffer = static_cast<uint8_t*>(data);
    memcpy(buffer, &cpu->state, sizeof(int));
    buffer += sizeof(int);
    memcpy(buffer, &cpu->cycle, sizeof(avr_cycle_count_t));
    buffer += sizeof(avr_cycle_count_t);
    memcpy(buffer, &cpu->run_cycle_count, sizeof(avr_cycle_count_t));
    buffer += sizeof(avr_cycle_count_t);
    memcpy(buffer, &cpu->run_cycle_limit, sizeof(avr_cycle_count_t));
    buffer += sizeof(avr_cycle_count_t);
    memcpy(buffer, cpu->sreg, sizeof(uint8_t) * 8);
    buffer += sizeof(uint8_t) * 8;
    memcpy(buffer, &cpu->interrupt_state, sizeof(int8_t));
    buffer += sizeof(int8_t);
    memcpy(buffer, &cpu->pc, sizeof(avr_flashaddr_t));
    buffer += sizeof(avr_flashaddr_t);
    memcpy(buffer, &cpu->reset_pc, sizeof(avr_flashaddr_t));
    buffer += sizeof(avr_flashaddr_t);
    memcpy(buffer, cpu->data, cpu->ramend + 1);
    buffer += cpu->ramend + 1;
    // TODO(jmaroeder): cpu->cycle_timers
    // for (int i = 0; i < MAX_CYCLE_TIMERS; i++) {
    //     memcpy(buffer, &cpu->cycle_timers.timer_slots[i].when, sizeof(avr_cycle_count_t));
    //     buffer += sizeof(avr_cycle_count_t);
    // }

    // TODO(jmaroeder): cpu->interrupts
    // TODO(jmaroeder): cpu->eeprom?

    memcpy(buffer, &screen.cursor, sizeof(ssd1306_virt_cursor_t));
    buffer += sizeof(ssd1306_virt_cursor_t);
    memcpy(buffer, screen.vram, sizeof(uint8_t) * SSD1306_VIRT_PAGES * SSD1306_VIRT_COLUMNS);
    buffer += sizeof(uint8_t) * SSD1306_VIRT_PAGES * SSD1306_VIRT_COLUMNS;
    memcpy(buffer, &screen.flags, sizeof(uint16_t));
    buffer += sizeof(uint16_t);
    memcpy(buffer, &screen.command_register, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(buffer, &screen.contrast_register, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(buffer, &screen.cs_pin, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(buffer, &screen.di_pin, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(buffer, &screen.spi_data, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(buffer, &screen.reg_write_sz, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(buffer, &screen.addr_mode, sizeof(ssd1306_addressing_mode_t));
    buffer += sizeof(ssd1306_addressing_mode_t);
    memcpy(buffer, &screen.twi_selected, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(buffer, &screen.twi_index, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    int esize = getEEPROMSize();
    memcpy(buffer, getEEPROM(), esize);
    buffer += esize;
    return true;
}

bool Arduous::load(const void* data, size_t size) {
    auto* buffer = static_cast<const uint8_t*>(data);
    memcpy(&cpu->state, buffer, sizeof(int));
    buffer += sizeof(int);
    memcpy(&cpu->cycle, buffer, sizeof(avr_cycle_count_t));
    buffer += sizeof(avr_cycle_count_t);
    memcpy(&cpu->run_cycle_count, buffer, sizeof(avr_cycle_count_t));
    buffer += sizeof(avr_cycle_count_t);
    memcpy(&cpu->run_cycle_limit, buffer, sizeof(avr_cycle_count_t));
    buffer += sizeof(avr_cycle_count_t);
    memcpy(cpu->sreg, buffer, sizeof(uint8_t) * 8);
    buffer += sizeof(uint8_t) * 8;
    memcpy(&cpu->interrupt_state, buffer, sizeof(int8_t));
    buffer += sizeof(int8_t);
    memcpy(&cpu->pc, buffer, sizeof(avr_flashaddr_t));
    buffer += sizeof(avr_flashaddr_t);
    memcpy(&cpu->reset_pc, buffer, sizeof(avr_flashaddr_t));
    buffer += sizeof(avr_flashaddr_t);
    memcpy(cpu->data, buffer, cpu->ramend + 1);
    buffer += cpu->ramend + 1;

    memcpy(&screen.cursor, buffer, sizeof(ssd1306_virt_cursor_t));
    buffer += sizeof(ssd1306_virt_cursor_t);
    memcpy(screen.vram, buffer, sizeof(uint8_t) * SSD1306_VIRT_PAGES * SSD1306_VIRT_COLUMNS);
    buffer += sizeof(uint8_t) * SSD1306_VIRT_PAGES * SSD1306_VIRT_COLUMNS;
    memcpy(&screen.flags, buffer, sizeof(uint16_t));
    buffer += sizeof(uint16_t);
    memcpy(&screen.command_register, buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(&screen.contrast_register, buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(&screen.cs_pin, buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(&screen.di_pin, buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(&screen.spi_data, buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(&screen.reg_write_sz, buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(&screen.addr_mode, buffer, sizeof(ssd1306_addressing_mode_t));
    buffer += sizeof(ssd1306_addressing_mode_t);
    memcpy(&screen.twi_selected, buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(&screen.twi_index, buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);

    int esize = getEEPROMSize();
    memcpy(getEEPROM(), buffer, esize);
    buffer += esize;
    return true;
}

size_t Arduous::getRamSize() {
    return cpu->ramend + 1;
}

void *Arduous::getRam() {
    return cpu->data;
}

size_t Arduous::getEEPROMSize() {
    return 0x400;
}

void *Arduous::getEEPROM() {
    avr_eeprom_desc_t d = {
	.ee = 0,
	.offset = 0,
	.size = 0
    };
    d.size = getEEPROMSize();
    avr_ioctl(cpu, AVR_IOCTL_EEPROM_GET, &d);

    return d.ee;
}

int16_t Arduous::getCurrentSpeakerSample() {
    switch (speakerPins.to_ulong()) {
        case 0:
        case 3:
            return 0;
            break;
        case 1:
            return INT16_MAX;
            break;
        case 2:
            return INT16_MIN;
        default:
            throw std::runtime_error("Invalid speaker pin value");
    }
}

void Arduous::extendAudioBuffer() {
    int endSampleIndex = 2 * std::min((cpu->cycle - frameStartCycle) / cyclesPerAudioSample,
                                      static_cast<uint64_t>(audioSamplesPerVideoFrame));
    int16_t currentSample = getCurrentSpeakerSample();
    for (uint i = audioBuffer.size(); i < endSampleIndex; i++) {
        audioBuffer.push_back(currentSample);
    }
}

void Arduous::soundPinCallback(struct avr_irq_t* irq, uint32_t value, void* param) {
    auto* pinCallbackParamT = static_cast<PinCallbackParamT*>(param);
    Arduous* self = pinCallbackParamT->self;
    self->extendAudioBuffer();
    self->speakerPins[pinCallbackParamT->speakerPin] = value & 0x1;
}
