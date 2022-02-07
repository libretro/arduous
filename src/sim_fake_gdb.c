#include "sim_avr.h"
#include "sim_gdb.h"

void
avr_gdb_handle_watchpoints(
		avr_t * avr,
		uint16_t addr,
		enum avr_gdb_watch_type type )
{
}

int
avr_gdb_init(
		avr_t * avr )
{
        return 0; // GDB server already is active
}

void
avr_deinit_gdb(
		avr_t * avr )
{
}

int
avr_gdb_processor(
		avr_t * avr,
		int sleep )
{
        return 0;
}
