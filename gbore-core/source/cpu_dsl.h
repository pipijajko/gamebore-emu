#pragma once
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


//
// CPU Domain Specific Language 
//
// Some macros to make CPU programming more concise
// A "DSL" here is definately an overstament, but at least
// should be indicative of the purpose of this file.
//


// CPU Register shorthands
#define F  (g_GB.r.F)
#define A  (g_GB.r.A)
#define B  (g_GB.r.B)
#define C  (g_GB.r.C)
#define D  (g_GB.r.D)
#define E  (g_GB.r.E)
#define H  (g_GB.r.H)
#define L  (g_GB.r.L)
#define AF (g_GB.r.AF)
#define BC (g_GB.r.BC)
#define DE (g_GB.r.DE)
#define HL (g_GB.r.HL)
#define SP (g_GB.r.SP)
#define PC (g_GB.r.PC)

/*
Opcode parsing macros*/
#define X(OP) (((OP) & 0b11000000) >> 6) //X(OP) - 1st octal digit, bits 7-6
#define Y(OP) (((OP) & 0b00111000) >> 3) //Y(OP) - 2nd octal digit, bits 5-3
#define Z(OP) (((OP) & 0b00000111) >> 0) //Z(OP) - 3rd octal digit, bits 2-0
#define P(OP) (((OP) & 0b00110000) >> 4) //P(OP) - Y(OP) >> 1, i.e. bits 5-4
#define Q(OP) (((OP) & 0b00001000) >> 3) //Q(OP) - Y(OP) % 2, i.e. bit 3

//Bit Rotations
#define DIR_LEFT  0
#define DIR_RIGHT 1

#define STOR8(ADDR, VAL) (gb_MMU_store8((ADDR), (VAL)))
#define LOAD8(ADDR) (gb_MMU_load8((ADDR)))

#define STOR16(ADDR,VAL) (gb_MMU_store16((ADDR),(VAL)))
#define LOAD16(ADDR)  (gb_MMU_load16((ADDR)))

//
//Selectors for 8-bit registers, in case when `d` is 0x6, we need to load/store to memory.
//
#define REG8_WRITE(d, VAL)\
 ( ((d) != HL_INDIRECT) ? (*g_regmap_D[(d)] = (VAL)) : (*gb_MMU_access_internal(HL) = (VAL)) )

#define REG8_READ(d)  ( ((d) != HL_INDIRECT) ? *g_regmap_D[(d)] : gb_MMU_load8(HL) )


//
//
// Selectors for 16-bit registers
//
#define RM1 1 // RegMap #1 selects one of: BC,DE,HL,SP registers
#define RM2 2 // RegMap #2 selects one of: BC,DE,HL,AF registers

#define REG16_RW(r, rmap) (*((rmap == RM1) ? g_regmap_R[(r)] : g_regmap_R2[(r)]))

#define REG16_READ  REG16_RW
#define REG16_WRITE REG16_RW


/*
Helper macros/functions for setting CPU flags */
typedef struct flagsetter_s
{
    int8_t ZR, NG, HC, CR; //using 2 letter names to avoid collisions with macros
}flagsetter_s;

#define FLAG_OMIT ((int8_t)(-127)) 

void flags_setter(flagsetter_s f) {
    if (f.ZR != FLAG_OMIT) SETCLR((f.ZR), F, GB_FLAG_Z);
    if (f.NG != FLAG_OMIT) SETCLR((f.NG), F, GB_FLAG_N);
    if (f.HC != FLAG_OMIT) SETCLR((f.HC), F, GB_FLAG_H);
    if (f.CR != FLAG_OMIT) SETCLR((f.CR), F, GB_FLAG_C);
}

#define FLAGS(...) \
    flags_setter((flagsetter_s){.ZR = FLAG_OMIT, \
                                .NG = FLAG_OMIT, \
                                .HC = FLAG_OMIT, \
                                .CR = FLAG_OMIT, __VA_ARGS__})

#define FLAGS_ALL(ZR, NG, HC, CR) \
    flags_setter((flagsetter_s){ZR, NG, HC, CR})

/*
Flag getters*/
#define GET_Z() (((F) & GB_FLAG_Z) == GB_FLAG_Z)
#define GET_N() (((F) & GB_FLAG_N) == GB_FLAG_N)
#define GET_H() (((F) & GB_FLAG_H) == GB_FLAG_H)
#define GET_C() (((F) & GB_FLAG_C) == GB_FLAG_C)


/*
CPU Register Maps */
#define HL_INDIRECT 0x6 //g_regmap_D index for (HL)
#define HL_INDIRECT_DUMMY NULL

static gb_dword_t * const g_regmap_R[] = { &BC, &DE, &HL, &SP }; //2bit
static gb_dword_t * const g_regmap_R2[] = { &BC, &DE, &HL, &AF }; //2bit'
static gb_word_t  * const g_regmap_D[] = { &B, &C, &D, &E, &H,&L, HL_INDIRECT_DUMMY, &A }; //3bit


// Move some of the macros from helpers.h here.