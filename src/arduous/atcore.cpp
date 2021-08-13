#include "arduous/atcore.h"

Atcore::Atcore() : Atcore(defaultDesc){};

Atcore::Atcore(AtcoreDesc desc) { this->desc = desc; }

void Atcore::tick() { pc++; }

AtcoreDesc Atcore::getDesc() { return desc; }

void Atcore::doNOP() {}

// void Atcore::doADD() {
//     uint8_t d = ((opcode & 0x01F0) >> 4);
//     uint8_t r = ((opcode & 0x0200) >> 5) | (opcode & 0x000F);
//     uint8_t r_d = memory[d];
//     uint8_t r_r = memory[r];
//     uint8_t result = r_d + r_r;

//     uint8_t carry = r_d & r_r | r_r & ~result | ~result & r_d;
//     setStatusH((carry >> 3) & 0x1);
//     setStatusV(((r_d & r_r & ~result | ~r_d & ~r_r & result) >> 7) & 0x1);
//     setStatusC((carry >> 7) & 0x1);
//     setStatusZ(result == 0);
//     setStatusN((result >> 7) & 0x1);
//     setStatusS(getStatusN() ^ getStatusV());

//     memory[r_d] = result;
// }
