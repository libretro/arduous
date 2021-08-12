#include "arduous/atcore.h"

Atcore::Atcore() : Atcore(defaultDesc){};

Atcore::Atcore(AtcoreDesc desc) { this->desc = desc; }

void Atcore::tick() { pc++; }

AtcoreDesc Atcore::getDesc() { return desc; }
