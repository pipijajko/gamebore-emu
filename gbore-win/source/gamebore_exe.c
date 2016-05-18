// gamebore_exe.cpp : Defines the entry point for the console application.


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <windows.h>
#include <gl/gl.h>
#include <gl/GLU.h>

#include "gamebore.h"

//Initialize Stopif macro
char error_mode = '\0';
FILE *error_log = NULL;


void gb_wariat(gbdisplay_h disp, void* context); //TEMP

gbdisp_event_evt machine_step;

int main(int argc, char *argv[])

{
    uint8_t *ROM      = NULL;
    size_t   ROM_size = 0;
    FILE    *fp       = NULL;
    size_t   fsize    = 0;
    errno_t  e        = 0;

    for (int i = 0; i < argc; i++) {
        printf("argv[%d]:%s\n", i, argv[i]);
    }

    if (argc > 1) {
        e = fopen_s(&fp, argv[1], "rb");
        if (e || !fp) goto cleanup_generic;

        // determine file size
        fseek(fp, 0L, SEEK_END);
        fsize = ftell(fp);
        rewind(fp);

        //TODO: add real checks
        assert(NULL == ROM);
        assert(fsize < 1024 * 1024); //it's gameboy after all

        ROM = (uint8_t*)malloc(fsize);
        if (!ROM) goto cleanup_generic;

        ROM_size = fread_s(ROM, fsize, 1, fsize, fp);
        assert(ROM_size == fsize);
    }


    /////////////////////////////////////////////
    // Initialize MMU, CPU and Cartdridge data
    /////////////////////////////////////////////
    gb_initialize(ROM, ROM_size);

    //
    gbdbg_mute_print(g_GB.dbg, true);
    

    //
    // Initialize program window 
    // Seed main program loop with IdleCallback
    //
    
    gbdisp_config_s configuration =
    {
        argc, argv,
        .height           = GB_SCREEN_H,
        .width            = GB_SCREEN_W,
        .size_modifier    = 2,
        .window_name      = "GameBore",
        .callbacks.OnIdle = machine_step,
        .callbacks.OnRedraw = gb_wariat,
        .callback_context = NULL,
    };
    gbdisplay_h display ;
    e = gbdisp_init(configuration, &display);
    if (e) goto cleanup_generic;

    gbdisp_run(display);

    
    //gbdisp_destroy()

cleanup_generic:

    printf("e:%d,errno:%d", e, errno);
    if (ROM) free(ROM);
    if (fp) fclose(fp);
    gb_destroy();

    return 0;
}



void 
gb_wariat(gbdisplay_h disp, void* context) {

    UNUSED(context);

    static int prev_mode;
    if ((g_GB.interrupts.display_mode != gb_LCDC_OAM_READ)) {
        return;
    }
    prev_mode = g_GB.interrupts.display_mode;
    //if (g_GB.interrupts.display_mode != gb_LCDC_OAM_READ) return;

    // display driving registers:
    //LCDC Bits
    //Bit 7 - LCD Display Enable(0 = Off, 1 = On)
    //Bit 6 - Window Tile Map Display Select(0 = 9800 - 9BFF, 1 = 9C00 - 9FFF)
    //Bit 5 - Window Display Enable(0 = Off, 1 = On)
    //Bit 4 - BG & Window Tile Data Select(0 = 8800 - 97FF, 1 = 8000 - 8FFF)
    //Bit 3 - BG Tile Map Display Select(0 = 9800 - 9BFF, 1 = 9C00 - 9FFF)
    //Bit 2 - OBJ(Sprite) Size(0 = 8x8, 1 = 8x16)
    //Bit 1 - OBJ(Sprite) Display Enable(0 = Off, 1 = On)
    //Bit 0 - BG Display(for CGB see below) (0 = Off, 1 = On)
    gb_word_t * const LCDC = GB_MMU_ACCESS_INTERNAL(GB_IO_LCDC); // LCD Control Register
    gb_word_t * const STAT = GB_MMU_ACCESS_INTERNAL(GB_IO_STAT); // LCDC Status Flag
    UNUSED(STAT);
    _Bool trigger = false;


    //gb_word_t * const LY = gb_MMU_access(&g_GB, GB_IO_LY, 'w'); // LCDC Y(0x00-0x99) (currently transfered Y line + vblank)
    //gb_word_t * const LYC = gb_MMU_access(&g_GB, GB_IO_LYC, 'w'); // LY Compare if LYC == LY, set MATCH in STAT

    //gb_word_t * const BCPS = gb_MMU_access(&g_GB, GB_IO_BCPS, 'w'); // Specifies a BG write
    //gb_word_t * const BCPD = gb_MMU_access(&g_GB, GB_IO_BCPD, 'w'); // Specifies the BG write data

    //gb_word_t * const OCPS = gb_MMU_access(&g_GB, GB_IO_OCPS, 'w'); // Specifies a OBJ write
    //gb_word_t * const OCPD = gb_MMU_access(&g_GB, GB_IO_OCPD, 'w'); // Specifies the OBJ write data

    //gb_word_t * const SCY = gb_MMU_access(&g_GB, GB_IO_SCY, 'w'); // BG Scroll Y (0x00-0xFF) 
    //gb_word_t * const SCX = gb_MMU_access(&g_GB, GB_IO_SCX, 'w'); // BG Scroll X (0x00-0xFF)

    //gb_word_t * const WY = gb_MMU_access(&g_GB, GB_IO_WY, 'w'); // Window-Y coordinate (0x0-0x8F)
    //gb_word_t * const WX = gb_MMU_access(&g_GB, GB_IO_WX, 'w'); //  Window-X coordinate (0x7-0xA6)


    //Display Background 
    //256x256 (aka 32x32 tiles of 8x8) of this only 160x144 can be displayed (via SCX,SCY)
    static gb_color_s VRAM[256][256] = { 0 };
    static gb_color_s DMG_palette[4] = {
                    { 255, 255, 255, 255 }, // Off
                    { 192, 192, 192, 0 }, // 33% on
                    { 96, 96, 96, 0 }, // 66% on 
                    { 0, 0, 0, 0 }, // On
    };

    gb_addr_t BG_map_base = (BIT_GET_N(*LCDC, 3) == 0) 
                            ? GB_VRAM_BGMAP1_BEGIN 
                            : GB_VRAM_BGMAP2_BEGIN; //32x32 tiles
    
    gb_addr_t tile_data_base = (BIT_GET_N(*LCDC, 4) == 0) 
                               ? GB_VRAM_TILES1_BEGIN 
                               : GB_VRAM_TILES2_BEGIN;
    
    _Bool signed_chr = (BIT_GET_N(*LCDC, 4) == 0);
    
    // for GBC bank switch is required to check CHR code attributes

    for (uint_least8_t chr_y = 0; chr_y < 32; chr_y++) {
        for (uint_least8_t chr_x = 0; chr_x < 32; chr_x++) {

            gb_addr_t const chr_index_address = BG_map_base + 32 * chr_y + chr_x;
            gb_word_t chr_index = *GB_MMU_ACCESS_INTERNAL(chr_index_address);

            
            if (signed_chr) {
                chr_index = (128)+(int8_t)chr_index;
            }

            uint_least8_t y = 0;
            for (uint_least8_t chr_byte = 0; chr_byte < 16; chr_byte +=2, y++) {

                gb_word_t const chr_size = 8 * 2;
                gb_addr_t const chr_address = tile_data_base + (chr_index * chr_size);
                
                // each line is 2 bytes
                gb_word_t const l1 = *GB_MMU_ACCESS_INTERNAL(chr_address + chr_byte);
                gb_word_t const l2 = *GB_MMU_ACCESS_INTERNAL(chr_address + chr_byte + 1);

                for (uint_least8_t x = 0; x < 8; x++) {

                    uint8_t screen_y = chr_y * 8 + y;
                    uint8_t screen_x = chr_x * 8 + x;

                    //does it make sense to do mapping?
                    uint8_t bit_hi = BIT_GET_N(l2, (7 - x));
                    uint8_t bit_lo = BIT_GET_N(l1, (7 - x));
                    uint8_t   color_n = (bit_hi << 1) | bit_lo;
                    if (l1 || l2) {
                        trigger = true;
                    }

                    VRAM[screen_y][screen_x] = DMG_palette[color_n];
                    gbdisp_putpixel(disp, screen_x, screen_y, DMG_palette[color_n]);
                }

            }
        }
    }
    
    



    //static i, j;
    //j++;
    //for (int y = 0; y < GB_SCREEN_H; y++) {
    //    for (int x = 0; x < GB_SCREEN_W; x++) {
    //        if (x == (i) % GB_SCREEN_W || y == j%GB_SCREEN_H) {
    //            gbdisp_putpixel(disp, x, y, (gb_color_s) { .RGBA = 0x0000000 });
    //        }
    //        else {
    //            gbdisp_putpixel(disp, x, y, (gb_color_s) { .RGBA = 0xFFFFFFFF });
    //        }
    //    }
    //}
    //i++;
    if (trigger) {
        gbdisp_buffer_ready(disp);
    }

}

void machine_step(gbdisplay_h disp, void* context) {
    
    
    byte_t ticks = gb_CPU_step(); //gb_CPU uses global instance g_GB
    gb_MMU_step(&g_GB);
    gb_INTERRUPT_step(ticks); //gb_CPU uses global instance g_GB

    gb_wariat(disp, context);



}