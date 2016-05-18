#include "stdafx.h"
#include "gamebore.h"





#define GB_TICKS_HBLANK    204   // MODE 00 average (201-207 ticks)
#define GB_TICKS_VBLANK    456   // MODE 01 (456 ticks per LY increment)
#define GB_TICKS_OAM_READ  80    // MODE 10 average (77-83 ticks)
#define GB_TICKS_VRAM_READ 171   // MODE 11 average (169-175 ticks)

#define GB_TICKS_MINIMAL    GB_TICKS_OAM_READ

#define GB_LY_MAX 153 //144 display lines + 10 lines for VBLANK

#define GB_LY_INCREMENT(LYval) (((LYval) + 1) % (GB_LY_MAX + 1))





void gb_INTERRUPT_request(byte_t const interrupt_signal_flag) {

    gb_word_t *const IF = GB_MMU_ACCESS_INTERNAL(GB_IO_IF);
    StopIf(interrupt_signal_flag > GB_INT_FLAG_KEYPAD, 
           abort(),
           "invalid INT signal:%hhu", interrupt_signal_flag);

    *IF |= interrupt_signal_flag; //Set Interrupt Request Flag

}


void gb_INTERRUPT_execute(void) {

    gb_word_t *const IF = GB_MMU_ACCESS_INTERNAL(GB_IO_IF);
    gb_word_t *const IE = GB_MMU_ACCESS_INTERNAL(GB_IO_IE);
    _Bool      const IME = g_GB.interrupts.IME;

    byte_t requested_interupts = (*IE) & (*IF);

    if (IME && requested_interupts != 0) {
        // One or more of the requested interrupts is enabled.
        g_GB.interrupts.IME = false;

        // Even though we have the interrupt signal flag given as an argument.
        // We must consider that another intterupt might have been requested, so
        // we evaluate them according to priority:

        gb_addr_t isr = 0x0000;

        for (int i = 0; i < GB_INT_NUM; i++) {

            if (BIT_GET_N(requested_interupts, i)) {

                *IF = BIT_RES_N(*IF, i); // Ok, reset the IF's `i` bit, we're going in!
                
                switch (1 << i) {
                case GB_INT_FLAG_VBLANK:    isr = GB_ISR_VBLANK; break;
                case GB_INT_FLAG_LCDC_STAT: isr = GB_ISR_LCDC_STATUS; break;
                case GB_INT_FLAG_TIMER:     isr = GB_ISR_TIMER_OVERFLOW; break;
                case GB_INT_FLAG_SERIAL:    isr = GB_ISR_SERIAL_XFER_COMPLETE; break;
                case GB_INT_FLAG_KEYPAD:    isr = GB_ISR_KEYPAD_HI_TO_LO; break;
                default:
                    //add trace
                    abort();
                }
            }
            if (isr) break;
        }
        gb_CPU_interrupt(isr); 
    }
}



void gb_INTERRUPT_step(byte_t ticks_delta) {


    g_GB.interrupts.last_opcode_ticks = ticks_delta;
    g_GB.interrupts.total_ticks += ticks_delta;
    g_GB.interrupts.display_modeclock += ticks_delta;


    //just for debug
    gb_word_t const * const IE = GB_MMU_ACCESS_INTERNAL(GB_IO_IE);
    if (((*IE) & GB_INT_FLAG_TIMER) != 0) {
        gdbg_trace(g_GB.dbg, "timer enabled!");
    }
    if (((*IE) & GB_INT_FLAG_KEYPAD) != 0) {
        gdbg_trace(g_GB.dbg, "keypad enabled!");
    }



    // Get pointers to memory area with control registers
    gb_word_t const * const LYC  = GB_MMU_ACCESS_INTERNAL(GB_IO_LYC);
    gb_word_t const * const LCDC = GB_MMU_ACCESS_INTERNAL(GB_IO_LCDC);
    gb_word_t * const STAT       = GB_MMU_ACCESS_INTERNAL(GB_IO_STAT);
    gb_word_t * const LY         = GB_MMU_ACCESS_INTERNAL(GB_IO_LY);
    _Bool const LCDC_enabled     = BIT_GET_N(*LCDC, 7);
    _Bool const LCDC_toggled     = (LCDC_enabled != (g_GB.interrupts.display_mode != gb_LCDC_DISABLED));
    /*
    LCDC_enabled| current_mode      | (mode eval) | result   | LCDC toggled?
    0           |  DISABLED         |    0        |    0!=0  |   false
    0           |  (H/VBLANK,OAM..) |    1        |    0!=1  |   true
    1           |  DISABLED         |    0        |    1!=0  |   true
    1           |  (H/VBLANK,OAM..) |    1        |    1!=1  |   false
    */


    if  (LCDC_toggled){
        //
        // Detected LCD controller toggle:
        //

        if (LCDC_enabled) {
            g_GB.interrupts.display_mode = gb_LCDC_HBLANK;
            g_GB.interrupts.display_modeclock = 0;

        }else{
            g_GB.interrupts.display_mode = gb_LCDC_DISABLED;
            g_GB.interrupts.display_line = 0;
            g_GB.interrupts.display_modeclock = 0;
            *LY = 0x00;
        }
    }

    uint32_t const mode_ticks       = g_GB.interrupts.display_modeclock;
    gb_LCDC_mode const current_mode = g_GB.interrupts.display_mode;
    gb_LCDC_mode next_mode          = gb_LCDC_NO_CHANGE;
    byte_t STAT_event               = 0x00; //event for STAT interrupt
                              
    // Evaluate mode changes only if controller is enabled & least GB_TICKS_MINIMAL has passed.
    if (LCDC_enabled && mode_ticks >= GB_TICKS_MINIMAL) {

        switch (current_mode) {
        case gb_LCDC_VBLANK: // V-Blank
            if (mode_ticks >= GB_TICKS_VBLANK) {

                *LY = GB_LY_INCREMENT(*LY);

                if (*LY > 144) {
                    next_mode = gb_LCDC_VBLANK;
                    STAT_event = GB_INT_STAT_HBLANK_FLAG; //yes,
                }
                else {
                    next_mode = gb_LCDC_OAM_READ;
                    STAT_event = GB_INT_STAT_OAM_FLAG;
                }
            }
            break;
        case gb_LCDC_HBLANK: // H-Blank
            if (mode_ticks >= GB_TICKS_HBLANK) {

                *LY = GB_LY_INCREMENT(*LY);

                if (*LY == 144) {
                    next_mode = gb_LCDC_VBLANK;
                    STAT_event = GB_INT_STAT_VBLANK_FLAG;

                    gb_INTERRUPT_request(GB_INT_FLAG_VBLANK);
                }
                else {
                    next_mode = gb_LCDC_OAM_READ;
                    STAT_event = GB_INT_STAT_OAM_FLAG;
                }
            }
            break;
        case gb_LCDC_OAM_READ: // OAM read
            if (mode_ticks >= GB_TICKS_OAM_READ) {
                next_mode = gb_LCDC_VRAM_READ;
            }
            break;
        case gb_LCDC_VRAM_READ: // OAM+VRAM read
            if (mode_ticks >= GB_TICKS_VRAM_READ) {
                next_mode = gb_LCDC_HBLANK;
                STAT_event = GB_INT_STAT_HBLANK_FLAG;
            }
            break;
        default:break;
        }
    }

    // If LY == LYC:  Set the Coincidence flag, and set flag for matching STAT interrupt
    // Otherwise:     Clear coincidenc flag
    _Bool is_coincident = (*LY) == (*LYC);

    BIT_SETCLR_IF(is_coincident, *STAT, GB_STAT_COINCIDENCE_FLAG);
    STAT_event |= is_coincident ? (GB_INT_STAT_LYLYC_FLAG) : (0);
    
    
    // If check if we are expepcted to request STAT interrupt
    // for the LCD status events that occured:
    /*
    In addition, the register allows selection of 1 of the 4 types of interrupts from the LCD
controller. Executing a write instruction for the match flag resets that flag but does not change
the mode flag.
    
    */

    if (((*STAT) & STAT_event) != 0) {

        gb_INTERRUPT_request(GB_INT_FLAG_LCDC_STAT);
    }


    if (next_mode != gb_LCDC_NO_CHANGE) {

        char* modename = NULL;
        switch (next_mode) {
        case gb_LCDC_HBLANK:    modename = "H-BLANK"; break;
        case gb_LCDC_VBLANK:    modename = "V-BLANK"; break;
        case gb_LCDC_OAM_READ:  modename = "OAM READ"; break;
        case gb_LCDC_VRAM_READ: modename = "OAM+VRAM READ"; break;
        }
        gdbg_trace(g_GB.dbg, "DISPLAY::new_mode:%s,ticks:%u,LY:%hhu\n", 
                   modename,
                   g_GB.interrupts.display_modeclock,
                   *LY);
        
        g_GB.interrupts.display_mode = next_mode;
        g_GB.interrupts.display_modeclock = 0;


        if (((*STAT) & 0b11) != (byte_t)current_mode) {

            gdbg_trace(g_GB.dbg, "DISPLAY::current_mode:%d doesn't match STAT MODE FLAG:%hhu",
                       current_mode, ((*STAT) & 0b11));

        }

        //Clear & Update MODE FLAG in STAT
        *STAT &= ~0b11; 
        *STAT |= (next_mode & 0b11);
    }


    //
    //Execute requested interupts:
    //
    gb_INTERRUPT_execute(); 
}