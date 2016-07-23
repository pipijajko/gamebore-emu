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



#define GB_TICKS_HBLANK    204   // MODE 00 average (201-207 ticks)
#define GB_TICKS_VBLANK    456   // MODE 01 (456 ticks per LY increment)
#define GB_TICKS_OAM_READ  80    // MODE 10 average (77-83 ticks)
#define GB_TICKS_VRAM_READ 171   // MODE 11 average (169-175 ticks)

#define GB_TICKS_MINIMAL    GB_TICKS_OAM_READ

#define GB_LY_MAX 153 //144 display lines + 10 lines for VBLANK

#define GB_LY_INCREMENT(LYval) (((LYval) + 1) % (GB_LY_MAX + 1))



void gb_INTERRUPT_request(byte_t const interrupt_signal_flag) 
{
    gb_word_t *const IF = GB_MMU_ACCESS_INTERNAL(GB_IO_IF);
    StopIf(interrupt_signal_flag > GB_INT_FLAG_KEYPAD, 
           abort(),
           "invalid INT signal:%hhu", interrupt_signal_flag);

    *IF |= interrupt_signal_flag; //Set Interrupt Request Flag

}


void gb_INTERRUPT_execute(void) 
{
    gb_word_t *const IF = GB_MMU_ACCESS_INTERNAL(GB_IO_IF);
    gb_word_t *const IE = GB_MMU_ACCESS_INTERNAL(GB_IO_IE);
    bool      const IME = g_GB.interrupts.IME;

    byte_t requested_interupts = ((*IE) & (*IF)) & GB_INT_ALL_MASK;

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
                gb_CPU_interrupt(isr);
                break;
            }
        }
        
    }
}

void gb_INTERRUPT_update_timers(void) //byte_t ticks_delta) ) 
{
    gb_interrupt_data_s * const interrupts = &g_GB.interrupts;

    uint64_t const total_ticks = interrupts->total_ticks; //shorthand

    //
    // DIV (Divisor) Timer 
    // Always running. 
    //
    // Since `gb_INTERRUPT_update_timers` call is running after every CPU instruction.
    // And no instruction is > 16 ticks, we can assume that at maximum one M-Clock
    // tick has passed - so we don't calc the difference between total and next_update ticks.
    //
    bool need_DIV_update  = total_ticks >= interrupts->DIV_next_update_ticks;

    if (need_DIV_update) {

        (*GB_MMU_ACCESS_INTERNAL(GB_IO_DIV))++;
        interrupts->DIV_next_update_ticks += GB_DIV_UPDATE_TICKS;
    }

    //
    // TIMA/TAC/TMA Configurable Timer
    //
    // Check timer control:
    gb_word_t const TAC    = *GB_MMU_ACCESS_INTERNAL(GB_IO_TAC);
    bool const is_timer_on = BIT(TAC, 2);


    if (is_timer_on) {
        bool need_TIMA_update;
        
        //
        // Detect if timer was just started
        //
        if (false == interrupts->TIMA_enabled) {

            need_TIMA_update = true;
            interrupts->TIMA_enabled = true;
            interrupts->TIMA_next_update_ticks = total_ticks;
        } else {

            need_TIMA_update = total_ticks >= interrupts->TIMA_next_update_ticks;
        }


        if (need_TIMA_update) {

            uint64_t tick_diff = total_ticks - interrupts->TIMA_next_update_ticks;
            uint64_t interval, increment;

            switch (BIT_RANGE_GET(TAC, 0, 1)) {

            case 0b00:  interval = GB_MCLOCK_UPDATE_TICKS * 64; 
                        increment = tick_diff >> 8;
                        break; //   4 096 KHz

            case 0b01:  interval = GB_MCLOCK_UPDATE_TICKS;       
                        increment = tick_diff >> 2;
                        break; // 262 133 KHz

            case 0b10:  interval = GB_MCLOCK_UPDATE_TICKS * 4;   
                        increment = tick_diff >> 4;
                        break; //  65 536 KHz

            case 0b11:  interval = GB_MCLOCK_UPDATE_TICKS * 16;  
                        increment = tick_diff >> 6;
                        break; //  16 384 KHz
            default:
                abort();
            }
            increment = 1; //this above is kind of stupid, diff is something else than you thought

            gb_word_t * const TIMA = GB_MMU_ACCESS_INTERNAL(GB_IO_TIMA);

            if (increment+(*TIMA) >= 0xFF) {

                // on TIMA overflow, generate interrupt and reinitialize with TMA
                gb_INTERRUPT_request(GB_INT_FLAG_TIMER);

                *TIMA = *GB_MMU_ACCESS_INTERNAL(GB_IO_TMA);

            } else {
                StopIf(increment > 0xFF, abort(), "Terribly bad timer increment calculation!");
                (*TIMA) += (gb_word_t)increment;
            }
            interrupts->TIMA_next_update_ticks += interval;
        } 
    } else {
        interrupts->TIMA_enabled = false;
    }

    //
    // How I avoided wraparound tracking with this one weird trick.
    //
    if (0x8000000000000000 & interrupts->total_ticks) {
        //
        // If the oldest bit is set in `total_ticks` 
        // rebase all tracked uint64 tick values
        // No value will be larger than total_ticks+0xFF.
        // This way there's no need to put any wraparound logic above.
        // 
        interrupts->total_ticks &= 0x7FFFFFFFFFFFFFFF;
        interrupts->TIMA_next_update_ticks &= 0x7FFFFFFFFFFFFFFF;
        interrupts->DIV_next_update_ticks  &= 0x7FFFFFFFFFFFFFFF;
    }

}


void gb_INTERRUPT_step(byte_t ticks_delta) 
{

    gb_interrupt_data_s * const interrupts = &g_GB.interrupts;
    interrupts->last_opcode_ticks = ticks_delta;
    interrupts->total_ticks += ticks_delta;
    interrupts->display_modeclock += ticks_delta;

    
    gb_INTERRUPT_update_timers();


    // Get pointers to memory area with control registers
    gb_word_t const * const LYC  = GB_MMU_ACCESS_INTERNAL(GB_IO_LYC);
    gb_word_t const * const LCDC = GB_MMU_ACCESS_INTERNAL(GB_IO_LCDC);
    gb_word_t * const STAT       = GB_MMU_ACCESS_INTERNAL(GB_IO_STAT);
    gb_word_t * const LY         = GB_MMU_ACCESS_INTERNAL(GB_IO_LY);
    bool const LCDC_enabled     = BIT_GET_N(*LCDC, 7);
    bool const LCDC_toggled     = (LCDC_enabled != (interrupts->display_mode != gb_LCDC_DISABLED));
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
            interrupts->display_mode = gb_LCDC_HBLANK;
            interrupts->display_modeclock = 0;

        }else{
            interrupts->display_mode = gb_LCDC_DISABLED;
            interrupts->display_line = 0;
            interrupts->display_modeclock = 0;
            *LY = 0x00;
        }
    }

    uint32_t const mode_ticks       = interrupts->display_modeclock;
    gb_LCDC_mode const current_mode = interrupts->display_mode;
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
    bool is_coincident = (*LY) == (*LYC);

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

        

        gdbg_trace(g_GB.dbg,"DISPLAY::new_mode:%s,ticks:%u,LY:%hhu,STAT:0x%x,LCDC:0x%x\n", 
                   modename,
                   interrupts->display_modeclock,
                   *LY,
                *STAT,
                *LCDC);
        
        interrupts->display_mode = next_mode;
        interrupts->display_modeclock = 0;


        if (((*STAT) & 0b11) != (byte_t)current_mode) {

            gdbg_trace(g_GB.dbg, "DISPLAY::current_mode:%d doesn't match STAT MODE FLAG:%hhu",
                       current_mode, ((*STAT) & 0b11));

        }

        //Clear & Update MODE FLAG in STAT
        *STAT &= ~0b11; 
        *STAT |= (next_mode & 0b11);
    }


    //
    //Execute requested interupts (if any):
    //
    gb_INTERRUPT_execute(); 
}


