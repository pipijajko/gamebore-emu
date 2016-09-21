#include "gamebore.h"
#include "memory.h"
#include "string.h"


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


void gb_CART_map_into_memory_view(gbmbc_h handle) 
{
    StopIf(!handle.unused, return, "No cartridge, can not populate memory view");
    gb_memory_controller_s * const cart = (gb_memory_controller_s*)handle.unused;

    // Map the ROM bank#0 data to memory view
    cart->instance->m.memory_view[gb_ROM_BANK_0].data = &cart->ROM_banks[0];
    cart->instance->m.memory_view[gb_ROM_BANK_0].attribs = GB_MEMATTR_READONLY | GB_MEMATTR_CARTRIDGE;

    // Map the 1st of switchable ROM banks to memory view (if present)
    if (cart->ROM_banks_n > 1) {
        cart->instance->m.memory_view[gb_ROM_BANK_S].data = &cart->ROM_banks[GB_ROMBANK_BYTES * 1];
        cart->instance->m.memory_view[gb_ROM_BANK_S].attribs = GB_MEMATTR_READONLY | GB_MEMATTR_CARTRIDGE;
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
