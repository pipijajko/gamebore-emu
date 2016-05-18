#pragma once


//
// COFFSET
//

#define GB_OFFSET_COLOR    0x143
#define GB_OFFSET_SGB      0x146
#define GB_OFFSET_CART     0x147
#define GB_OFFSET_ROMSIZE  0x148
#define GB_OFFSET_RAMSIZE  0x149
#define GB_OFFSET_DESTCODE 0x14A
#define GB_OFFSET_LICENSEE 0x14B
#define GB_OFFSET_MASKROMV 0x14C
#define GB_OFFSET_CMPLMNTC 0x14D
#define GB_OFFSET_CHECKSUM 0x14E


enum gb_cart_type {
    gb_ROM_ONLY = 0x00, //256kb ROM 0000-7FFF
    gb_ROM_MBC1 = 0x01, //MBC1 modes: (16Mbit ROM 8KB RAM - default) or (4Mbit ROM/32KB RAM)
    gb_ROM_MBC1_RAM = 0x02, //MBC2 modes: only ROM switch up to 2MBit, 512x4 bits of RAM
    gb_ROM_MBC1_RAM_BATT = 0x03,
    gb_ROM_MBC2 = 0x05,
    gb_ROM_MBC2_BATT = 0x06,
    gb_ROM_RAM = 0x08,
    gb_ROM_RAM_BATT = 0x09,
    gb_ROM_MMM01 = 0x0B,
    gb_ROM_MMM01_SRAM = 0x0C,
    gb_ROM_MMM01_SRAM_BATT = 0x0D,
    gb_ROM_MBC3_TIMER_BATT = 0x0F,
    gb_ROM_MBC3_TIMER_RAM_BATT = 0x10,
    gb_ROM_MBC3 = 0x11,
    gb_ROM_MBC3_RAM = 0x12,
    gb_ROM_MBC3_RAM_BATT = 0x13,
    gb_ROM_MBC5_ = 0x19,
    gb_ROM_MBC5_RAM = 0x19,
    gb_ROM_MBC5_RAM_BATT = 0x19,
    gb_ROM_MBC5_RUMBLE = 0x19,
    gb_ROM_MBC5_RUMBLE_SRAM = 0x1D,
    gb_ROM_MBC5_RUMBLE_SRAM_BATT = 0x1E,
    gb_EXTDEV_PocketCamera = 0x1F,
    gb_EXTDEV_Bandai_TAMA5 = 0xFD,
    gb_EXTDEV_Hudson_HuC_3 = 0xFE,
    gb_EXTDEV_Hudson_HuC_1 = 0xFF,
};
//TODO: MBC1 memory mode selector
//TODO: MBC1 memory ROM/RAM bank selector
//TODO: MBC2 memory ROM bank selector


enum gb_cart_romsize {
    gb_ROM_x2_32KiB = 0x0, //2x 8KB banks
    gb_ROM_x4_64KiB = 0x1,
    gb_ROM_x4_128KiB = 0x2,
    gb_ROM_x16_256KiB = 0x3,
    gb_ROM_x32_512KiB = 0x4,
    gb_ROM_x64_1MiB = 0x5,
    gb_ROM_x128_2MiB = 0x6,
    //TODO: some special entries $52,$53,$54
};

enum gb_cart_ramsize {

    gb_RAM_x0_None = 0x0,
    gb_RAM_x1_2KiB = 0x1,
    gb_RAM_x1_8KiB = 0x2,
    gb_RAM_x4_32KiB = 0x3,
    gb_RAM_x16_128KiB = 0x4,
    //TODO: some special entries $52,$53,$54
};



struct gb_cart {
    byte_t const * buffer;
    size_t  size;
    enum gb_cart_romsize  ROM;
    enum gb_cart_ramsize  RAM;
    enum gb_cart_type Type;
};








//enum gb_cart_romsize_bit {
//    gb_ROM_x2_256Kbit = 0x0, //2x 8KB banks
//    gb_ROM_x4_512Kbit = 0x1,
//    gb_ROM_x4_1MBit = 0x2,
//    gb_ROM_x16_2MBit = 0x3,
//    gb_ROM_x32_4MBit = 0x4,
//    gb_ROM_x64_8MBit = 0x5,
//    gb_ROM_x128_16MBit = 0x6,
//    //TODO: some special entries $52,$53,$54
//};
//
//
//
//enum gb_cart_ramsize_bit {
//
//    gb_RAM_x0_None = 0x0,
//    gb_RAM_x1_16Kbit = 0x1,
//    gb_RAM_x1_64Kbit = 0x2,
//    gb_RAM_x4_256Kbit = 0x3,
//    gb_RAM_x16_1MKbit = 0x4,
//    //TODO: some special entries $52,$53,$54
//};