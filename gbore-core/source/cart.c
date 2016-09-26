#include "gamebore.h"
#include "string.h"
#include "helpers.h"


typedef struct gb_memory_controller_s //for MBC1
{
    byte_t   * ROM_banks;
    uint16_t   ROM_banks_n; // >= 2
    uint16_t   ROM1_selector; // >= 1


    byte_t *   EXT_RAM_banks;
    uint16_t   EXT_RAM_banks_n;  // >= 2 (>=4 on GBC)
    uint16_t   EXT_RAM_selector;

    struct gb_machine_s *instance;
    struct gb_cart meta;

} gb_memory_controller_s;


errno_t gb_CART_initialize(struct gb_machine_s *const instance,
                           void const*const cart_data,
                           size_t const cart_data_size,
                           gbmbc_h * const handle
) {
    errno_t err = 0;
    gb_memory_controller_s * cart = calloc(1, sizeof(gb_memory_controller_s));
    if (!instance || !cart) { err = EINVAL; goto cleanup_generic; }
    if (cart_data_size < 0x4000 ) { 
        gdbg_trace(g_GB.dbg, "Invalid cartridge size:%d (too small)", cart_data_size);
        err = EINVAL; 
        goto cleanup_generic; 
    }

    //
    // Read metadata from Cartridge file
    //
    cart->meta.Type = ((byte_t*)cart_data)[GB_OFFSET_CART];
    cart->meta.ROM  = ((byte_t*)cart_data)[GB_OFFSET_ROMSIZE];
    cart->meta.RAM  = ((byte_t*)cart_data)[GB_OFFSET_RAMSIZE];
    gdbg_trace(g_GB.dbg, "------------------ROM DATA-------------------\n");
    gdbg_trace(g_GB.dbg, "Type:0x%hhx\nROM-Size:0x%hhx\nRAM-Size:0x%hhx\n",
               cart->meta.Type, cart->meta.ROM, cart->meta.RAM);
    gdbg_trace(g_GB.dbg, "Recognized ROM Banks:0x%x\n", (cart_data_size / GB_ROMBANK_BYTES));


    //    
    // Initialize memory sections holding Cartridge's ROM
    //
    uint16_t const number_of_ROM_banks = (uint16_t)(cart_data_size / GB_ROMBANK_BYTES);
    size_t const   total_ROM_bytes     = number_of_ROM_banks * GB_ROMBANK_BYTES;

    cart->ROM_banks_n = number_of_ROM_banks;
    cart->ROM_banks   = calloc(number_of_ROM_banks, GB_ROMBANK_BYTES);
    StopIf(!cart->ROM_banks, abort(), "ROM Calloc failed! Abort!");
    // We don't own the Cartridge data here, copy it to internally allocated buffer 
    // (as a multiplier of GB_ROMBANK_BYTES)
    memcpy_s(cart->ROM_banks, total_ROM_bytes, cart_data, total_ROM_bytes);

    //
    // Initialize memory sections for cartridge held external RAM
    //
    uint16_t const number_of_RAM_Banks = (uint16_t)(cart->meta.RAM << 2);
    
    cart->EXT_RAM_banks = calloc(number_of_RAM_Banks, GB_RAMBANK_BYTES);
    StopIf(!cart->ROM_banks, abort(), "RAM Calloc failed! Abort!");
    
    cart->instance  = instance;
cleanup_generic:
    if (err) { free(cart); cart = NULL; }
    if (handle) handle->unused = (void*)cart;
    return err;
}


void gb_CART_destroy(gbmbc_h handle) 
{
    StopIf(!handle.unused, return, "No cartridge present!");
    gb_memory_controller_s* cart = (gb_memory_controller_s*)handle.unused;

    free(cart->EXT_RAM_banks);
    cart->EXT_RAM_banks = NULL;

    free(cart->ROM_banks);
    cart->ROM_banks = NULL;

    free(cart);
}


//TEMPORARY HERE
gb_word_t gb_CART_MBC1_write_hook(gbmbc_h handle, gb_addr_t const address, gb_word_t const write_val);


void gb_CART_map_into_memory_view(gbmbc_h handle) 
{
    StopIf(!handle.unused, return, "No cartridge, can not populate memory view");
    gb_memory_controller_s * const cart = (gb_memory_controller_s*)handle.unused;

    // Map the ROM bank#0 data to memory view
    cart->instance->m.memory_view[gb_ROM_BANK_0].data = &cart->ROM_banks[0];
    cart->instance->m.memory_view[gb_ROM_BANK_0].attribs = GB_MEMATTR_READONLY | GB_MEMATTR_CARTRIDGE;
    cart->instance->m.memory_view[gb_ROM_BANK_0].mbc_hook = gb_CART_MBC1_write_hook;

    // Map the 1st of switchable ROM banks to memory view (if present)
    if (cart->ROM_banks_n > 1) {
        cart->instance->m.memory_view[gb_ROM_BANK_S].data = &cart->ROM_banks[GB_ROMBANK_BYTES * 1];
        cart->instance->m.memory_view[gb_ROM_BANK_S].attribs = GB_MEMATTR_READONLY | GB_MEMATTR_CARTRIDGE;
        cart->instance->m.memory_view[gb_ROM_BANK_S].mbc_hook = gb_CART_MBC1_write_hook;
        cart->ROM1_selector = 1;
    } else {
        cart->instance->m.memory_view[gb_ROM_BANK_S].attribs = GB_MEMATTR_INVALID;
        cart->ROM1_selector = 0;
    }

    // Map MBC's external RAM to the memory view (if present)
    if (cart->EXT_RAM_banks_n > 0) {
        cart->instance->m.memory_view[gb_RAM_BANK_S].data = &cart->EXT_RAM_banks[0];
        cart->instance->m.memory_view[gb_RAM_BANK_S].attribs = GB_MEMATTR_CARTRIDGE;
        cart->EXT_RAM_selector = 0;
    } else {
        cart->instance->m.memory_view[gb_RAM_BANK_S].attribs = GB_MEMATTR_INVALID;
        cart->EXT_RAM_selector = 0;
    }
}


//////////////////////////////////////////////////////
// MBC1 Calls (TODO)
/////////////////////////////////////////////////////
/*
MBC1 Notes

0000-3FFF - ROM Bank 00 (Read Only)
4000-7FFF - ROM Bank 01-7F (Read Only)
A000-BFFF - RAM Bank 00-03, if any (Read/Write)
0000-1FFF - RAM Enable (Write Only)  (00h  Disable RAM (default) / 0Ah  Enable RAM)

2000-3FFF - ROM Bank Number (Write Only)
Writing a value (XXXXBBBB - X = Don't cares, B = bank select bits) into
2000-3FFF area will select an appropriate ROM bank at 4000-7FFF.


The least significant bit of the upper address byte must be one to select a
ROM bank. For example the following addresses can be used to select a ROM
bank: 2100-21FF, 2300-23FF, 2500-25FF, ..., 3F00-3FFF.
The suggested address range to use for MBC2 rom bank selection is 2100-21FF.
*/


gb_word_t gb_CART_MBC1_write_hook(gbmbc_h handle, gb_addr_t const address , gb_word_t const write_val)
{
    if (IN_RANGE(address, 0x0000, 0x1FFF)) {
        bool is_enable = (write_val & 0x0A) == 0x0A;
        gdbg_trace(g_GB.dbg, "MBC1: RAM Enable:%d (ignored)", is_enable);
        /*  0000 - 1FFF - RAM Enable(Write Only)
            Before external RAM can be read or written, it must be enabled by writing to
            this address space.It is recommended to disable external RAM after accessing
            it, in order to protect its contents from damage during power down of the
            gameboy.Usually the following values are used :
            00h  Disable RAM(default)
            0Ah  Enable RAM
            Practically any value with 0Ah in the lower 4 bits enables RAM, and any other
            value disables RAM.
        */

    }else if (IN_RANGE(address, 0x2000, 0x3FFF)) {
        /*  Writing to this address space selects the lower 5 bits of the ROM Bank Number
            (in range 01 - 1Fh).When 00h is written, the MBC translates that to bank 01h
            also.That doesn't harm so far, because ROM Bank 00h can be always directly
            accessed by reading from 0000 - 3FFF.
            But(when using the register below to specify the upper ROM Bank bits), the
            same happens for Bank 20h, 40h, and 60h.Any attempt to address these ROM
            Banks will select Bank 21h, 41h, and 61h instead.
            */

        gb_memory_controller_s * const cart = (gb_memory_controller_s*)handle.unused;
        byte_t const bank_select_bits = (write_val & 0x1F);
        byte_t const bank_index = (bank_select_bits & 0xF) == 0x00
                            ? bank_select_bits + 1
                            : bank_select_bits;
        gdbg_trace(g_GB.dbg, "MBC1: ROM Bank switch. Value:0x%hhx, Selected:%d", 
                   write_val, bank_index);

        if(cart->ROM_banks_n > bank_index){
            cart->instance->m.memory_view[gb_ROM_BANK_S].data = &cart->ROM_banks[GB_ROMBANK_BYTES * bank_index];
            cart->ROM1_selector = bank_index;
        } else {
            gdbg_trace(g_GB.dbg, "MBC1: ROM Bank switch failed! Bank:0x%hhx does not exist",
                       bank_index);
        }



    } else if(IN_RANGE(address, 0x4000,0x5FFF)){
        gdbg_trace(g_GB.dbg, "MBC1: RAM Bank switch /ROM Bank upper bits. NOT SUPPORTED YET");
        /*  4000-5FFF - RAM Bank Number - or - Upper Bits of ROM Bank Number (Write Only)
            This 2bit register can be used to select a RAM Bank in range from 00-03h, or
            to specify the upper two bits (Bit 5-6) of the ROM Bank number, depending on
            the current ROM/RAM Mode. (See below.)
        */

    } else if (IN_RANGE(address, 0x6000, 0x7FFF)) {
        gdbg_trace(g_GB.dbg, "MBC1: ROM/RAM mode Select. NOT SUPPORTED YET");
        /*  6000-7FFF - ROM/RAM Mode Select (Write Only)
            This 1bit Register selects whether the two bits of the above register should
            be used as upper two bits of the ROM Bank, or as RAM Bank Number.
                00h = ROM Banking Mode (up to 8KByte RAM, 2MByte ROM) (default)
                01h = RAM Banking Mode (up to 32KByte RAM, 512KByte ROM)
            The program may freely switch between both modes, the only limitiation is that
            only RAM Bank 00h can be used during Mode 0, and only ROM Banks 00-1Fh can be
            used during Mode 1.
        */
    }
    return write_val;
}

