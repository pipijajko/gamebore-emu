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
#define GB_ROMBANK_BYTES       0x4000
#define GB_RAMBANK_BYTES       0x2000
#define GB_WRAMBANK_BYTES      0x1000
#define GB_ECHORAM_BYTES       0x1F81
#define GB_MMIO_HRAM_BYTES     0x007F
#define GB_MEMBANK_SIZE        0x4000
#define GB_SWITCHBANK_OFFSET   0x4000
#define GB_TOTAL_MEMSIZE       0x10000
#define GB_OAM_DMA_TRANSFER_SIZE  0x9F

typedef struct gb_memory_range_s
{
    uint32_t begin; // memory section begin address (inclusive)
    uint32_t end;   // memory section end address (exclusive)
    int       index; // index of the section in the memory view

} gb_memory_range_s;

struct gb_machine_s;

/* Memory controller callbacks*/
typedef void* gbmmu_section_context_h; // Memory controller internal context
typedef gb_word_t (gbmmu_access_evt)(gb_addr_t const, char const, gbmmu_section_context_h*);
typedef gbmmu_access_evt*  gbmmu_access_pfn;

typedef uint16_t gb_section_attribs_t;
#define GB_MEMATTR_READONLY     0x1
#define GB_MEMATTR_CARTRIDGE    0x2
#define GB_MEMATTR_INVALID      0x4

typedef struct gb_memory_section_s
{
    byte_t* data;

    gb_section_attribs_t attribs; //R, W, RW, Extension, storeVal
    gbmmu_access_pfn mbc_hook;

}gb_memory_section_s;


//////////////////////////////////////////////////////
// Define Generic Memory Sections
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
    MEMSEC(0xFFFF, 0x10000, INTERRUPT_ENABLE)


#define MEMORY_SECTION_N 12

//////////////////////////////////////////////////////
// Use X-MACROS to generate memory_view layout 
//////////////////////////////////////////////////////
#include "memory_xmacros.h"

typedef struct gb_memory_unit_s { 
    // Operational memory is divided into sections.
    gb_memory_section_s    memory_view[MEMORY_SECTION_N]; 
    gbmbc_h                cartridge;
    void * _private;
} gb_memory_unit_s;



//TODO:Enable LTCG for inlining
gb_word_t *gb_MMU_access_internal(gb_addr_t const address);
gb_word_t  gb_MMU_load8(gb_addr_t const address);
void       gb_MMU_store8(gb_addr_t const address, gb_word_t const value);

gb_dword_t gb_MMU_load16(gb_addr_t const address);
void       gb_MMU_store16(gb_addr_t const address, gb_dword_t const value);

void gb_MMU_initialize_internal_RAM();
bool gb_MMU_validate_is_memory_continous();

