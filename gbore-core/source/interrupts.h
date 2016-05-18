#pragma once




/*
Interupt Service Routines start addresses

When multiple interrupts occur, the interruptwith highest priority should be handled (?)
When ISR is executed it's bit should be zeroed before setting IE

*/
#define GB_ISR_VBLANK               0x0040 // Priority 1
#define GB_ISR_LCDC_STATUS          0x0048 // Priority 2
#define GB_ISR_TIMER_OVERFLOW       0x0050 // Priority 3
#define GB_ISR_SERIAL_XFER_COMPLETE 0x0050 // Priority 4
#define GB_ISR_KEYPAD_HI_TO_LO      0x0060 // Priority 5



// Interrupt flags for use with IME and IF:
#define GB_INT_FLAG_VBLANK      0x01
#define GB_INT_FLAG_LCDC_STAT   0x02
#define GB_INT_FLAG_TIMER       0x04
#define GB_INT_FLAG_SERIAL      0x08
#define GB_INT_FLAG_KEYPAD      0x10


#define GB_INT_NUM             5

/*IME - Interrupt Master Enable Flag(Write Only)
0 - Disable all Interrupts
1 - Enable all Interrupts that are enabled in IE Register(FFFF)
The IME flag is used to disable all interrupts, overriding any enabled bits in
the IE Register.It isn't possible to access the IME flag by using a I/O
address, instead IME is accessed directly from the CPU, by the following
opcodes / operations:
EI; Enable Interrupts(ie.IME = 1)
DI; Disable Interrupts(ie.IME = 0)
RETI; Enable Ints & Return(same as the opcode combination EI, RET)
<INT>; Disable Ints & Call to Interrupt Vector
*/



/* 
Interrupt Enable IE (R/W)
    Bit 4: Transition from High to Low of Pin number P10-P13.
    Bit 3: Serial I/O transfer complete
    Bit 2: Timer Overflow
    Bit 1: LCDC (see STAT)
    Bit 0: V-Blank

0: disable; 1: enable 
*/
#define GB_IO_IE 0xFFFF 
/*
Interrupt Flag IF (R/W)
    Bit 4: Transition from High to Low of Pin number P10-P13
    Bit 3: Serial I/O transfer complete
    Bit 2: Timer Overflow
    Bit 1: LCDC (see STAT)
    Bit 0: V-Blank

*/
#define GB_IO_IF 0xFF0F


/*
Timer Counter TIMA (R/W) 
    Incremetned by a clock requency specified by TAC. Generates interrupt on overflow.
*/
#define GB_IO_TIMA 0xFF05 

/* Timer Modulo TMA (R/W) 
When TIMA overflows, this data will be loaded. 
*/
#define GB_IO_TMA 0xFF06 

/* 
Tmer Clock Frequency (TAC)
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

#define GB_IO_P1 0xFF00 //Register for reading joypad info
#define GB_IO_SB 0xFF01 //Serial Transfer Data
#define GB_IO_SC 0xFF02 //Serial I/O Control Register (R/W)
#define GB_IO_DIV 0xFF04 //Divider register incremented 16384 times per sec. (R/W) Writing sets it to 0.


/*
LCDC Status STAT (R/W)
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

The two lower STAT bits show the current status of the LCD controller.
    Mode 0: 
        The LCD controller is in the H-Blank period and
        the CPU can access both the display RAM (8000h-9FFFh)
        and OAM (FE00h-FE9Fh)

    Mode 1:
        The LCD contoller is in the V-Blank period (or the
        display is disabled) and the CPU can access both the
        display RAM (8000h-9FFFh) and OAM (FE00h-FE9Fh)

    Mode 2: 
        The LCD controller is reading from OAM memory.
        The CPU <cannot> access OAM memory (FE00h-FE9Fh)
        during this period.

    Mode 3: 
        The LCD controller is reading from both OAM and VRAM,
        The CPU <cannot> access OAM and VRAM during this period.
        CGB Mode: Cannot access Palette Data (FF69,FF6B) either.



        The following are typical when the display is enabled:
        Mode 2  2_____2_____2_____2_____2_____2___________________2____
        Mode 3  _33____33____33____33____33____33__________________3___
        Mode 0  ___000___000___000___000___000___000________________000
        Mode 1  ____________________________________11111111111111_____

        The Mode Flag goes through the values 0, 2, and 3 at a cycle of about 109uS. 0
        is present about 48.6uS, 2 about 19uS, and 3 about 41uS. This is interrupted
        every 16.6ms by the VBlank (1). The mode flag stays set at 1 for about 1.08
        ms.

        Mode 0 is present between 201-207 clks, 2 about 77-83 clks, and 3 about
        169-175 clks. A complete cycle through these states takes 456 clks. VBlank
        lasts 4560 clks. A complete screen refresh occurs every 70224 clks.)

        ISTAT98.TXT 1998 BY MARTIN KORTH
        --------------------------------
        Interrupt INT 48h - LCD STAT
        ----------------------------


        The STAT register (FF41) selects the conditions that will generate this
        interrupt (expecting that interrupts are enabled via EI or RETI and that
        IE.1 (FFFF.1) is set).
        STAT.3        HBLANK  (start of mode 0)
        STAT.4        VBLANK  (start of mode 1) (additional to INT 40)
        STAT.5        OAM     (start of mode 2 and mode 1)
        STAT.6        LY=LYC  (see info about LY=00)


        If two STAT-condiditions come true at the same time only one INT 48 is
        generated. This happens in combinations
        LYC=01..90  and  OAM     at the same time  (see info about LY=00)
        LYC=90      and  VBLANK  at the same time
        OAM         and  VBLANK  at the same time
        HBLANK and LYC=00 and LYC=91..99 are off-grid and cannot hit others.


        Some STAT-conditions cause the following STAT-condition to be ignored:
        Past  VBLANK           following  LYC=91..99,00        is ignored
        Past  VBLANK           following  OAM         (at 00)  is ignored
        Past  LYC=00 at 99.2   following  OAMs (at 00 and 01) are ignored
        Past  LYC=01..8F       following  OAM     (at 02..90)  is ignored
        Past  LYC=00..8F       following  HBLANK  (at 00..8F)  is ignored
        Past  LYC=8F           following  VBLANK               is ignored
        Past  HBLANK           following  OAM                  is ignored
        Past  HBLANK at 8F     following  VBLANK               is ignored


        If the OAM condition occurs, everything following -is- recognized.
        An ignored VBLANK condition means that INT 48h does not produce a V-Blank
        interrupt, INT 40h is not affected and still produces V-Blank interrupts.


        The last LY period (LY=99) is a shorter than normal LY periodes. It is followed
        by the first LY period (LY=00) this period is longer than normal periodes.
        LY value    clks    description
        -------------------------------
        LY=01..8F   456     at the same moment than OAM
        LY=90       456     at the same moment than pseudo-OAM and VBLANK
        LY=91..98   456     during vblank (similiar to LY=01..8F)
        LY=99       ca.56   similiar to LY=91..98, but shorter
        LY=00       ca.856  starts during vblank, then present until second OAM
        Because of the pre-started long LY=00 period, LYC=00 occurs within vblank
        period and before first OAM (where we would have expected it)


        *EOF*




    */
#define GB_IO_STAT 0xFF41 

//Stat interrupt subtype
#define GB_INT_STAT_LYLYC_FLAG  0x40  //bit6
#define GB_INT_STAT_OAM_FLAG    0x20  //bit5
#define GB_INT_STAT_VBLANK_FLAG 0x10  //bit4
#define GB_INT_STAT_HBLANK_FLAG 0x08  //bit3
#define GB_STAT_COINCIDENCE_FLAG 0x04 //bit2


typedef enum gb_LCDC_period {

    gb_LCDC_HBLANK = 0b00,
    gb_LCDC_VBLANK = 0b01,
    gb_LCDC_OAM_READ = 0b10,
    gb_LCDC_VRAM_READ = 0b11,

    gb_LCDC_DISABLED = -2,
    gb_LCDC_NO_CHANGE = -1,
} gb_LCDC_mode;


typedef enum gb_IME_transition{
    
    gb_IME_unchanged,
    gb_IME_pending_enable,
    gb_IME_pending_disable,

} gb_IME_transition;

typedef struct gb_interrupt_data_s {
    

    _Bool    IME;
    gb_IME_transition IME_change;
    
    

    uint32_t total_ticks;
    uint32_t last_opcode_ticks;
    
    
    uint32_t display_modeclock;
    uint32_t display_line;
    gb_LCDC_mode display_mode;


    
} gb_interrupt_data_s;


void gb_INTERRUPT_step(byte_t ticks_delta);
void gb_INTERRUPT_request(byte_t const interrupt_signal_flag);

