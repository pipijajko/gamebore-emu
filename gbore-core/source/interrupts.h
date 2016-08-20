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

/*
Interupt Service Routines start addresses

When multiple interrupts occur, the interruptwith highest priority should be handled (?)
When ISR is executed it's bit should be zeroed before setting IE

*/
#define GB_ISR_VBLANK               0x0040 // Priority 1
#define GB_ISR_LCDC_STATUS          0x0048 // Priority 2
#define GB_ISR_TIMER_OVERFLOW       0x0050 // Priority 3
#define GB_ISR_SERIAL_XFER_COMPLETE 0x0058 // Priority 4
#define GB_ISR_KEYPAD_HI_TO_LO      0x0060 // Priority 5

// Interrupt flags for use with IME and IF:
#define GB_INT_FLAG_VBLANK      0x01
#define GB_INT_FLAG_LCDC_STAT   0x02
#define GB_INT_FLAG_TIMER       0x04
#define GB_INT_FLAG_SERIAL      0x08
#define GB_INT_FLAG_KEYPAD      0x10

#define GB_INT_ALL_MASK (GB_INT_FLAG_VBLANK|GB_INT_FLAG_LCDC_STAT|GB_INT_FLAG_TIMER|GB_INT_FLAG_SERIAL|GB_INT_FLAG_KEYPAD)
#define GB_INT_NUM             5

//
//LCDC Stat interrupt subtypes:
//
#define GB_INT_STAT_LYLYC_FLAG  0x40  //bit6
#define GB_INT_STAT_OAM_FLAG    0x20  //bit5
#define GB_INT_STAT_VBLANK_FLAG 0x10  //bit4
#define GB_INT_STAT_HBLANK_FLAG 0x08  //bit3
#define GB_STAT_COINCIDENCE_FLAG 0x04 //bit2


//
// Clocks and shit
//
#define GB_MCLOCK_UPDATE_TICKS 4
#define GB_DIV_UPDATE_TICKS (GB_MCLOCK_UPDATE_TICKS*4) // 16384Hz - quarter of the base M-Clock speed (262,144Hz)
#define CPU_TO_MCLOCK(ticks) ((ticks) << 2)

#define GB_TICKS_HBLANK    204   // MODE 00 average (201-207 ticks)
#define GB_TICKS_VBLANK    456   // MODE 01 (456 ticks per LY increment)
#define GB_TICKS_OAM_READ  80    // MODE 10 average (77-83 ticks)
#define GB_TICKS_VRAM_READ 171   // MODE 11 average (169-175 ticks)

#define GB_TICKS_MINIMAL    GB_TICKS_OAM_READ

#define GB_LY_MAX 153 //144 display lines + 10 lines for VBLANK

#define GB_LY_INCREMENT(LYval) (((LYval) + 1) % (GB_LY_MAX + 1))


//
// Structures and types for interrupt control
//

typedef enum gb_LCDC_period
{

    gb_LCDC_HBLANK = 0b00,
    gb_LCDC_VBLANK = 0b01,
    gb_LCDC_OAM_READ = 0b10,
    gb_LCDC_VRAM_READ = 0b11,

    gb_LCDC_DISABLED = -2, //special value
    gb_LCDC_NO_CHANGE = -1, //special value
} gb_LCDC_mode;


typedef enum gb_IME_transition
{

    gb_IME_unchanged,
    gb_IME_pending_enable,
    gb_IME_pending_disable,

} gb_IME_transition;

typedef struct gb_interrupt_data_s
{

    // 2 variables to track HALTed CPU state
    // Upon HALT opcode, both are set to TRUE, CPU waits for interrupt.
    // Emulator will loop on HALT command until the `is_waiting_for_ISR` is set to FALSE
    // by a Interupt Service Routine.

    bool    HALT;
    bool    HALT_is_waiting_for_ISR;
    bool    STOP;
    bool    STOP_is_waiting_for_JOYP;


    bool    IME;
    gb_IME_transition IME_change;
    uint64_t total_ticks; //ticks @ 4,194,304Hz
    uint64_t last_opcode_ticks;

    //
    // pre-calculated values
    // when `total_ticks` > *_next_update_ticks,
    // the appropriate counter is incremented.
    //
    uint64_t DIV_next_update_ticks;
    uint64_t TIMA_next_update_ticks;
    bool     TIMA_enabled;


    uint32_t display_modeclock;
    uint32_t display_line;
    gb_LCDC_mode display_mode;

} gb_interrupt_data_s;


void gb_INTERRUPT_step(byte_t ticks_delta);
void gb_INTERRUPT_request(byte_t const interrupt_signal_flag);




//
// Memory mapped IO (IO_PORTS) addresses:
//

/* Interrupt Enable IE (R/W)
    Bit 4: Transition from High to Low of Pin number P10-P13.
    Bit 3: Serial I/O transfer complete
    Bit 2: Timer Overflow
    Bit 1: LCDC (see STAT)
    Bit 0: V-Blank
0: disable; 1: enable 
*/
#define GB_IO_IE 0xFFFF 

/* Interrupt Flag IF (R/W)
    Bit 4: Transition from High to Low of Pin number P10-P13
    Bit 3: Serial I/O transfer complete
    Bit 2: Timer Overflow
    Bit 1: LCDC (see STAT)
    Bit 0: V-Blank
*/
#define GB_IO_IF 0xFF0F

/* LCDC Status STAT (R/W)
    Bit 6 - LYC=LY Coincidence Interrupt (1=Enable) (Read/Write)
    Bit 5 - Mode 2 OAM Interrupt         (1=Enable) (Read/Write)
    Bit 4 - Mode 1 V-Blank Interrupt     (1=Enable) (Read/Write)
    Bit 3 - Mode 0 H-Blank Interrupt     (1=Enable) (Read/Write)
    Bit 2 - Coincidence Flag  (0:LYC<>LY, 1:LYC=LY) (Read Only)
    Bit 1-0 - Mode Flag       (Mode 0-3, see below) (Read Only)
    0: During H-Blank
    1: During V-Blank
    2: During Searching OAM-RAM
    3: During Transfering Data to LCD Driver
*/
#define GB_IO_STAT 0xFF41 


/*Timer Counter TIMA (R/W) 
    Incremetned by a clock requency specified by TAC. Generates interrupt on overflow.
*/
#define GB_IO_TIMA 0xFF05 

/* Timer Modulo TMA (R/W) 
    When TIMA overflows, this data will be loaded. 
*/
#define GB_IO_TMA 0xFF06 

/* Tmer Clock Frequency (TAC)
    Bits 1+0 -- Input clock select
        00 : 4 096 KHz
        01 : 262 144 KHz
        10 : 65 536 KHz
        11 : 16 384 KHz

    Bit 2 -- 
        0: timer stop, 
        1: start timer
*/
#define GB_IO_TAC 0xFF07  

/* Divider register incremented 16384 times per sec. (R/W) Writing sets it to 0.
*/
#define GB_IO_DIV 0xFF04 
