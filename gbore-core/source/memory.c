#include <string.h>
#include <assert.h>

#include "gamebore.h"



struct gb_memory_section {
    gb_addr_t begin;
    gb_addr_t end;
    void *  handler;
};

//
// Generate memory-section data and lookups
//
#define GENERATE_ID_ENUM(a, b, ENUM) gb_##ENUM,
#define GENERATE_TXTLOOKUP(a, b, STRING) #STRING,
#define GENERATE_STRUCT_INIT(abegin, aend, NAME) {abegin, aend, NULL},


// generate section structs
static const struct gb_memory_section gb_mmu_sec[] = {
    FOREACH_MEMORY_SECTION(GENERATE_STRUCT_INIT)
};


//verify size
#define MEMORY_SECTION_N (sizeof(gb_mmu_sec)/sizeof(gb_mmu_sec[0]))
static_assert(MEMORY_SECTION_N == 12, "gb_mmu_sec is not const size array or new sections were added");


enum gb_mem_section {
    FOREACH_MEMORY_SECTION(GENERATE_ID_ENUM)
};


const char *gb_mem_section2text[] = {
    FOREACH_MEMORY_SECTION(GENERATE_TXTLOOKUP)
};

#undef GENERATE_ID_ENUM
#undef GENERATE_TXTLOOKUP
#undef GENERATE_STRUCT_INIT


//
// To delegate the side effects let's not act (eg. bank switch, VideoSettings adjust etc.) on the 
// data written.
// 


gb_word_t* gb_MMU_access(gb_addr_t const address, char const mode) 
{
    gb_memory_unit_s *const u = &g_GB.m; //global 

    if ('r' == mode) {
        //
        // Seems like for read we can skip additional actions.
        // Just put an assert here to verify that no one messes with echo memory
        //
        if (IN_RANGE(address, 0xE000, 0xFE00)) {
            gdbg_trace(g_GB.dbg, "Shadow Memory Access!@0x%04x\n", address);
        }
        return &u->mem[address];
    }
    if ('w' == mode) {

        //Special IO 
        // Maybe a more generic handler which just saves modified address and prev value in
        // SIO range. Now we have two switch()es :(
        if (address == GB_IO_OAM_DMA) {
            u->SIO.is_OAM_DMA_scheduled = true;
        }

        if (address == 0x9800) {
            gdbg_trace(g_GB.dbg, "First sprite write @ 0x%04x\n", address);
        }


        //Temporary here
        if (address < 0x8000) {
            gdbg_trace(g_GB.dbg,"Invalid Memory Access! Write over ROM,@0x%04x\n", address);
            //this is actually legit, for special operations, 
            //assert(false);
        }
        

        //
        // For write find memory section handler and run it. 
        // If handler is NULL, ignore.
        //
        for (size_t i = address / 0x4000; i < MEMORY_SECTION_N; i++) {
            
            
            if (IN_RANGE(address, gb_mmu_sec[i].begin, gb_mmu_sec[i].end)) {
                
                // Special IO:
                if (i == gb_IO_PORTS) {
                    u->SIO.IO_port_write_detected  = true;
                    u->SIO.IO_port_write_address   = address;
                    u->SIO.IO_port_pre_write_value = u->mem[address];


                }
                //gdbg_trace(gb->dbg, "Write @0x%04X in %s\n", address, gb_mem_section2text[i]);
                break;

            }
        }
        return &u->mem[address];
    }
    if ('s' == mode || 'i' == mode) {

        //Special/Silent or Internal mode - perform no checks and no tracing
        return &u->mem[address];
    }

    assert(false);
    return NULL;
}


void gb_MMU_step() 
{
    gb_memory_unit_s *const u = &g_GB.m; //global 

    if (u->SIO.is_OAM_DMA_scheduled) {
        u->SIO.is_OAM_DMA_scheduled = false;

        //
        // simple implemention: missing clock sync (should take 5x40 cycles)
        // 
        gb_addr_t const dest_address  = 0xFE00; //OAM start address
        gb_addr_t const source_address = u->mem[GB_IO_OAM_DMA] << 8;
        size_t const transfer_size = 0x9F;
        
        gdbg_trace(g_GB.dbg, "OAM DMA 0x%04hx<--0x%04hx (0x%02xB)",
                   dest_address, source_address, transfer_size);

        memcpy_s(&u->mem[dest_address],
                 GB_TOTAL_MEMSIZE - dest_address,
                 &u->mem[source_address],
                 transfer_size);
    }

    if (u->SIO.IO_port_write_detected) {
        u->SIO.IO_port_write_detected = false;
        gb_addr_t const port_address = u->SIO.IO_port_write_address;
        //gb_word_t const old_value = u->SIO.IO_port_pre_write_value;
        //gb_word_t const new_value = u->mem[port_address];

        switch (port_address) {
        case GB_IO_LY:
            // If write to LY was detected - reset it's value to 0
            u->mem[port_address] = 0x00;
            break;

        case GB_IO_LCDC:
            //F-U
            break;


        }
    }


}


void gb_MMU_cartridge_init(void const * cart_data, size_t const cart_data_size)
{
    gb_memory_unit_s *const u = &g_GB.m; //global instance
    memset(u, 0, sizeof(gb_memory_unit_s));

    assert(cart_data);
    assert(cart_data_size);

    u->source.buffer = cart_data;
    u->source.size   = cart_data_size;
    //
    // Copy ROM Bank #0 of cartdidge to main memory
    //
    memcpy_s(&u->mem[0x0000], GB_TOTAL_MEMSIZE, u->source.buffer, GB_MEMBANK_SIZE);

    if (u->source.size > GB_MEMBANK_SIZE) {
        //cartridge has multiple banks
        // Copy Switchable ROM Bank of cartdidge to main memory
        memcpy_s(&u->mem[GB_SWITCHBANK_OFFSET], (GB_TOTAL_MEMSIZE - GB_SWITCHBANK_OFFSET),
                 &u->source.buffer[GB_SWITCHBANK_OFFSET], GB_MEMBANK_SIZE);
    }

    u->source.Type = u->mem[GB_OFFSET_CART];
    u->source.ROM  = u->mem[GB_OFFSET_ROMSIZE];
    u->source.RAM  = u->mem[GB_OFFSET_RAMSIZE];


    /* TODO: Scroll Nintendo logo from $104:$133 */
    /* TODO: Add cheksum verification - LSB_of(SUM($143:$14d)+25) == 0 */



    u->mem[0xFF05] = u->mem[0x0000]; // TIMA
    u->mem[0xFF06] = u->mem[0x0000]; // TMA
    u->mem[0xFF07] = u->mem[0x0000]; // TAC
    u->mem[0xFF10] = u->mem[0x0080]; // NR10
    u->mem[0xFF11] = u->mem[0x00BF]; // NR11
    u->mem[0xFF12] = u->mem[0x00F3]; // NR12
    u->mem[0xFF14] = u->mem[0x00BF]; // NR14
    u->mem[0xFF16] = u->mem[0x003F]; // NR21
    u->mem[0xFF17] = u->mem[0x0000]; // NR22
    u->mem[0xFF19] = u->mem[0x00BF]; // NR24
    u->mem[0xFF1A] = u->mem[0x007F]; // NR30
    u->mem[0xFF1B] = u->mem[0x00FF]; // NR31
    u->mem[0xFF1C] = u->mem[0x009F]; // NR32
    u->mem[0xFF1E] = u->mem[0x00BF]; // NR33
    u->mem[0xFF20] = u->mem[0x00FF]; // NR41
    u->mem[0xFF21] = u->mem[0x0000]; // NR42
    u->mem[0xFF22] = u->mem[0x0000]; // NR43
    u->mem[0xFF23] = u->mem[0x00BF]; // NR30
    u->mem[0xFF24] = u->mem[0x0077]; // NR50
    u->mem[0xFF25] = u->mem[0x00F3]; // NR51
    u->mem[0xFF26] = (g_GB.gb_model == gb_dev_SGB) ? u->mem[0x00F0] : u->mem[0x00F1]; // NR52
    u->mem[0xFF40] = u->mem[0x0091]; // LCDC
    u->mem[0xFF42] = u->mem[0x0000]; // SCY
    u->mem[0xFF43] = u->mem[0x0000]; // SCX
    u->mem[0xFF45] = u->mem[0x0000]; // LYC
    u->mem[0xFF47] = u->mem[0x00FC]; // BGP
    u->mem[0xFF48] = u->mem[0x00FF]; // OBP0
    u->mem[0xFF49] = u->mem[0x00FF]; // OBP1
    u->mem[0xFF4A] = u->mem[0x0000]; // WY
    u->mem[0xFF4B] = u->mem[0x0000]; // WX
    u->mem[0xFFFF] = u->mem[0x0000]; // IE


}




/*
Gameboy Memory Bank Controller 1 (MBC1)


Two modes:
 * 16Mbit ROM 8KB RAM - default on powerup
 * 4Mbit ROM/32KB RAM


Writing given word into 6000-7FFF area will select memory model:
-------0 -> ROM:16/RAM:8
-------1 -> ROM:4/RAM:32



Writing given word into 2000-3FFF area will select ROM bank at $4000:
---BBBBB
---00000 -> ROM Bank #1 (yes, 1)
---00001 -> ROM Bank #1

In 16/8 mode writing given value in 4000-5FFF area will set the two 
most significant addres bits to BB.
------BB


In 4/32 mode writing given value in 4000-5FFF area will select appropriate RAM bank
at A000-C000.
Before you can read or write to a RAM bank you have to enable
it by writing a XXXX1010 into 0000-1FFF area. To
disable RAM bank operations write any value but
XXXX1010 into 0000-1FFF area. Disabling a RAM bank
probably protects that bank from false writes
during power down of the GameBoy. (NOTE: Nintendo
suggests values $0A to enable and $00 to disable
RAM bank!!)



( '-' don't matter)


*/


