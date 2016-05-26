#include "gamebore.h"



//gb_addr_t const BG_map_base = (BIT_GET_N(LCDC, 3) == 0)
//? GB_VRAM_BGMAP1_BEGIN
//    : GB_VRAM_BGMAP2_BEGIN;

static gb_color_s DMG_palette[4] = {
    { 255, 255, 255, 255 }, // Off
    { 192, 192, 192, 0 }, // 33% on
    { 96, 96, 96, 0 }, // 66% on 
    { 0, 0, 0, 0 }, // On
};


//uint8_t gb_DISPLAY_translate_coordinates() {
//
//}
//
//struct {
//    struct { uint8_t x, y } vram;
//    struct { uint8_t x, y } screen;
//    struct { uint8_t x, y, number } chr_tile; 
//    //gb_addr_t tiles_base;
//
//};




//gb_DISPLAY_decompose_vram_coords()

gb_color_s gb_DISPLAY_get_tile_pixel(gb_addr_t tiles_base, uint8_t tile_no, uint8_t x, uint8_t y) {

    gb_word_t const tile_size = 8 * 2;
    _Bool const is_signed = (tiles_base == GB_VRAM_TILES1_BEGIN); //is tile_no signed or not?

    if (is_signed) {
        tile_no = ((128) + (int8_t)tile_no);
    }
    gb_addr_t const chr_address = tiles_base + (tile_no * tile_size);
    //
    // each 8x8 sprite line is encoded by 2 bytes (high and low): 2bits per pixel.
    //
    gb_word_t const line_lo = *GB_MMU_ACCESS_INTERNAL(chr_address + y*2);
    gb_word_t const line_hi = *GB_MMU_ACCESS_INTERNAL(chr_address + y*2 + 1);

    uint8_t const bit_hi = BIT_GET_N(line_hi, (7 - x));
    uint8_t const bit_lo = BIT_GET_N(line_lo, (7 - x));

    uint8_t const color_n = (bit_hi << 1) | bit_lo;
    return DMG_palette[color_n];
}


//should this be moved to DISPLAY?
void gb_DISPLAY_render_line(gbdisplay_h disp, byte_t ticks_delta) {

    UNUSED(ticks_delta);
    static gb_LCDC_mode prev_mode; //later move to separate display struct (not gbdisp_config_s)
    gb_LCDC_mode const current_mode = g_GB.interrupts.display_mode;

    //

    // We only want to enter once per mode (on mode change).
    // If mode is unchanged -> exit
    //
    if (prev_mode == current_mode) {
        return;
    }
    prev_mode = current_mode;

    gb_word_t const LCDC = *GB_MMU_ACCESS_INTERNAL(GB_IO_LCDC); // LCD Control Register

//if transition to VBLANK -> refresh display
    if (current_mode == gb_LCDC_VBLANK) {

        //printf("\nvblank, LCDC:%x\n", LCDC);
        gbdisp_buffer_ready(disp);
        return;
    }
    // Not VRAM/OAM read -> exit
    if (current_mode != gb_LCDC_VRAM_READ) return;
    //printf("VRAM, LCDC:%x,\n", LCDC);

    if ((LCDC & 0x1) == 0) return; // BG drawing disabled


    gb_word_t const LY = *GB_MMU_ACCESS_INTERNAL(GB_IO_LY); // Currently transfered Y line 

    //
    // Y-coordinate is in V-BLANK period, nothing to do here
    //
    if (LY >= GB_SCREEN_H) return;


    //    gb_word_t const STAT = *GB_MMU_ACCESS_INTERNAL(GB_IO_STAT); // LCDC Status Flag

    //    gb_word_t const WY = *GB_MMU_ACCESS_INTERNAL(GB_IO_WY); // Window-Y coordinate (0x0-0x8F)
    //    gb_word_t const WX = *GB_MMU_ACCESS_INTERNAL(GB_IO_WX); // Window-X coordinate (0x7-0xA6)

    gb_word_t const SCY = *GB_MMU_ACCESS_INTERNAL(GB_IO_SCY); // BG Scroll Y (0x00-0xFF) 
    gb_word_t const SCX = *GB_MMU_ACCESS_INTERNAL(GB_IO_SCX); // BG Scroll X (0x00-0xFF)

    //LCDC Bits
    //Bit 7 - LCD Display Enable(0 = Off, 1 = On)
    //Bit 6 - Window Tile Map Display Select(0 = 9800 - 9BFF, 1 = 9C00 - 9FFF)
    //Bit 5 - Window Display Enable(0 = Off, 1 = On)
    //Bit 4 - BG & Window Tile Data Select(0 = 8800 - 97FF, 1 = 8000 - 8FFF)
    //Bit 3 - BG Tile Map Display Select(0 = 9800 - 9BFF, 1 = 9C00 - 9FFF)
    //Bit 2 - OBJ(Sprite) Size(0 = 8x8, 1 = 8x16)
    //Bit 1 - OBJ(Sprite) Display Enable(0 = Off, 1 = On)
    //Bit 0 - BG Display(for CGB see below) (0 = Off, 1 = On)

    // Get base address for Background tile mapping
    gb_addr_t const BG_map_base = (BIT_GET_N(LCDC, 3) == 0)
        ? GB_VRAM_BGMAP1_BEGIN
        : GB_VRAM_BGMAP2_BEGIN; //32x32 tiles

// Get base address for Tile data
    gb_addr_t const tile_data_base = (BIT_GET_N(LCDC, 4) == 0)
        ? GB_VRAM_TILES1_BEGIN
        : GB_VRAM_TILES2_BEGIN;
    // OAM only
    //uint8_t const tile_side_px = BIT_GET_N(LCDC, 3) == 0
    //                                        ? GB_TILE_8x8 
    //                                        : GB_TILE_32x32;

//    _Bool const is_signed_chr = BIT_GET_N(LCDC, 4) == 0; //is chr_index signed or not?


    //Display Background 
    //256x256 (aka 32x32 tiles of 8x8) of this only 160x144 can be displayed (via SCX,SCY)
    static gb_color_s VRAM[GB_VRAM_H][GB_VRAM_W] = { 0 };


    //uint8_t const screen_y = LY;
    //uint8_t const scr_x = i;


    uint8_t const vram_y = (SCY + LY) % (GB_VRAM_H);

    //For each visible pixel (screen_x)
    for (uint8_t i = 0; i < GB_SCREEN_W; i++) {

        uint8_t const vram_x = (SCX + i) % (GB_VRAM_W);
        uint8_t const tilemap_X = (vram_x / GB_TILE_8x8);
        uint8_t const tilemap_Y = (vram_y / GB_TILE_8x8);
        uint16_t const tile_map_idx = (tilemap_Y * 32 + tilemap_X);

        gb_word_t const tile_no = *GB_MMU_ACCESS_INTERNAL(BG_map_base + tile_map_idx);

        tile_data_base; 

        //x,y pixel within the tile
        uint8_t const tile_x = vram_x % GB_TILE_8x8;
        uint8_t const tile_y = vram_y % GB_TILE_8x8;

        gb_color_s const c = gb_DISPLAY_get_tile_pixel(GB_VRAM_TILES2_BEGIN, tile_no, tile_x, tile_y);

        VRAM[vram_y][vram_x] = c;
        gbdisp_putpixel(disp, i, LY, c);
    }
}
