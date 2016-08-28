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
#include "gamebore.h"
#include <string.h>
#include <assert.h>



static gb_color_s DMG_palette[5] = {
    { 255, 255, 255, 255 }, // Off (white)
    { 192, 192, 192, 255 }, // 33% on
    { 96, 96, 96, 255 }, // 66% on 
    { 0, 0, 0, 255 }, // On (black)
    { 0, 0, 0, 0}, //Transparent
};


typedef
struct gb_screen_state_s
{
    uint8_t LY;
    uint8_t SCX, SCY;
    uint8_t WX, WY;

    //LCDC inferred values
    bool     is_BG_display_enabled; 
    bool     is_OBJ_display_enabled;
    bool     is_Window_display_enabled;
    bool     is_CHR_map_index_signed;
    gb_addr_t CHR_map_addres;
    gb_addr_t CHR_tile_data_address;
    gb_addr_t WIN_map_address;

    //OBJ (Sprite) 
    uint8_t   OBJ_height_px;
    gb_addr_t OBJ_map_address;
    
    gb_LCDC_mode LCDC_mode;

    uint8_t STAT;
    uint8_t LCDC;
    

}gb_screen_state_s; 
static_assert(sizeof(gb_screen_state_s) == 28, "got 28bytes yo");


gb_screen_state_s gb_DISPLAY_get_state(void) {
    //
    // Do a snapshot of current display and LCD controller state
    // and wrap it all in struct so we can pass it around conveniently
    //


    // LCD Control Register:
    gb_word_t const LCDC = *GB_MMU_ACCESS_INTERNAL(GB_IO_LCDC);
    //LCDC Bits
    //Bit 7 - LCD Display Enable(0 = Off, 1 = On)
    //Bit 6 - Window Tile Map Display Select(0 = 9800 - 9BFF, 1 = 9C00 - 9FFF)
    //Bit 5 - Window Display Enable(0 = Off, 1 = On)
    //Bit 4 - BG & Window Tile Data Select(0 = 8800 - 97FF, 1 = 8000 - 8FFF)
    //Bit 3 - BG Tile Map Display Select(0 = 9800 - 9BFF, 1 = 9C00 - 9FFF)
    //Bit 2 - OBJ(Sprite) Size(0 = 8x8, 1 = 8x16)
    //Bit 1 - OBJ(Sprite) Display Enable(0 = Off, 1 = On)
    //Bit 0 - BG Display(for CGB see below) (0 = Off, 1 = On)
    uint8_t OBJ_height = BIT(LCDC, 2) ? 16 : 8;
    gb_addr_t BG_map_base = BIT(LCDC, 3) ? GB_VRAM_BGMAP2_BEGIN : GB_VRAM_BGMAP1_BEGIN;
    gb_addr_t BG_tile_data_base = BIT(LCDC, 4) ? GB_VRAM_TILES2_BEGIN : GB_VRAM_TILES1_BEGIN;
    gb_addr_t Win_map_base = BIT(LCDC, 6) ? GB_VRAM_BGMAP2_BEGIN : GB_VRAM_BGMAP1_BEGIN;
    bool is_BG_map_signed = !BIT(LCDC, 4);


    return (gb_screen_state_s) {
        .LY = *GB_MMU_ACCESS_INTERNAL(GB_IO_LY), // Currently transfered Y line , 
            .SCX = *GB_MMU_ACCESS_INTERNAL(GB_IO_SCX), // BG Scroll X (0x00-0xFF)
            .SCY = *GB_MMU_ACCESS_INTERNAL(GB_IO_SCY), // BG Scroll Y (0x00-0xFF)  
            .WY = *GB_MMU_ACCESS_INTERNAL(GB_IO_WY), // Window-Y coordinate (0x0-0x8F)
            .WX = *GB_MMU_ACCESS_INTERNAL(GB_IO_WX), // Window-X coordinate (0x7-0xA6)
            .STAT = *GB_MMU_ACCESS_INTERNAL(GB_IO_STAT), // LCDC Status Flag
            .LCDC = LCDC,
            .is_BG_display_enabled = BIT(LCDC, 0),
            .is_OBJ_display_enabled = BIT(LCDC, 1),
            .is_Window_display_enabled = BIT(LCDC, 5),
            .is_CHR_map_index_signed = is_BG_map_signed,
            .CHR_map_addres = BG_map_base,
            .CHR_tile_data_address = BG_tile_data_base,
            .WIN_map_address = Win_map_base,
            .OBJ_height_px = OBJ_height,
            .OBJ_map_address = GB_OBJ_MAP_BEGIN,
    };
}



bool gb_DISPLAY_can_scan() {
    //
    // Function to check whether we should render the current LY line.
    //
    static int last_LY; //later move to separate display struct (not gbdisp_config_s)
    gb_word_t const LY = *GB_MMU_ACCESS_INTERNAL(GB_IO_LY); // Currently transfered Y line 

    //Allow for scan only once per visible line
    if (LY == last_LY || LY >= GB_SCREEN_H) {
        return false;
    }

    //Allow only for OAM READ / VRAM READ
    if (g_GB.interrupts.display_mode != gb_LCDC_OAM_READ
        && g_GB.interrupts.display_mode != gb_LCDC_VRAM_READ) {
        return false;
    }

    last_LY = LY;
    return true;
}


///////////////////////////////////////////////////////////////////
//
// Internal sprite and tile helpers
//
//////////////////////////////////////////////////////////////////

__forceinline
gb_color_s gb_DISPLAY_get_DMG_color(gb_addr_t palette_addr, uint8_t color_n) {

    StopIf(color_n > 3, abort(), "out of pallete range");
    uint8_t palette = *GB_MMU_ACCESS_INTERNAL(palette_addr);
    uint8_t i = BIT_RANGE_GET(palette, color_n*2, color_n*2 + 1);
    return DMG_palette[i];
}


__forceinline
uint8_t gb_DISPLAY_get_tile_px(gb_addr_t const tile_addr, uint8_t const x, uint8_t const y) {
    /*
    This is "raw" function for taking color number out of arbitrary sprite/tile.
    Use more specific functions for tiles(CHR) and sprites(OBJ): 
        * gb_DISPLAY_get_CHR_px 
        * gb_DISPLAY_get_OBJ_px 
    */
    StopIf(x >= 8, return 0, "no tile or sprite can be wider than 8px");
    StopIf(y >= 8, return 0, "no tile or sprite can be taller than 8px");
    //
    // this function returns color number for given x,y coordinates of sprite\tile (OBJ\CHR)
    //
    gb_word_t const line_lo = *GB_MMU_ACCESS_INTERNAL(tile_addr + y * 2);
    gb_word_t const line_hi = *GB_MMU_ACCESS_INTERNAL(tile_addr + y * 2 + 1);

    uint8_t const bit_hi = BIT_GET_N(line_hi, (7 - x));
    uint8_t const bit_lo = BIT_GET_N(line_lo, (7 - x));

    uint8_t const color_n = (bit_hi << 1) | bit_lo;
    return color_n;
}


__forceinline
gb_color_s gb_DISPLAY_get_CHR_px(gb_screen_state_s const*const s, gb_word_t CHR_no, uint8_t x, uint8_t y) {
    
    //If CHR_no is signed value then XOR with 0x80:
    gb_word_t const adjusted_CHR_no = (s->is_CHR_map_index_signed) ? (CHR_no ^ 0x80) : (CHR_no);
    gb_addr_t const chr_data = s->CHR_tile_data_address + (adjusted_CHR_no * GB_TILE_BYTES);

    uint8_t color_n = gb_DISPLAY_get_tile_px(chr_data, x, y);
    
    return gb_DISPLAY_get_DMG_color(GB_IO_BGP, color_n);
}

__forceinline
gb_color_s gb_DISPLAY_get_OBJ_px(gb_screen_state_s const*const s, gb_OBJ_s sprite, uint8_t x, uint8_t y) {

    bool const flip_x = BIT(sprite.attrib, 5);
    bool const flip_y = BIT(sprite.attrib, 6);
    bool const behind_bg = BIT(sprite.attrib, 7);
    UNUSED(behind_bg); //TODO: IMPLEMENT BG PRIORITY BLENDING

    gb_addr_t const dmg_palette_addr = BIT(sprite.attrib, 4) ? GB_IO_OBP0 : GB_IO_OBP1;
    uint8_t const sprite_y = flip_y ? (s->OBJ_height_px - 1) - y : y;
    uint8_t const sprite_x = flip_x ? ((GB_TILE_WIDTH - 1) - x) : x;
    
    uint8_t tile_no;

    if (s->OBJ_height_px == GB_TILE_WIDTH) { //8x8 sprites
        tile_no = sprite.tile_no;

    } else { //8x16 sprites (made of 2x 8x8 tile)
        if (y < 8) tile_no = sprite.tile_no & 0xFE; //upper 
        else       tile_no = sprite.tile_no | 0x01; //lower
        y %= 8;
    }
    gb_addr_t const tile_address = GB_VRAM_TILES2_BEGIN + (tile_no * GB_TILE_BYTES);

    uint8_t color_n = gb_DISPLAY_get_tile_px(tile_address, sprite_x, sprite_y);
    if (0x00 == color_n) {
        return DMG_palette[4]; //transparent
    }else{
        return gb_DISPLAY_get_DMG_color(dmg_palette_addr, color_n);
    }
}


/*
Sprite Priorities and Conflicts
When sprites with different x coordinate values overlap, the one with the
smaller x coordinate (closer to the left) will have priority and appear above
any others. This applies in Non CGB Mode only.
When sprites with the same x coordinate values overlap, they have priority
according to table ordering. (i.e. $FE00 - highest, $FE04 - next highest,
etc.) In CGB Mode priorities are always assigned like this.

Only 10 sprites can be displayed on any one line. When this limit is exceeded,
the lower priority sprites (priorities listed above) won't be displayed. To
keep unused sprites from affecting onscreen sprites set their Y coordinate to
Y=0 or Y=>144+16. Just setting the X coordinate to X=0 or X=>160+8 on a sprite
will hide it but it will still affect other sprites sharing the same lines.
*/



void gb_DISPLAY_sort_sprites_X(gb_OBJ_s sprites[], uint_fast8_t sprites_n) { //insertion sort

    uint_fast8_t i,j;
    gb_OBJ_s tmp;

    for (i = 1; i < sprites_n; i++) {
        tmp = sprites[i];
        for (j = i; (j > 0) && (tmp.x < sprites[j - 1].x); j--) {
            
            sprites[j] = sprites[j - 1];
        }
        sprites[j] = tmp;
    }
}


uint8_t gb_DISPLAY_collect_intersecting_sprites(gb_screen_state_s const*const s, gb_OBJ_s out_sprites[]) {

    
    // Collect OBJs (Sprites) intersecting currently drawn Y line (s.LY)
    // and put them into `out_sprites` array they will be further processed
    uint8_t sprites_n = 0;
    //
    // We want to start with the last OBJ sprite for easier resolving of sprite priorities with same X 
    //
    for (int_fast8_t i = GB_OBJ_COUNT - 1; i >= 0; i--) {

        gb_addr_t const obj_address = s->OBJ_map_address + i * sizeof(gb_OBJ_s);
        gb_OBJ_s const * const sprite = (gb_OBJ_s*)GB_MMU_ACCESS_INTERNAL(obj_address);
        
        
        // Check if the sprite is on screen, only check Y (as X will still affect ordering)
        // Sprites have X(8) and Y(16) offsets to allow for drawing them partially offscreen

        if (sprite->y == 0  
            || sprite->y >= GB_SCREEN_H + GB_OBJ_Y_OFFSET
            || sprite->y + s->OBJ_height_px < GB_OBJ_Y_OFFSET) continue;
        
        assert(sprite->y - GB_OBJ_Y_OFFSET < GB_SCREEN_H);

        // Above check should prevent any negative values here, hence we cast to uint
        uint8_t const onscreen_y = sprite->y - GB_OBJ_Y_OFFSET; 
        

        // Check if the sprite intersects current LY
        if ((onscreen_y + s->OBJ_height_px) <= s->LY || onscreen_y > s->LY) continue;

        out_sprites[sprites_n++] = *sprite;
    }
    return sprites_n;
}


void gb_DISPLAY_render_sprite_line(gb_screen_state_s const*const s, gbdisplay_h disp, gb_OBJ_s sprite) {

    
    // Get Y coordinate of sprite that corresponds to LY
    // GB_OBJ_Y_OFFSET: Sprites' Y has 16px offset to allow for drawing partially off screen
    uint8_t sprite_y = s->LY - (sprite.y - GB_OBJ_Y_OFFSET);

    assert(sprite_y >= 0);
    assert(sprite_y < s->OBJ_height_px);

    for (int16_t i = 0; i < GB_TILE_WIDTH; i++) {
        
        int16_t const screen_x = i + (sprite.x - GB_OBJ_X_OFFSET); 
        int16_t const sprite_x = i;
        //
        // Skip negative X, break on off-screen
        //
        if (screen_x < 0) continue;
        if (screen_x >= GB_SCREEN_W) break;
        gb_color_s const c = gb_DISPLAY_get_OBJ_px(s, sprite, (uint8_t)sprite_x, (uint8_t)sprite_y);

        gbdisp_putpixel(disp, (uint8_t)screen_x, s->LY, c);
    }
}


uint8_t gb_DISPLAY_render_line(gbdisplay_h disp, byte_t ticks_delta) 
/*
Render currently processed line (according to LY register). 
Return true if we reached last screen line, false otherwise
*/
{
    UNUSED(ticks_delta);


    if (!gb_DISPLAY_can_scan()) return 0;
    gb_screen_state_s s = gb_DISPLAY_get_state();

    uint8_t const vram_y = (s.SCY + s.LY) % (GB_VRAM_H);

    //For each visible pixel (screen_x)
    for (uint_fast8_t x = 0; x < GB_SCREEN_W; x++) {

        uint8_t const vram_x        = (s.SCX + x) % (GB_VRAM_W);
        uint8_t const tilemap_X     = (vram_x / 8);
        uint8_t const tilemap_Y     = (vram_y / 8);
        uint16_t const tile_map_idx = (tilemap_Y * 32 + tilemap_X);

        gb_word_t const CHR_code = *GB_MMU_ACCESS_INTERNAL(s.CHR_map_addres + tile_map_idx);

        //x,y pixel within the tile
        uint8_t const tile_x = vram_x % 8;
        uint8_t const tile_y = vram_y % 8;

        gb_color_s const c = gb_DISPLAY_get_CHR_px(&s, CHR_code, tile_x, tile_y);

        
        gbdisp_putpixel(disp, x, s.LY, c);
    }

    
    static gb_OBJ_s sprites[GB_OBJ_COUNT];

    //
    //Collect all OBJs intersecting with LY 
    //
    uint8_t const sprites_n = gb_DISPLAY_collect_intersecting_sprites(&s, sprites);
    //
    //Sort inplace the intersecting sprites along Y.
    //
    gb_DISPLAY_sort_sprites_X(sprites, sprites_n);

    //
    // Render current line of all `GB_OBJ_MAX_PER_LINE` sprites
    //
    for (uint_fast8_t i = 0; i < sprites_n && i < GB_OBJ_MAX_PER_LINE; i++) {
        
        gb_DISPLAY_render_sprite_line(&s, disp, sprites[i]);

    }

    if ((GB_SCREEN_H - 1) == s.LY) { //done with last line, buffer switch
        gbdisp_buffer_set_ready(disp);
    }
    return s.LY;
}