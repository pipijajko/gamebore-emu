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


void gb_INTERRUPT_request(byte_t const interrupt_signal_flag) 
{
    gb_word_t *const IF = gb_MMU_access_internal(GB_IO_IF);
    StopIf(interrupt_signal_flag > GB_INT_FLAG_KEYPAD, 
           abort(),
           "invalid INT signal:%hhu", interrupt_signal_flag);

    *IF |= interrupt_signal_flag; //Set Interrupt Request Flag
    
    // In case if the CPU is halted, clear the `is_waiting` flag:
    g_GB.interrupts.HALT_is_waiting_for_ISR = false;
    //i think STOP must be hooked to FF00 register also, not just interrupt
    g_GB.interrupts.STOP_is_waiting_for_JOYP = ((*IF & GB_INT_FLAG_KEYPAD) != 0) ;

}


void gb_INTERRUPT_execute(void) 
{
    gb_word_t *const IF = gb_MMU_access_internal(GB_IO_IF);
    gb_word_t *const IE = gb_MMU_access_internal(GB_IO_IE);
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

        (*gb_MMU_access_internal(GB_IO_DIV))++;
        interrupts->DIV_next_update_ticks += GB_DIV_UPDATE_TICKS;
    }

    //
    // TIMA/TAC/TMA Configurable Timer
    //
    // Check timer control:
    gb_word_t const TAC    = *gb_MMU_access_internal(GB_IO_TAC);
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

            gb_word_t * const TIMA = gb_MMU_access_internal(GB_IO_TIMA);

            if (increment+(*TIMA) >= 0xFF) {

                // on TIMA overflow, generate interrupt and reinitialize with TMA
                gb_INTERRUPT_request(GB_INT_FLAG_TIMER);

                *TIMA = *gb_MMU_access_internal(GB_IO_TMA);

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
    // Temporary here.
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
    gb_word_t const * const LYC  = gb_MMU_access_internal(GB_IO_LYC);
    gb_word_t const * const LCDC = gb_MMU_access_internal(GB_IO_LCDC);
    gb_word_t * const STAT       = gb_MMU_access_internal(GB_IO_STAT);
    gb_word_t * const LY         = gb_MMU_access_internal(GB_IO_LY);
    bool const LCDC_enabled      = BIT_GET_N(*LCDC, 7);
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
            gdbg_trace(g_GB.dbg, "LCDC DISABLED!");
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
    
    
    // If check if we are expected to request STAT interrupt
    // for the LCD status events that occured:
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
                   modename,interrupts->display_modeclock, *LY, *STAT, *LCDC);
        
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




void gb_INTERRUPT_ioports_initialize()
{
    // IO PORTS initialization -- I'm not particularly sure this belongs here,
    // but I know that the code above will not fly if this is not run. 
    // So I'll leave it here.
    *gb_MMU_access_internal(0xFF05) = (0x00); // TIMA
    *gb_MMU_access_internal(0xFF06) = (0x00); // TMA
    *gb_MMU_access_internal(0xFF07) = (0x00); // TAC
    *gb_MMU_access_internal(0xFF10) = (0x80); // NR10
    *gb_MMU_access_internal(0xFF11) = (0xBF); // NR11
    *gb_MMU_access_internal(0xFF12) = (0xF3); // NR12
    *gb_MMU_access_internal(0xFF14) = (0xBF); // NR14
    *gb_MMU_access_internal(0xFF16) = (0x3F); // NR21
    *gb_MMU_access_internal(0xFF17) = (0x00); // NR22
    *gb_MMU_access_internal(0xFF19) = (0xBF); // NR24
    *gb_MMU_access_internal(0xFF1A) = (0x7F); // NR30
    *gb_MMU_access_internal(0xFF1B) = (0xFF); // NR31
    *gb_MMU_access_internal(0xFF1C) = (0x9F); // NR32
    *gb_MMU_access_internal(0xFF1E) = (0xBF); // NR33
    *gb_MMU_access_internal(0xFF20) = (0xFF); // NR41
    *gb_MMU_access_internal(0xFF21) = (0x00); // NR42
    *gb_MMU_access_internal(0xFF22) = (0x00); // NR43
    *gb_MMU_access_internal(0xFF23) = (0xBF); // NR30
    *gb_MMU_access_internal(0xFF24) = (0x77); // NR50
    *gb_MMU_access_internal(0xFF25) = (0xF3); // NR51
    *gb_MMU_access_internal(0xFF26) = (g_GB.gb_model == gb_dev_SGB) ? (0xF0) : (0xF1); // NR52
    *gb_MMU_access_internal(0xFF40) = (0x91); // LCDC
    *gb_MMU_access_internal(0xFF42) = 0x00; // SCY 
    *gb_MMU_access_internal(0xFF43) = 0x00; // SCX
    *gb_MMU_access_internal(0xFF45) = (0x00); // LYC
    *gb_MMU_access_internal(0xFF47) = (0xFC); // BGP
    *gb_MMU_access_internal(0xFF48) = (0xFF); // OBP0
    *gb_MMU_access_internal(0xFF49) = (0xFF); // OBP1
    *gb_MMU_access_internal(0xFF4A) = (0x00); // WY
    *gb_MMU_access_internal(0xFF4B) = (0x00); // WX
    *gb_MMU_access_internal(0xFFFF) = (0x00); // IE
                                              /* TODO: Scroll Nintendo logo from $104:$133 */
                                              /* TODO: Add cheksum verification - LSB_of(SUM($143:$14d)+25) == 0 */
}
