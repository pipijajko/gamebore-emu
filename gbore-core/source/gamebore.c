// gamebore.cpp : Defines the entry point for the console application.
//
#define GDEBUG
#include "gamebore.h"
#include "dispatch_tables.h"
#include "string.h"

struct gb_machine_s g_GB;

void 
gb_initialize(
    void *      cart_data,
    size_t      cart_data_size
){
    memset(&g_GB, 0, sizeof(gb_machine_s));
    g_GB.gb_model = gb_dev_DMG;

    gb_build_decoder_table(g_GB.ops, GB_OPCODES_N);
    gbdbg_initialize(&g_GB, &g_GB.dbg);
    gb_MMU_cartridge_init(&g_GB, cart_data, cart_data_size); //mmu should take ownership of cart_data
    gb_CPU_init();

}



void
gb_destroy()
{
    gbdbg_destroy(g_GB.dbg);

}