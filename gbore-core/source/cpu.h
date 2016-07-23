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


// Internal definitions for CPU.C

typedef
enum gb_jump_cond_e
{
    gb_jump_NZ = 0b00, //NotZero
    gb_jump_Z = 0b01, //Zero
    gb_jump_NC = 0b10, //NotCarry
    gb_jump_C = 0b11, //Carry
} gb_jump_cond_e;

typedef
enum gb_alu_mode_e
{
    gb_ALU_ADD = 0b000,
    gb_ALU_ADC = 0b001,
    gb_ALU_SUB = 0b010,
    gb_ALU_SBC = 0b011,
    gb_ALU_AND = 0b100,
    gb_ALU_XOR = 0b101,
    gb_ALU_OR = 0b110,
    gb_ALU_CP = 0b111,
} gb_alu_mode_e;

//Bit Rotations
#define DIR_LEFT  0
#define DIR_RIGHT 1

#define STOR8(ADDR) (*gb_MMU_store8((ADDR)))
#define LOAD8(ADDR)  (*gb_MMU_load8((ADDR)))

#define STOR16(ADDR) (*gb_MMU_store16((ADDR)))
#define LOAD16(ADDR)  (*gb_MMU_load16((ADDR)))

//
//Selectors for 8-bit registers, in case when `d` is 0x6, we need to load/store to memory.
//
#define REG8_WRITE(d) (*( ((d) != HL_INDIRECT) ? g_regmap_D[(d)] : gb_MMU_store8(HL) ))
#define REG8_READ(d)  (*( ((d) != HL_INDIRECT) ? g_regmap_D[(d)] : gb_MMU_load8(HL)  ))


//
//
// Selectors for 16-bit registers
//
#define RM1 1 // RegMap #1 selects one of: BC,DE,HL,SP registers
#define RM2 2 // RegMap #2 selects one of: BC,DE,HL,AF registers

#define REG16_RW(r, rmap) (*((rmap == RM1) ? g_regmap_R[(r)] : g_regmap_R2[(r)]))

#define REG16_READ  REG16_RW
#define REG16_WRITE REG16_RW


