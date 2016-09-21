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


typedef struct { void* unused; } gbdisplay_h; 

//
// Time - specified in microseconds
//
//typedef int64_t gbtime_t;
typedef void (gbdisp_event_evt)(gbdisplay_h, void*);
typedef gbdisp_event_evt *gbdisp_event_pfn;


typedef struct {
    int   argc;
    void* argv;

    char *   window_name;
    uint32_t width;
    uint32_t height;
    uint8_t size_modifier;
    
    struct{
        gb_input_handler_pfn OnKeyInput;
        gbdisp_event_pfn OnPrepareFrame;
        gbdisp_event_pfn OnRedraw;

    }callbacks;
    void * callback_context;


} gbdisp_config_s;


typedef union  {
    struct { uint8_t R, G, B, A; };
    uint32_t RGBA;
} gb_color_s;
static_assert(sizeof(gb_color_s) == 4, "DUPA");




errno_t gbdisp_init(gbdisp_config_s cfg, gbdisplay_h *handle);
void    gbdisp_run(gbdisplay_h handle);

//
// These functions should only be used from within the callbacks provided to gbdisp
// Not worth implementing multithreaded screen for this shit
//
void    gbdisp_putpixel(gbdisplay_h handle, uint8_t x, uint8_t y, gb_color_s c);
void    gbdisp_buffer_set_ready(gbdisplay_h handle);
bool    gbdisp_is_buffer_ready(gbdisplay_h handle);
void    gbdisp_stop(gbdisplay_h handle);


uint8_t gb_DISPLAY_render_line(gbdisplay_h disp, byte_t ticks_delta);


#define GB_SCREEN_W 160
#define GB_SCREEN_H 144
#define GB_FPS_LIMIT 60
#define GB_VRAM_W 256
#define GB_VRAM_H 256


#define GB_TILE_WIDTH 8
#define GB_TILE_BYTES (GB_TILE_WIDTH * 2) // 2 bytes per line
#define GB_OBJ_X_OFFSET GB_TILE_WIDTH
#define GB_OBJ_Y_OFFSET (GB_TILE_WIDTH * 2)



//LCDC Bits
//Bit 7 - LCD Display Enable(0 = Off, 1 = On)
//Bit 6 - Window Tile Map Display Select(0 = 9800 - 9BFF, 1 = 9C00 - 9FFF)
//Bit 5 - Window Display Enable(0 = Off, 1 = On)
//Bit 4 - BG & Window Tile Data Select(0 = 8800 - 97FF, 1 = 8000 - 8FFF)
//Bit 3 - BG Tile Map Display Select(0 = 9800 - 9BFF, 1 = 9C00 - 9FFF)
//Bit 2 - OBJ(Sprite) Size(0 = 8x8, 1 = 8x16)
//Bit 1 - OBJ(Sprite) Display Enable(0 = Off, 1 = On)
//Bit 0 - BG Display(for CGB see below) (0 = Off, 1 = On)


#define GB_IO_LCDC 0xFF40 // LCD Controller Register

#define GB_IO_SCY  0xFF42 // Scroll X (buffer:256, screen:144)
#define GB_IO_SCX  0xFF43 // Scroll Y (buffer:256, screen:160)
#define GB_IO_LY   0xFF44 // Y line currently transferred to LCD
#define GB_IO_LYC  0xFF45 // Y line compare. LYC=LY flag is set if equal to LY
#define GB_IO_BCPS 0xFF68 //
#define GB_IO_BCPD 0xFF69
#define GB_IO_OCPS 0xFF6A
#define GB_IO_OCPD 0xFF6B
#define GB_IO_WY   0xFF4A // Window Y
#define GB_IO_WX   0xFF4B // Window X

#define GB_IO_BGP  0xFF47
#define GB_IO_OBP0 0xFF48
#define GB_IO_OBP1 0xFF49

#define GB_VRAM_BGMAP1_BEGIN 0x9800
#define GB_VRAM_BGMAP2_BEGIN 0x9C00
#define GB_VRAM_BGMAP_BYTES   0x3FF

#define GB_VRAM_TILES1_BEGIN 0x8800
#define GB_VRAM_TILES2_BEGIN 0x8000
#define GB_VRAM_TILES_BYTES   0xFFF


#define GB_OBJ_MAP_BEGIN 0xFE00
#define GB_OBJ_TILES_BEGIN GB_VRAM_TILES2_BEGIN
#define GB_OBJ_COUNT        40
#define GB_OBJ_MAX_PER_LINE 10




#pragma pack(1)

typedef 
struct gb_CHR_tile_s {
    uint8_t row[8][2];
}gb_CHR_tile_s;


typedef 
struct  gb_OBJ_sprite_s {
    uint8_t y;
    uint8_t x;
    uint8_t tile_no; // 8000h-8FFFh 
    /*In 8x16 mode, the lower bit of the tile number is ignored.Ie.the upper 8x8
        tile is "NN AND FEh", and the lower 8x8 tile is "NN OR 01h".*/
    uint8_t attrib;

}gb_OBJ_s;



static_assert(sizeof(gb_CHR_tile_s) == 8 * 2, "invalid tile size");
static_assert(sizeof(gb_OBJ_s) == sizeof(uint8_t) * 4, "invalid sprite size");


#pragma pack()
