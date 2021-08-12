#ifndef ARDUOUS_ATCORE_H
#define ARDUOUS_ATCORE_H

#include <cstdint>
#include <cstdio>

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
    size_t pc = 0;
};

#endif
