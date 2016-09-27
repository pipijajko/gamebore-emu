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
#include <string.h>
#include <assert.h>

#include "gamebore.h"

__forceinline
gb_memory_range_s gb_MMU_get_section_range(const gb_addr_t address)
{
    //TODO: figure out faster or static lookup
    //TODO: heuristic to precalculate starting index

    for (int i = 0; i < MEMORY_SECTION_N; i++) {
        gb_memory_range_s const * const section = &gb_memory_ranges[i];
        if (IN_RANGE(address, section->begin, section->end)) {
            return  *section;
        }
    }
    //We should never reach this point
    StopIf(true, abort(), "FATAL: Could not map address 0x%hx to any memory section", address);
    return (gb_memory_range_s){ 0, 0, 0 };
}


__forceinline
gb_word_t gb_MMU_load8(gb_addr_t const address)
{
    gb_memory_range_s const sec_r = gb_MMU_get_section_range(address);
    return g_GB.m.memory_view[sec_r.index].data[address - sec_r.begin];
}


void gb_MMU_OAM_DMA_execute(gb_word_t IO_OAM_DMA_port_value) 
{
    gb_addr_t const dest_address   = 0xFE00; //OAM start address
    gb_addr_t const source_address = (IO_OAM_DMA_port_value << 8);
    //
    // naive implemention:  missing clock sync (should take 5x40 cycles)
    // 
    for (uint16_t i = 0x00; i < GB_OAM_DMA_TRANSFER_SIZE; i++) {
        *gb_MMU_access_internal(dest_address + i) = *gb_MMU_access_internal(source_address + i);
    }
    gdbg_trace(g_GB.dbg, "Executed OAM DMA 0x%04hx<--0x%04hx (0x%02x)\n",
               dest_address, source_address, GB_OAM_DMA_TRANSFER_SIZE);

}


void gb_MMU_store8(gb_addr_t const address, gb_word_t const value) 
{
    gb_memory_range_s const sec_r       = gb_MMU_get_section_range(address);
    gb_memory_section_s * const section = &g_GB.m.memory_view[sec_r.index];
    bool is_writable                    = (section->attribs & GB_MEMATTR_READONLY) == 0;
    bool is_cartridge                   = (section->attribs & GB_MEMATTR_CARTRIDGE) == GB_MEMATTR_CARTRIDGE;

    if(!is_writable){
        if (is_cartridge && section->mbc_hook) section->mbc_hook(g_GB.m.cartridge, address, value);
        else gdbg_trace(g_GB.dbg, "Unhandled ROM Write attempt @ 0x%x!", address);
    } 
    //
    //  Special I/O write:
    //
    if (gb_IO_PORTS == sec_r.index) {
        switch (address) {
        case GB_IO_OAM_DMA:
            gb_MMU_OAM_DMA_execute(value);
            is_writable = false;
        
        case GB_IO_DIV:
        case GB_IO_LY:
            //IO-PORTs that autoreset on write:
            section->data[address - sec_r.begin] = 0;
            is_writable = false;
            break;
        default:
            break;
        }
    }
    if(is_writable) section->data[address - sec_r.begin] = value;
}
    

gb_dword_t gb_MMU_load16(gb_addr_t const address) 
{
    gb_dword_t lo = gb_MMU_load8(address);
    gb_dword_t hi = gb_MMU_load8(address + 1);
    return (hi << 8) + lo;
}


void gb_MMU_store16(gb_addr_t const address, gb_dword_t const value) 
{
    gb_word_t lo = value & 0xFF;
    gb_word_t hi = (value & 0xFF00) >> 8;
    gb_MMU_store8(address, lo);
    gb_MMU_store8(address + 1, hi);
}


void gb_MMU_initialize_internal_RAM() 
{
    gb_memory_unit_s * const u    = &g_GB.m; //global instance
    gb_memory_range_s range;

    byte_t * const internal_memory = calloc(1, GB_TOTAL_MEMSIZE);
    u->_private = internal_memory; //keep in `_private` for free();
    StopIf(!internal_memory, abort(), "Internal memory calloc() failed! Abort()");


    // Initialize all memory sections (without data ptr) to mapping to internal memory.
    // This is not most readable, but we will have a continous block of memory.
    // Regardless of how fine-grained we have the MEMORY SECTION mapping.

    // Sections corresponding to EXT_RAM/ROM sections shall be overwritten by gb_CART_map function.

    for (int i = 0; i < MEMORY_SECTION_N; i++) {
        range = gb_memory_ranges[i];
        if (NULL == u->memory_view[i].data ) {
            u->memory_view[i].data = &internal_memory[range.begin];
        }
    }
    //
    //Re-map RAM_INTERNAL to ECHO
    //
    u->memory_view[gb_RAM_INTERNAL_ECHO].data  = u->memory_view[gb_RAM_INTERNAL].data;
}


gb_word_t * gb_MMU_access_internal(gb_addr_t const address) 
{
    // Unsafe memory access via pointer for internal subsystems (interrupter, display, input)
    // TODO: Optimize by starting indexing @ IO_ports (most common)
    gb_memory_range_s sec_r = gb_MMU_get_section_range(address);
    return &g_GB.m.memory_view[sec_r.index].data[address - sec_r.begin];
}

//TODO: consider static section lookup instead of hint version of internal access
//gb_word_t * gb_MMU_access_internal_hint(gb_addr_t const address, enum gb_mem_section hint) {
//    // Unsafe memory access via pointer for internal subsystems (interrupter, display, input)
//    gb_memory_range_s sec_r = gb_memory_ranges[(int)hint];
//    if (!IN_RANGE(address, sec_r.begin, sec_r.end)) {
//        sec_r = gb_MMU_get_section_range(address);
//        gdbg_trace(g_GB.dbg, "gb_MMU_access_internal_hint: INCORRECT HINT:%d!\n"
//                   "Actual memory section:%d", (int)hint, sec_r.index);
//    }
//    return &g_GB.m.memory_view[sec_r.index].data[address - sec_r.begin];
//}

bool gb_MMU_validate_is_memory_continous() 
{
    gb_memory_unit_s const * const u = &g_GB.m; //global 
    uint32_t total_memsize = 0;
    uint32_t prev_section_end = 0;

    for (int i = 0; i < MEMORY_SECTION_N; i++) {

        gb_memory_range_s range = gb_memory_ranges[i];

        if (NULL == u->memory_view[i].data) {
            gdbg_trace(g_GB.dbg, "Memory section:%d has no memory mapped!\n"
                       "Make sure you initialzed MMUs internal RAM and CART first!", i);
            return false;
        }

        if (range.begin != prev_section_end) {
            gdbg_trace(g_GB.dbg, "Memory section:%d is not continous!\n"
                       "Fix FOREACH_MEMORY_SECTION macro!", i);
            return false;
        }

        prev_section_end = range.end;
        total_memsize += range.end - range.begin;
    }

    if (GB_TOTAL_MEMSIZE != total_memsize) {

        gdbg_trace(g_GB.dbg, "Memory sections do not cover full %uBytes.\n"
                   "Only %uBytes are present.\n"
                   "Fix FOREACH_MEMORY_SECTION macro!", GB_TOTAL_MEMSIZE, total_memsize);
        return false;
    }
    return true;
}