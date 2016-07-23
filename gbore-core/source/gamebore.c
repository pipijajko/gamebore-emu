/*
Copyright 2016 Maciej Kurczewski

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
// gamebore.cpp : Defines the entry point for the console application.
//
#define GDEBUG
#include "gamebore.h"
#include "dispatch_tables.h"
#include "string.h"

struct gb_machine_s g_GB;

void gb_initialize(void const* const cart_data, size_t const cart_data_size)
{
    memset(&g_GB, 0, sizeof(gb_machine_s));
    g_GB.gb_model = gb_dev_DMG;

    gb_build_decoder_table(g_GB.ops, GB_OPCODES_N);
    gbdbg_initialize(&g_GB, &g_GB.dbg);
    gb_MMU_cartridge_init(cart_data, cart_data_size); //mmu should take ownership of cart_data
    gb_CPU_init();
    gb_INPUT_initialize(&g_GB.keypad);
    gbdbg_mute_print(g_GB.dbg, true);
}



void gb_machine_step(gbdisplay_h disp) 
{
    byte_t ticks = gb_CPU_step(); 
    gb_MMU_step();
    gb_INPUT_step(); 
    gb_INTERRUPT_step(ticks); 
    gb_DISPLAY_render_line(disp, ticks);
}



void
gb_destroy()
{
    gbdbg_destroy(g_GB.dbg);

}