#pragma once


typedef struct { void* unused; } gbdisplay_h; 

//
// Time - specified in microseconds
//
//typedef int64_t gbtime_t;
typedef struct gbdisp_input_s {
    //int32_t mouse_x, mouse_y;
    uint8_t key;
    uint8_t key_modifiers;
} gbdisp_input_s;


typedef void (gbdisp_event_evt)(gbdisplay_h, void*);
typedef void (gbdisp_input_evt)(gbdisplay_h, gbdisp_input_s, void*);
typedef gbdisp_event_evt *gbdisp_event_pfn;
typedef gbdisp_input_evt *gbdisp_input_pfn;


typedef struct {
    int   argc;
    void* argv;

    char *   window_name;
    uint32_t width;
    uint32_t height;
    uint8_t size_modifier;
    
    struct{
        gbdisp_event_pfn OnKeyInput;
        gbdisp_event_pfn OnIdle;
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
void    gbdisp_buffer_ready(gbdisplay_h handle);
void    gbdisp_stop(gbdisplay_h handle);


void gb_DISPLAY_render_line(gbdisplay_h disp, byte_t ticks_delta);


#define GB_SCREEN_W 160
#define GB_SCREEN_H 144
#define GB_FPS_LIMIT 60
#define GB_VRAM_W 256
#define GB_VRAM_H 256





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

#define GB_VRAM_BGMAP1_BEGIN 0x9800
#define GB_VRAM_BGMAP2_BEGIN 0x9C00
#define GB_VRAM_BGMAP_BYTES   0x3FF

#define GB_VRAM_TILES1_BEGIN 0x8800
#define GB_VRAM_TILES2_BEGIN 0x8000
#define GB_VRAM_TILES_BYTES   0xFFF

#define GB_TILE_8x8 8
#define GB_TILE_32x32 32




//struct gb_RGB{ uint8_t r,g,b};
//


//where to put general interrupt handling, CPU i guess?
//


/*
Notes:
GB display emulates CRT display. We have H-Blank after each line display, 
and V-Blank after all 144 lines been displayed.

GPU periods as counted by CPU T-clock @4194304 Hz.

Period	                    GPU mode    clocks
Scanline (accessing OAM)	2	        80
Scanline (accessing VRAM)	3	        172
Horizontal blank	        0	        204
One line (scan and blank)		        456
Vertical blank	            1	        4560 (10 lines)
Full frame (scans and vblank)		    70224

8000-87FF	Tile set #1: tiles 0-127
8800-8FFF	Tile set #1: tiles 128-255
            Tile set #0: tiles -1 to -128

9000-97FF	Tile set #0: tiles 0-127



//one tilemap can be displayed at a time
9800-9BFF	Tile map #0 --> 32x32 tiles -> 256x256 px
9C00-9FFF	Tile map #1 --> 32x32 tiles -> 256x256 px

Mapped as 16 bits

uint8_t chr_code;



-> CHR code (d.8 bits)
->



Each tile is 8x8x2(color/shade).

Only part of the active tile map is visible, this is defined by 
scrolly(from top) and scrollx(fromleft).

Background Palette register defines GB colors
Value	Pixel	Emulated colour
0	    Off	    [255, 255, 255]
1	    33% on	[192, 192, 192]
2	    66% on	[96, 96, 96]
3	    On	    [0, 0, 0]

BG pallete bits
Bits:   [7, 6][5, 4][3, 2][1, 0]
ColorN:  0b11  0b10  0b01  0b00



Row of tile pixels are stored VERTICALLY!

Byte1 & 2:[01101111][10101001]
Byte1:      [01101111]
Byte2:      [10101001]
            ---------- +
PixColor:   [12303221]

Display notes:
DMG has 1 x 8kB VRAM bank @ 0x8000 (CGB has another switchable by register VBK)
- Character data can be written to the 6144 bytes from 0x8000 to 0x97FF .
- By default, the area from 0x8000 to 0x8FFF is allocated for OBJ character data storage.
- The register LCDC can be used to select either 0x8000-0x8FFF or 0x8800-0x97FF as the area for storing BG and window character data.
- If the BG character data is allocated to 0x8000-0x8FFF, this data shares an area with OBJ data, and the
character dot data that corresponds to the CHR codes is also the same.




Tile sets are overlapping (makes sense)
*/

//static const uint32_t

typedef enum gb_gpu_mode_e{
    H_BLANK   = 0x0, // After the last hblank, push the screen data to canvas
    V_BLANK   = 0x1,  // Vblank (10 lines)
    OAM_Read  = 0x2, // OAM read mode, scanline active
    VRAM_Read = 0x3,   // VRAM read mode, scanline active - Treat end of mode 3 as end of scanline

}gb_gpu_mode_e;

typedef struct gb_gpu_s {
    //mode
    //mode_clock
    //scanline
    int mode, clock, scanline;

}gb_gpu_s;



//typedef struct gb_gpu_CHRMAP_s { //aka BG Display Data (data for 32x32 chr codes)
//    uint8_t CHR_code;
//    union {
//        struct{  //TODO: is the order good?
//            int display_priority : 1;
//            int UD_flip : 1;
//            int RL_flip : 1;
//            int reserved : 1;
//            int CHR_bank : 1;
//            int pallete : 3;
//        };
//
//    };
//
//} gb_gpu_CHARMAP_s;


typedef struct gb_gpu_CHR_s {
    uint8_t row[8][2];
}gb_gpu_CHR_s;

#define PX(b1,b2, n) (b1 << n)

#pragma pack(1)
typedef struct  gb_gpu_OBJ_s {
    uint8_t y, x;
    uint8_t chr;
    union {
        uint8_t attr;
        struct {
            int CGB_PALLETE   : 3; //CGB Pallete
            int CGB_CHAR_BANK : 1;
            int DMG_PALLETE : 1;
            int H_FLIP      : 1;
            int V_FLIP      : 1;
            int BG_PRIORITY : 1;
        }z;
    };

}gb_gpu_OBJ_s;

static_assert(sizeof(gb_gpu_OBJ_s) < 0x8, "invalid OBJ size");




#pragma pack()
