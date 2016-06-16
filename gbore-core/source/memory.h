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
#pragma once

// MMU Defines
#define GB_MEMBANK_SIZE        0x4000
#define GB_SWITCHBANK_OFFSET   0x4000
#define GB_TOTAL_MEMSIZE       0x10000


//TODO:Move rest of GB_IO_* here?
#define GB_IO_OAM_DMA 0xFF46
#define GB_IO_TIMER_MODULO 0xFF06 //belongs to interruptz


struct gb_machine_s;


typedef struct gb_memory_unit_s { 
    
    gb_word_t      mem[GB_TOTAL_MEMSIZE]; //operational memory 
    struct gb_cart source; //cartridge buffer


    gb_dword_t     ROM_write_buffer; //if anyone tries to write ROM, let him write here
    gb_addr_t      last_read_addr;    
    gb_addr_t      last_write_addr;
    //bool           mmu_verbose; //or mmu_hooks_enabled 

    struct {
        bool is_OAM_DMA_scheduled;


        bool     IO_port_write_detected;
        gb_addr_t IO_port_write_address;    
        gb_word_t IO_port_pre_write_value; //SIO value before modification

    } SIO; // Special IO directives (for side effects)
    
} gb_memory_unit_s;




//////////////////////////////////////////////////////
// Define Memory Sections
/////////////////////////////////////////////////////

#define FOREACH_MEMORY_SECTION(MEMSEC)          \
    MEMSEC(0x0000, 0x4000, ROM_BANK_0)          \
    MEMSEC(0x4000, 0x8000, ROM_BANK_S)          \
    MEMSEC(0x8000, 0xA000, RAM_VIDEO)           \
    MEMSEC(0xA000, 0xC000, RAM_BANK_S)          \
    MEMSEC(0xC000, 0xE000, RAM_INTERNAL)        \
    MEMSEC(0xE000, 0xFE00, RAM_INTERNAL_ECHO)   \
    MEMSEC(0xFE00, 0xFEA0, SPRITE_ATTRIB_MEM)   \
    MEMSEC(0xFEA0, 0xFF00, UNUSABLE_EMPTY_1)    \
    MEMSEC(0xFF00, 0xFF4C, IO_PORTS)            \
    MEMSEC(0xFF4C, 0xFF80, UNUSABLE_EMPTY_2)    \
    MEMSEC(0xFF80, 0xFFFF, RAM_INTERNAL_EXT)    \
    MEMSEC(0xFFFF, 0xFFFF, INTERRUPT_ENABLE)


//Access memory in special 's' mode - without any checks and tracing in MMU module
#define GB_MMU_ACCESS_INTERNAL(address) gb_MMU_access((address), 's')

//TODO:Enable LTCG for inlining
gb_word_t * gb_MMU_access(gb_addr_t const address, char const mode);


//
// Type and l/r-value restraining wrappers for each type/size of memory operation.
//
__forceinline
gb_word_t const * const
gb_MMU_load8(gb_addr_t const address) {

    return gb_MMU_access(address, 'r');
}


__forceinline
gb_dword_t const * const
gb_MMU_load16(gb_addr_t const address) {

    return (gb_dword_t*)gb_MMU_access(address, 'r');
}


__forceinline
gb_word_t * const
gb_MMU_store8(gb_addr_t const address) {

    return gb_MMU_access(address, 'w');
}




__forceinline
gb_dword_t * const
gb_MMU_store16(gb_addr_t const address) {

    return (gb_dword_t*)gb_MMU_access(address, 'w');
}



void 
gb_MMU_cartridge_init(void const * const cart_data, size_t const cart_data_size);

void gb_MMU_step();


