#ifndef ARDUOUS_ATCORE_H
#define ARDUOUS_ATCORE_H

#include <cstdint>
#include <cstdio>
#include <vector>

struct AtcoreDesc {
    size_t flash = 32 * 1024;
    size_t eeprom = 1 * 1024;
    size_t sram = 2 * 1024;
    uint32_t clock = 16 * 1000 * 1000;
};

constexpr AtcoreDesc defaultDesc{};

class Atcore {
   public:
    Atcore();
    explicit Atcore(AtcoreDesc desc);

    void tick();

    AtcoreDesc getDesc();

   private:
    AtcoreDesc desc;

    std::vector<uint8_t> memory;
    std::vector<uint8_t> flash;
    std::vector<uint8_t> eeprom;

    // Registers
    uint32_t pc;
    uint16_t sp;
    uint8_t sreg;

    // Instructions
    // https://en.wikipedia.org/wiki/Atmel_AVR_instruction_set#Instruction_encoding

    void doNOP();
    void doMOVW();
    void doMULS();
    void doMULSU();
    void doFMUL();
    void doFMULS();
    void doCP();
    void doSUB();
    void doADD();
    void doCPSE();
    void doAND();
    void doEOR();
    void doOR();
    void doMOV();
    void doCPI();
    void doSUBI();
    void doORI();
    void doANDI();
};

#endif
