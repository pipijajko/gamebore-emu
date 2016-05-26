#pragma once
#include <stddef.h>
//SharpLR35902 CPU register layout


#pragma pack(1)
typedef struct gb_cpu_registers_s{

    /*Register Layout
    +-------+------+
    | 15..8 | 7..0 |
    +-------+------+
    |   A   |  F   |
    +-------+------+
    |   B   |  C   |
    +-------+------+
    |   D   |  E   |
    +-------+------+
    |   H   |  L   |
    +-------+------+
    |       SP     |
    +--------------+
    |       PC     |
    +--------------+
    */

    union {//Anynymous unions supported as of C11
        struct {
            uint8_t F;
            uint8_t A;
        };
        uint16_t AF;
    };

    union {
        struct {
            uint8_t C;
            uint8_t B;
        };
        uint16_t BC;
    };

    union {
        struct {
            uint8_t E;
            uint8_t D;
        };
        uint16_t DE;
    };

    union {
        struct {
            uint8_t L;
            uint8_t H;
        };
        uint16_t HL;
    };
    uint16_t PC;
    uint16_t SP;
} gb_cpu_registers_s;
static_assert (sizeof(gb_cpu_registers_s) == 12, "incorrect pragma pack");
#pragma pack()

//
// CPU Defines
//
#define GB_OPCODES_N     256

// F-register flags
#define GB_FLAG_Z        0b10000000
#define GB_FLAG_N        0b01000000
#define GB_FLAG_H        0b00100000
#define GB_FLAG_C        0b00010000
#define GB_FLAG_Z_OFFSET 7
#define GB_FLAG_N_OFFSET 6
#define GB_FLAG_H_OFFSET 5
#define GB_FLAG_C_OFFSET 4

//
// CPU External Functions
//
void gb_CPU_init(void);
byte_t gb_CPU_step(void);

byte_t gb_CPU_interrupt(gb_addr_t vector);

//
// static struct checks 
//
#ifdef AF
static_assert(0, "Invalid include order - macros.h included before cpu.h");
#endif
static_assert(offsetof(gb_cpu_registers_s, AF) == offsetof(gb_cpu_registers_s, F), "bad register offset: AF/A ");
static_assert(offsetof(gb_cpu_registers_s, BC) == offsetof(gb_cpu_registers_s, C), "bad register offset: BC/C ");
static_assert(offsetof(gb_cpu_registers_s, DE) == offsetof(gb_cpu_registers_s, E), "bad register offset: DE/E ");
static_assert(offsetof(gb_cpu_registers_s, HL) == offsetof(gb_cpu_registers_s, L), "bad register offset: HL/L ");
static_assert(offsetof(gb_cpu_registers_s, PC) == offsetof(gb_cpu_registers_s, HL) + 2, "bad register offset: PC/HL ");
static_assert(offsetof(gb_cpu_registers_s, SP) == offsetof(gb_cpu_registers_s, PC) + 2, "bad register offset: SP/PC ");
static_assert(offsetof(gb_cpu_registers_s, A) == offsetof(gb_cpu_registers_s, F) + 1, "bad register offset: A/F ");
static_assert(offsetof(gb_cpu_registers_s, B) == offsetof(gb_cpu_registers_s, C) + 1, "bad register offset: B/C ");
static_assert(offsetof(gb_cpu_registers_s, D) == offsetof(gb_cpu_registers_s, E) + 1, "bad register offset: D/E ");
static_assert(offsetof(gb_cpu_registers_s, H) == offsetof(gb_cpu_registers_s, L) + 1, "bad register offset: H/L ");
