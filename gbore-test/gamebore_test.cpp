// gamebore_test.cpp : Defines the entry point for the console application.
//

#include "gtest\gtest.h"



extern "C"{
#include "types.h"
#include "cpu.h"
#include "dispatch_tables.h"
#include "gamebore.h"
}
#include <iostream>

//Initialize Stopif macro
char error_mode = '\0';
FILE *error_log = NULL;



//macros for tests
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define HL_INDIRECT NULL


TEST(Macros, BitMask) {
    
    EXPECT_EQ(0b00000001, BIT_MASK8(0, 0));

    EXPECT_EQ(0b00000011, BIT_MASK8(0, 1));

    EXPECT_EQ(0b00000111, BIT_MASK8(0, 2));
    EXPECT_EQ(0b00000110, BIT_MASK8(1, 2));
    EXPECT_EQ(0b00000100, BIT_MASK8(2, 2));

    EXPECT_EQ(0b00001111, BIT_MASK8(0, 3));
    EXPECT_EQ(0b00001110, BIT_MASK8(1, 3));
    EXPECT_EQ(0b00001100, BIT_MASK8(2, 3));
    EXPECT_EQ(0b00001000, BIT_MASK8(3, 3));

    EXPECT_EQ(0b00011111, BIT_MASK8(0, 4));
    EXPECT_EQ(0b00011110, BIT_MASK8(1, 4));
    EXPECT_EQ(0b00011100, BIT_MASK8(2, 4));
    EXPECT_EQ(0b00011000, BIT_MASK8(3, 4));
    EXPECT_EQ(0b00010000, BIT_MASK8(4, 4));

    EXPECT_EQ(0b00111111, BIT_MASK8(0, 5));
    EXPECT_EQ(0b00111110, BIT_MASK8(1, 5));
    EXPECT_EQ(0b00111100, BIT_MASK8(2, 5));
    EXPECT_EQ(0b00111000, BIT_MASK8(3, 5));
    EXPECT_EQ(0b00110000, BIT_MASK8(4, 5));
    EXPECT_EQ(0b00100000, BIT_MASK8(5, 5));


    EXPECT_EQ(0b01111111, BIT_MASK8(0, 6));
    EXPECT_EQ(0b01111110, BIT_MASK8(1, 6));
    EXPECT_EQ(0b01111100, BIT_MASK8(2, 6));
    EXPECT_EQ(0b01111000, BIT_MASK8(3, 6));
    EXPECT_EQ(0b01110000, BIT_MASK8(4, 6));
    EXPECT_EQ(0b01100000, BIT_MASK8(5, 6));
    EXPECT_EQ(0b01000000, BIT_MASK8(6, 6));

    EXPECT_EQ(0b11111111, BIT_MASK8(0, 7));
    EXPECT_EQ(0b11111110, BIT_MASK8(1, 7));
    EXPECT_EQ(0b11111100, BIT_MASK8(2, 7));
    EXPECT_EQ(0b11111000, BIT_MASK8(3, 7));
    EXPECT_EQ(0b11110000, BIT_MASK8(4, 7));
    EXPECT_EQ(0b11100000, BIT_MASK8(5, 7));
    EXPECT_EQ(0b11000000, BIT_MASK8(6, 7));
    EXPECT_EQ(0b10000000, BIT_MASK8(7, 7));
}



TEST(Macros, BitGet) {


    for (int i = 0; i < 8; i++){
        EXPECT_EQ(1, BIT_GET_N(0b11111111, i));
        if (i < 7) EXPECT_EQ(1, BIT_GET_N(0b01111111, i));
        else       EXPECT_EQ(0, BIT_GET_N(0b00111111, i));

        if (i < 6) EXPECT_EQ(1, BIT_GET_N(0b00111111, i));
        else       EXPECT_EQ(0, BIT_GET_N(0b00111111, i));
        if (i < 5) EXPECT_EQ(1, BIT_GET_N(0b00011111, i));
        else       EXPECT_EQ(0, BIT_GET_N(0b00011111, i));
        if (i < 4) EXPECT_EQ(1, BIT_GET_N(0b00001111, i));
        else       EXPECT_EQ(0, BIT_GET_N(0b00001111, i));
        if (i < 3) EXPECT_EQ(1, BIT_GET_N(0b0000111, i));
        else       EXPECT_EQ(0, BIT_GET_N(0b0000111, i));
        if (i < 2) EXPECT_EQ(1, BIT_GET_N(0b0000011, i));
        else       EXPECT_EQ(0, BIT_GET_N(0b0000011, i));
        if (i < 1) EXPECT_EQ(1, BIT_GET_N(0b0000001, i));
        else       EXPECT_EQ(0, BIT_GET_N(0b0000001, i));
    }

    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(1, BIT_GET_N((byte_t)1 << i, i));
    }

}

TEST(Macros, BitSetRes) {
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(1 << i, BIT_SET_N(0, i));
        EXPECT_EQ(0,      BIT_RES_N(1 << i, i));
    }
}


TEST(OpcodeSig, ParsePatternSingleOp)
{
    //r - relevant bits, mask used to filter opcode signature
    //m - match bits, mask used to identify opcode
    //n - number of opcodes matching the signature
    
    gb_instruction_bits_s s[6] = {
        gb_parse_opcode_pattern("00000000", 8),
        gb_parse_opcode_pattern("00000 0 00", 10),
        gb_parse_opcode_pattern("11111111", 8),
        gb_parse_opcode_pattern("1010 1010", 9),
        gb_parse_opcode_pattern("11110000", 8),
        gb_parse_opcode_pattern("11112345ABCD0000", 16) //arbitrary chars do not interfere with parsing
    };

    
    //Signatures given will only match one opcode, hence
    // n = 1
    // m = 0xFF
    for (int i = 0; i < 6; i++) {
        EXPECT_EQ(1, s[i].n);
        EXPECT_EQ(0xFF, s[i].r);
    }

    EXPECT_EQ(0, s[0].m);
    EXPECT_EQ(0, s[1].m);
    EXPECT_EQ(0b11111111, s[2].m);
    EXPECT_EQ(0b10101010, s[3].m);
    EXPECT_EQ(0b11110000, s[4].m);
    EXPECT_EQ(0b11110000, s[5].m);
}


TEST(OpcodeSig, ParsePatternNegative)
{
    //r - relevant bits, mask used to filter opcode signature
    //m - match bits, mask used to identify opcode
    //n - number of opcodes matching the signature

    gb_instruction_bits_s s[5] = {
        gb_parse_opcode_pattern("00000000", 0),
        gb_parse_opcode_pattern(NULL, 8),
        gb_parse_opcode_pattern(NULL, 0),
        gb_parse_opcode_pattern("-------", 7), //not all bits are defined
        gb_parse_opcode_pattern("-- -- -- --", 11) //will not match any opcode
        
    };


    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(0, s[i].n);
        EXPECT_EQ(0, s[i].m);
        EXPECT_EQ(0, s[i].r);
    }
}


TEST(OpcodeSig, ParsePatternSanity)
{
    //r - relevant bits, mask used to filter opcode signature
    //m - match bits, mask used to identify opcode
    //n - number of opcodes matching the signature
    gb_instruction_bits_s s[8] = {
        gb_parse_opcode_pattern("00010000", 8),
        gb_parse_opcode_pattern("000-0000", 8), 
        gb_parse_opcode_pattern("00--0 0 00", 10),
        gb_parse_opcode_pattern("-11-111-", 8),
        gb_parse_opcode_pattern("1111----", 8),
        gb_parse_opcode_pattern("000---- -", 9),
        gb_parse_opcode_pattern("-11---- -", 9),
        gb_parse_opcode_pattern("---- ---1", 9),
        
    };
    //Signatures given will only match one opcode, hence
    // n = 1, m = 0xFF
    for (int i = 0; i < 6; i++) {
        EXPECT_EQ(1 << i, s[i].n);
        
    }

    //r-check.
    //basically wherever theres '-' in pattern, relevant bit mask should have 0
    EXPECT_EQ(s[0].r, 0b11111111);
    EXPECT_EQ(s[1].r, 0b11101111);
    EXPECT_EQ(s[2].r, 0b11001111);
    EXPECT_EQ(s[3].r, 0b01101110);
    EXPECT_EQ(s[4].r, 0b11110000);
    EXPECT_EQ(s[5].r, 0b11100000);
    EXPECT_EQ(s[6].r, 0b01100000); 
    EXPECT_EQ(s[7].r, 0b00000001);
    


    //m-check.
    //basically wherever theres '1' in pattern, match mask should have 1 set.

    EXPECT_EQ(s[0].m, 0b00010000);
    EXPECT_EQ(s[1].m, 0);
    EXPECT_EQ(s[2].m, 0);
    EXPECT_EQ(s[3].m, 0b01101110);
    EXPECT_EQ(s[4].m, 0b11110000);
    EXPECT_EQ(s[5].m, 0);
    EXPECT_EQ(s[6].m, 0b01100000);
    EXPECT_EQ(s[7].m, 0b00000001);

    //all signatures should match 
    EXPECT_EQ(OP_SIGMATCH(s[0], 0b00010000), true);
    EXPECT_EQ(OP_SIGMATCH(s[1], 0), true);
    EXPECT_EQ(OP_SIGMATCH(s[2], 0), true);
    EXPECT_EQ(OP_SIGMATCH(s[3], 0b01101110), true);
    EXPECT_EQ(OP_SIGMATCH(s[4], 0b11110000), true);
    EXPECT_EQ(OP_SIGMATCH(s[5], 0), true);
    EXPECT_EQ(OP_SIGMATCH(s[6], 0b01100000), true);
    EXPECT_EQ(OP_SIGMATCH(s[7], 0b00000001), true);



    //all signatures should not match 
    EXPECT_EQ(OP_SIGMATCH(s[0], 0b11101010), false);
    EXPECT_EQ(OP_SIGMATCH(s[1], 0b11101010), false); // only 00000000 & 00010000 should match
    EXPECT_EQ(OP_SIGMATCH(s[2], 0b11101010), false);
    EXPECT_EQ(OP_SIGMATCH(s[3], 0b01101010), false);
    EXPECT_EQ(OP_SIGMATCH(s[4], 0b11010000), false);
    EXPECT_EQ(OP_SIGMATCH(s[5], 0b10000000), false);
    EXPECT_EQ(OP_SIGMATCH(s[6], 0b00100000), false);
    EXPECT_EQ(OP_SIGMATCH(s[7], 0b00000010), false);
}


void reset_registers(gb_word_t initial_value) {
    memset(&g_GB.r, initial_value, sizeof(struct gb_cpu_registers_s));
}




TEST(OpCodes, CBPrefixed) {
    
    gb_initialize(NULL, 0);
    auto f = g_GB.ops[0xCB];
    EXPECT_EQ(f.size, 2);
    EXPECT_FALSE(f.handler == NULL);
    
    reset_registers(0b0001111);

    f.handler(0xCB, 0x00);
    EXPECT_EQ(0b00011110, g_GB.r.B);
    gb_destroy();
    

}

typedef 
struct op_def {
    gb_opcode_t opcode;
    char*       op_name;
    byte_t      op_size;
    byte_t      cpu_cycles;
    char*       flag_affinity;
} op_def_t;


enum flags {
    f_0 = 0x00,
    f_c = GB_FLAG_C,
    f_h = GB_FLAG_H,
    f_z = GB_FLAG_Z,
    f_n = GB_FLAG_N,
    f_hc = GB_FLAG_C | GB_FLAG_H,
    f_zh = GB_FLAG_Z | GB_FLAG_H,
    f_nh = GB_FLAG_N | GB_FLAG_H,
    f_zc = GB_FLAG_C | GB_FLAG_Z,
    f_zn = GB_FLAG_N | GB_FLAG_Z,
    f_znc = GB_FLAG_Z | GB_FLAG_N | GB_FLAG_C,
    f_znch = GB_FLAG_Z | GB_FLAG_N | GB_FLAG_C | GB_FLAG_H,
};

//
//void compare_registers(struct gb_cpu_registers_s *r) {
//
//    EXPECT_EQ(r->A, g_GB.r.A);
//    EXPECT_EQ(r->B, g_GB.r.B);
//    EXPECT_EQ(r->C, g_GB.r.C);
//    EXPECT_EQ(r->D, g_GB.r.D);
//    EXPECT_EQ(r->E, g_GB.r.E);
//    EXPECT_EQ(r->H, g_GB.r.H);
//    EXPECT_EQ(r->L, g_GB.r.L);
//}



TEST(Opcodes, NOP) {
    gb_initialize(NULL, 0);
    struct gb_cpu_registers_s r = g_GB.r;

    op_def_t o = { 0, "NOP", 1, 4, "- - - -" };
    EXPECT_FALSE(g_GB.ops[o.opcode].handler == NULL);
    EXPECT_EQ(g_GB.ops[o.opcode].size, o.op_size);
    byte_t cycles = g_GB.ops[o.opcode].handler(o.opcode, 0x0);

    EXPECT_EQ(cycles, o.cpu_cycles);
    EXPECT_TRUE(0 == std::memcmp(&r, &g_GB.r, sizeof(r)));
    gb_destroy();
}


TEST(Loads16Bit, LDI) {
    gb_initialize(NULL, 0);
    gbdbg_mute_print(g_GB.dbg, true);
    op_def_t oa[] = {
        { 34, "LD {HL+},A", 1, 8, "- - - -" },
        { 42, "LD A,{HL+}", 1, 8, "- - - -" },
        { 50, "LD {HL-},A", 1, 8, "- - - -" },
        { 58, "LD A,{HL-}", 1, 8, "- - - -"},
    };
    gb_word_t vals[] = { 0xAA, 0x81, 0xFF, 0x00};
    
    for (int i = 0; i < COUNT_OF(oa); i++) {
        
        EXPECT_FALSE(g_GB.ops[oa[i].opcode].handler == NULL);
        EXPECT_EQ(g_GB.ops[oa[i].opcode].size, oa[i].op_size);

        gb_word_t *src;
        gb_word_t *dst;

        for (int adr = 0x8001; adr < 0xFFFF ; adr+=32) {

            for (int t = 0; t < COUNT_OF(vals); t++) {
                g_GB.r.HL = (gb_dword_t)adr; 
                if (i % 2 == 0) { 
                    src = &g_GB.r.A;
                    dst = gb_MMU_access_internal(g_GB.r.HL);
                }
                else {
                    src = gb_MMU_access_internal(g_GB.r.HL); //we use store8 to get writable ptr value
                    dst = &g_GB.r.A;
                }
                *src = vals[t];
                *dst = ~vals[t];

                byte_t cycles = g_GB.ops[oa[i].opcode].handler(oa[i].opcode, 0);
                if (i < 2) {
                    EXPECT_EQ(((gb_dword_t)adr + 1), g_GB.r.HL);
                }
                else {
                    EXPECT_EQ(((gb_dword_t)adr-1), g_GB.r.HL);
                }
                EXPECT_EQ(cycles, oa[i].cpu_cycles);
                EXPECT_EQ(*src, *dst);
            }
        }
    }
    gb_destroy();
}


TEST(Loads16Bit, LD_RR_d16) {
    gb_initialize(NULL, 0);
    op_def_t oa[] = {
        {1,   "LD BC,d16", 3, 12, "- - - -"},
        {17, "LD DE,d16", 3, 12, "- - - -" },
        {33,  "LD HL,d16", 3, 12, "- - - -" },
        {49,  "LD SP,d16", 3, 12, "- - - -"},
    };

    gb_dword_t* r[] = { &g_GB.r.BC, &g_GB.r.DE, &g_GB.r.HL, &g_GB.r.SP };

    //Check that the number we load from d16 ends up indeed in the register
    for (int i = 0; i < COUNT_OF(oa); i++) {
        
        EXPECT_FALSE(g_GB.ops[oa[i].opcode].handler == NULL);
        EXPECT_EQ(g_GB.ops[oa[i].opcode].size, oa[i].op_size);

        for (gb_dword_t d16 = 0; d16 < 0xFFFF; d16+=5) {

            byte_t cycles = g_GB.ops[oa[i].opcode].handler(oa[i].opcode, d16);
            EXPECT_EQ(d16, *r[i]);
            EXPECT_EQ(cycles, oa[i].cpu_cycles);
        }
        //EXPECT_TRUE(0 == std::memcmp(&r, &g_GB.r, sizeof(r)));
    }
    gb_destroy();
}

TEST(Loads16Bit, LD_R_d16) {

    gb_initialize(NULL, 0);
    op_def_t oa[] = {
        { 1,   "LD BC,d16", 3, 12, "- - - -" },
        { 17, "LD DE,d16", 3, 12, "- - - -" },
        { 33,  "LD HL,d16", 3, 12, "- - - -" },
        { 49,  "LD SP,d16", 3, 12, "- - - -" },
    };

    gb_dword_t* r[] = { &g_GB.r.BC, &g_GB.r.DE, &g_GB.r.HL, &g_GB.r.SP };

    //Check that the number we load from d16 ends up indeed in the register
    for (int i = 0; i < COUNT_OF(oa); i++) {

        EXPECT_FALSE(g_GB.ops[oa[i].opcode].handler == NULL);
        EXPECT_EQ(g_GB.ops[oa[i].opcode].size, oa[i].op_size);

        for (gb_dword_t d16 = 0; d16 < 0xFFFF; d16 += 5) {

            byte_t cycles = g_GB.ops[oa[i].opcode].handler(oa[i].opcode, d16);
            EXPECT_EQ(d16, *r[i]);
            EXPECT_EQ(cycles, oa[i].cpu_cycles);
        }
        //EXPECT_TRUE(0 == std::memcmp(&r, &g_GB.r, sizeof(r)));
    }
    gb_destroy();
}

TEST(Loads8Bit, LD_R_R) {

    //too lazy to make it separate tests
    gb_initialize(NULL, 0);
    gbdbg_mute_print(g_GB.dbg, true);
    op_def_t oa[] = {
        {64, "LD B,B", 1, 4, "- - - -"},
        {65, "LD B,C", 1, 4, "- - - -"},
        {66, "LD B,D", 1, 4, "- - - -"},
        {67, "LD B,E", 1, 4, "- - - -"},
        {68, "LD B,H", 1, 4, "- - - -"},
        {69, "LD B,L", 1, 4, "- - - -"},
        {70, "LD B,{HL}", 1, 8, "- - - -"},
        {71, "LD B,A", 1, 4, "- - - -"},
        {72, "LD C,B", 1, 4, "- - - -"},
        {73, "LD C,C", 1, 4, "- - - -"},
        {74, "LD C,D", 1, 4, "- - - -"},
        {75, "LD C,E", 1, 4, "- - - -"},
        {76, "LD C,H", 1, 4, "- - - -"},
        {77, "LD C,L", 1, 4, "- - - -"},
        {78, "LD C,{HL}", 1, 8, "- - - -"},
        {79, "LD C,A", 1, 4, "- - - -"},
        {80, "LD D,B", 1, 4, "- - - -"},
        {81, "LD D,C", 1, 4, "- - - -"},
        {82, "LD D,D", 1, 4, "- - - -"},
        {83, "LD D,E", 1, 4, "- - - -"},
        {84, "LD D,H", 1, 4, "- - - -"},
        {85, "LD D,L", 1, 4, "- - - -"},
        {86, "LD D,{HL}", 1, 8, "- - - -"},
        {87, "LD D,A", 1, 4, "- - - -"},
        {88, "LD E,B", 1, 4, "- - - -"},
        {89, "LD E,C", 1, 4, "- - - -"},
        {90, "LD E,D", 1, 4, "- - - -"},
        {91, "LD E,E", 1, 4, "- - - -"},
        {92, "LD E,H", 1, 4, "- - - -"},
        {93, "LD E,L", 1, 4, "- - - -"},
        {94, "LD E,{HL}", 1, 8, "- - - -"},
        {95, "LD E,A", 1, 4, "- - - -"},
        {96, "LD H,B", 1, 4, "- - - -"},
        {97, "LD H,C", 1, 4, "- - - -"},
        {98, "LD H,D", 1, 4, "- - - -"},
        {99, "LD H,E", 1, 4, "- - - -"},
        {100, "LD H,H", 1, 4, "- - - -"},
        {101, "LD H,L", 1, 4, "- - - -"},
        {102, "LD H,{HL}", 1, 8, "- - - -"},
        {103, "LD H,A", 1, 4, "- - - -"},
        {104, "LD L,B", 1, 4, "- - - -"},
        {105, "LD L,C", 1, 4, "- - - -"},
        {106, "LD L,D", 1, 4, "- - - -"},
        {107, "LD L,E", 1, 4, "- - - -"},
        {108, "LD L,H", 1, 4, "- - - -"},
        {109, "LD L,L", 1, 4, "- - - -"},
        {110, "LD L,{HL}", 1, 8, "- - - -"},
        {111, "LD L,A", 1, 4, "- - - -"},
        {112, "LD {HL},B", 1, 8, "- - - -"},
        {113, "LD {HL},C", 1, 8, "- - - -"},
        {114, "LD {HL},D", 1, 8, "- - - -"},
        {115, "LD {HL},E", 1, 8, "- - - -"},
        {116, "LD {HL},H", 1, 8, "- - - -"},
        {117, "LD {HL},L", 1, 8, "- - - -"},
        {0,   "NOP",       1, 4, "- - - -" }, //FAKE OP to align number of ops to 64
        {119, "LD {HL},A", 1, 8, "- - - -"},
        {120, "LD A,B", 1, 4, "- - - -"},
        {121, "LD A,C", 1, 4, "- - - -"},
        {122, "LD A,D", 1, 4, "- - - -"},
        {123, "LD A,E", 1, 4, "- - - -"},
        {124, "LD A,H", 1, 4, "- - - -"},
        {125, "LD A,L", 1, 4, "- - - -"},
        {126, "LD A,{HL}", 1, 8, "- - - -"},
        {127, "LD A,A", 1, 4, "- - - -"}
    };

    gb_word_t *r[] = { &g_GB.r.B, &g_GB.r.C,
                       &g_GB.r.D, &g_GB.r.E,
                       &g_GB.r.H, &g_GB.r.L,
                       HL_INDIRECT, &g_GB.r.A };
    


    //some arbitrary test values:
    gb_word_t test_values[] = { 
        0x00, 0xF0, 0x02, 0x04, 0x08, 0x0A, 0x00, 0xFC, 0x0C, 0x01, 
        0xAA, 0x88, 0x38, 0x9F, 0xCB, 0xBB, 0x00, 0x27, 0x61, 0x77
    };

    static_assert(COUNT_OF(oa) == COUNT_OF(r)*COUNT_OF(r),
                        "number of opcodes does not equal to square of number of registers -1 ");

    //Check that the number we load from d16 ends up indeed in the register
    int i = 0;
    bool is_mutating_hl;

    for (int d = 0; d < COUNT_OF(r); d++) {

        for (int s = 0; s < COUNT_OF(r); s++,i++) { //<-- `i` is incremented here, look!
            
            EXPECT_FALSE(g_GB.ops[oa[i].opcode].handler == NULL);
            EXPECT_EQ(g_GB.ops[oa[i].opcode].size, oa[i].op_size);
            
            //side effect verify -- hl should never be modified in this test
            gb_word_t *dst = r[d];
            gb_word_t *src = r[s];
            //printf("%s\n", oa[i].op_name);
            if (dst || src) { //otherwise we have case of LD {HL}, {HL} which is illegal op
                is_mutating_hl = false;
                if (dst == HL_INDIRECT) {
                    //HL will be mutating when we write test values from
                    //source registers H or L
                    if (r[s] == &g_GB.r.H || r[s] == &g_GB.r.L)  is_mutating_hl = true;
                    else {
                        g_GB.r.HL = 0x8080;
                        dst = gb_MMU_access_internal(g_GB.r.HL);
                    }
                }
                

                if (src == HL_INDIRECT) {
                    g_GB.r.HL = 0x8080;
                    src = gb_MMU_access_internal(g_GB.r.HL);
                }

                

                for (int vi = 0; vi < COUNT_OF(test_values); vi++) {
                    *src = test_values[vi];
                    if (is_mutating_hl) {
                        dst = gb_MMU_access_internal(g_GB.r.HL);
                    }
                    
                    byte_t cycles = g_GB.ops[oa[i].opcode].handler(oa[i].opcode, 0x0);
                    EXPECT_EQ(*src, *dst);
                    EXPECT_EQ(test_values[vi], *dst);
                    g_GB.r.HL = 0x8080;
                }

            }
        }
    }
    gbdbg_mute_print(g_GB.dbg, false);
    gb_destroy();
}


TEST(ALU16Bit, INC_R) {

    gb_initialize(NULL, 0);

    op_def_t oa[] = {
        { 3, "INC BC", 1, 8, "- - - -" },
        { 19, "INC DE", 1, 8, "- - - -" },
        { 35, "INC HL", 1, 8, "- - - -" },
        { 51, "INC SP", 1, 8, "- - - -" },
    };
    gb_dword_t* r[] = { &g_GB.r.BC, &g_GB.r.DE,&g_GB.r.HL,&g_GB.r.SP };

    gb_dword_t input[] = { 0x0000, 0x00FF, 0xFF00, 0xFFFE, 0xFFFF };
    gb_dword_t expected[] = { 0x0001, 0x0100, 0xFF01, 0xFFFF, 0x0000 };

    static_assert(COUNT_OF(r) == COUNT_OF(oa), "register mapping size is invalid");
    static_assert(COUNT_OF(input) == COUNT_OF(expected),
                  "numbers of expected outputs does not match number of inputs");

    for (int i = 0; i < COUNT_OF(oa); i++) {

        EXPECT_FALSE(g_GB.ops[oa[i].opcode].handler == NULL);
        EXPECT_EQ(g_GB.ops[oa[i].opcode].size, oa[i].op_size);

        for (int t = 0; t < COUNT_OF(input); t++) {
            *r[i] = input[t];
            byte_t cycles = g_GB.ops[oa[i].opcode].handler(oa[i].opcode, 0);
            EXPECT_EQ(cycles, oa[i].cpu_cycles);
            EXPECT_EQ(expected[t], *r[i]);
        }
    }
    gb_destroy();
}

TEST(ALU16Bit, DEC_R) {

    gb_initialize(NULL, 0);

    op_def_t oa[] = {
        { 11, "DEC BC", 1, 8, "- - - -" },
        { 27, "DEC DE", 1, 8, "- - - -" },
        { 43, "DEC HL", 1, 8, "- - - -" },
        { 59, "DEC SP", 1, 8, "- - - -" },
    };
    gb_dword_t* r[] = { &g_GB.r.BC, &g_GB.r.DE,&g_GB.r.HL,&g_GB.r.SP };

    gb_dword_t initial[] = { 0x0000, 0x00FF, 0xFF00, 0xFFFE, 0xFFFF };
    gb_dword_t expected[] = { 0xFFFF, 0x00FE, 0xFEFF, 0xFFFD, 0xFFFE };

    static_assert(COUNT_OF(r) == COUNT_OF(oa), "register mapping size is invalid");
    static_assert(COUNT_OF(initial) == COUNT_OF(expected),
                  "numbers of expected outputs does not match number of inputs");

    for (int i = 0; i < COUNT_OF(oa); i++) {

        EXPECT_FALSE(g_GB.ops[oa[i].opcode].handler == NULL);
        EXPECT_EQ(g_GB.ops[oa[i].opcode].size, oa[i].op_size);

        for (int t = 0; t < COUNT_OF(initial); t++) {
            *r[i] = initial[t];
            byte_t cycles = g_GB.ops[oa[i].opcode].handler(oa[i].opcode, 0);
            EXPECT_EQ(cycles, oa[i].cpu_cycles);
            EXPECT_EQ(expected[t], *r[i]);
        }
    }
    gb_destroy();
}


TEST(ALU16Bit, ADD_HL_R) {

    gb_initialize(NULL, 0);
    gb_word_t const none = 0x0, H = GB_FLAG_H, C = GB_FLAG_C;
    op_def_t const oa[]  = {
        { 9, "ADD HL,BC", 1, 8, "- 0 H C" },
        { 25, "ADD HL,DE", 1, 8, "- 0 H C" },
        { 57, "ADD HL,SP", 1, 8, "- 0 H C" },
    };
    gb_dword_t * const r[] = { &g_GB.r.BC, &g_GB.r.DE,&g_GB.r.SP };

    gb_dword_t const initial[]  = { 0x0000, 0x0FFF, 0xFFFF, 0x0001, 0xFF00, 0xFFFF, 0xF000 };
    gb_dword_t const addend[]   = { 0x0000, 0x00FF, 0x0001, 0xFFFF, 0x00FF, 0xFFFF, 0xF000 };
    gb_dword_t const expected[] = { 0x0000, 0x10FE, 0x0000, 0x0000, 0xFFFF, 0xFFFE, 0xE000  };
    gb_word_t const flags[]     = { none, H, H | C, H | C, none, H | C, C };
        
    static_assert(COUNT_OF(r) == COUNT_OF(oa), "register mapping size is invalid");
    static_assert(COUNT_OF(initial) == COUNT_OF(addend) ,"wrong1");
    static_assert(COUNT_OF(expected) == COUNT_OF(flags), "wrong2");
    static_assert(COUNT_OF(initial) == COUNT_OF(flags), "wrong3");


    for (int i = 0; i < COUNT_OF(oa); i++) {

        EXPECT_FALSE(g_GB.ops[oa[i].opcode].handler == NULL);
        EXPECT_EQ(g_GB.ops[oa[i].opcode].size, oa[i].op_size);
        for (int t = 0; t < COUNT_OF(initial); t++) {
            g_GB.r.F  = 0x00;
            g_GB.r.HL = initial[t];
            *r[i]     = addend[t];

            byte_t cycles = g_GB.ops[oa[i].opcode].handler(oa[i].opcode, 0);
            EXPECT_EQ(cycles, oa[i].cpu_cycles);
            EXPECT_EQ(expected[t], g_GB.r.HL);
            EXPECT_EQ(flags[t], g_GB.r.F);

        }
    }
    gb_destroy();
}
TEST(ALU16Bit, ADD_HL_HL) {
    op_def_t op = { 41, "ADD HL,HL", 1, 8, "- 0 H C" };

    gb_initialize(NULL, 0);

    gb_dword_t const initial[]  = { 0x0000, 0x0FFF, 0x0001, 0xFF00, 0xFFFF, 0xF000 };
    gb_dword_t const expected[] = { 0x0000, 0x1FFE, 0x0002, 0xFE00, 0xFFFE, 0xE000 };
    gb_word_t const flags[]     = { f_0,  f_h,      f_0,   f_hc,   f_hc, f_c };

    EXPECT_FALSE(g_GB.ops[op.opcode].handler == NULL);
    EXPECT_EQ(g_GB.ops[op.opcode].size, op.op_size);

    for (int t = 0; t < COUNT_OF(initial); t++) {
        g_GB.r.F = 0x00;
        g_GB.r.HL = initial[t];

        byte_t cycles = g_GB.ops[op.opcode].handler(op.opcode, 0);
        EXPECT_EQ(cycles, op.cpu_cycles);
        EXPECT_EQ(expected[t], g_GB.r.HL);
        EXPECT_EQ(flags[t], g_GB.r.F);

    }
    gb_destroy();
}



TEST(RotShift, RRCA) {
    gb_initialize(NULL, 0);
    op_def_t op = { 15, "RRCA", 1, 4, "0 0 0 C" };
    gb_word_t const initial[]  = { 0x00, 0x01, 0x02, 0xF0, 0xFF, 0x0F };
    gb_word_t const expected[] = { 0x00, 0x80, 0x01, 0x78, 0xFF, 0x87 };
    gb_word_t const flags[]    = { f_0, f_c, f_0, f_0,  f_c,  f_c };


    EXPECT_FALSE(g_GB.ops[op.opcode].handler == NULL);
    EXPECT_EQ(g_GB.ops[op.opcode].size, op.op_size);

    for (int t = 0; t < COUNT_OF(initial); t++) {
        g_GB.r.F = 0x00;
        g_GB.r.A = initial[t];
        byte_t cycles = g_GB.ops[op.opcode].handler(op.opcode, 0);
        EXPECT_EQ(cycles, op.cpu_cycles);
        EXPECT_EQ(expected[t], g_GB.r.A);
        EXPECT_EQ(flags[t],    g_GB.r.F);
    }
    gb_destroy();
}


TEST(RotShift, RRA) {

    gb_initialize(NULL, 0);
    enum flags {
        f_0 = 0x00,
        f_c = f_c,
    };

    op_def_t op = { 31, "RRA", 1, 4, "0 0 0 C" };

    gb_word_t const initial[]    = { 0x00, 0x01, 0x02, 0xF1, 0xFF, 0x00, 0x01 };
    gb_word_t const expected[]   = { 0x00, 0x00, 0x01, 0x78, 0x7F, 0x80, 0x80 };
    gb_word_t const flags_init[] = { f_0,  f_0,  f_0,  f_0,  f_0,  f_c,  f_c};
    gb_word_t const flags_expc[] = { f_0,  f_c,  f_0,  f_c,  f_c,  f_0 , f_c };


    EXPECT_FALSE(g_GB.ops[op.opcode].handler == NULL);
    EXPECT_EQ(g_GB.ops[op.opcode].size, op.op_size);

    for (int t = 0; t < COUNT_OF(initial); t++) {
        g_GB.r.F = flags_init[t];
        g_GB.r.A = initial[t];
        byte_t cycles = g_GB.ops[op.opcode].handler(op.opcode, 0);
        EXPECT_EQ(cycles, op.cpu_cycles);
        EXPECT_EQ(expected[t], g_GB.r.A);
        EXPECT_EQ(flags_expc[t], g_GB.r.F);
    }
    gb_destroy();
}


TEST(RotShift, RLCA) {
    gb_initialize(NULL, 0);
    op_def_t op = { 7, "RLCA", 1, 4, "0 0 0 C" };
    gb_word_t const initial[]  = { 0x00, 0x80, 0x01, 0x78, 0xFF, 0x87 };
    gb_word_t const expected[] = { 0x00, 0x01, 0x02, 0xF0, 0xFF, 0x0F };
    gb_word_t const flags[]    = { f_0,  f_c,  f_0,  f_0,  f_c,  f_c };


    EXPECT_FALSE(g_GB.ops[op.opcode].handler == NULL);
    EXPECT_EQ(g_GB.ops[op.opcode].size, op.op_size);

    for (int t = 0; t < COUNT_OF(initial); t++) {
        g_GB.r.F = 0x00;
        g_GB.r.A = initial[t];
        byte_t cycles = g_GB.ops[op.opcode].handler(op.opcode, 0);
        EXPECT_EQ(cycles, op.cpu_cycles);
        EXPECT_EQ(expected[t], g_GB.r.A);
        EXPECT_EQ(flags[t], g_GB.r.F);
    }
    gb_destroy();
}


TEST(RotShift, RLA) {

    gb_initialize(NULL, 0);
    op_def_t op = { 23, "RLA", 1, 4, "0 0 0 C" };

    gb_word_t const initial[]  = { 0x00, 0x00, 0x01, 0x78, 0x7F, 0x80, 0x80 };
    gb_word_t const expected[] = { 0x00, 0x01, 0x02, 0xF1, 0xFF, 0x00, 0x01 };
    gb_word_t const flags_init[] = { f_0,  f_c,  f_0,  f_c,  f_c,  f_0,  f_c };
    gb_word_t const flags_expc[] = { f_0,  f_0,  f_0,  f_0,  f_0,  f_c , f_c };


    EXPECT_FALSE(g_GB.ops[op.opcode].handler == NULL);
    EXPECT_EQ(g_GB.ops[op.opcode].size, op.op_size);

    for (int t = 0; t < COUNT_OF(initial); t++) {
        g_GB.r.F = flags_init[t];
        g_GB.r.A = initial[t];
        byte_t cycles = g_GB.ops[op.opcode].handler(op.opcode, 0);
        EXPECT_EQ(cycles, op.cpu_cycles);
        EXPECT_EQ(expected[t], g_GB.r.A);
        EXPECT_EQ(flags_expc[t], g_GB.r.F);
    }
    gb_destroy();
}


void DAA_reference() {

    int f = g_GB.r.F;
    int a = g_GB.r.A;

    if (!((f & f_n)== f_n)) {
        if (((f & f_h) == f_h) || (a & 0xF) > 9)
            a += 0x06;

        if (((f & f_c) == f_c) || a > 0x9F)
            a += 0x60;
    } else {
        if (((f & f_h) == f_h))
            a = (a - 6) & 0xFF;

        if (((f & f_c) == f_c))
            a -= 0x60;
    }

    g_GB.r.F &= ~(f_zh);

    if ((a & 0x100) == 0x100)
        g_GB.r.F |= f_c;

    a &= 0xFF;

    if (a == 0)
        g_GB.r.F |= f_z;

    g_GB.r.A = (byte_t)a;
}


TEST(DAA_compliance, DAA_comparison) {

    gb_initialize(NULL, 0);
    op_def_t op = {39, "DAA", 1, 4, "Z - 0 C"};


    EXPECT_FALSE(g_GB.ops[op.opcode].handler == NULL);
    EXPECT_EQ(g_GB.ops[op.opcode].size, op.op_size);

    for (int A_val = 0; A_val <= 0xFF; A_val++) {
        for (uint_fast8_t f = 0; f < 0x0F; f++) {
            uint_fast8_t flags = f << 4; // we want to iterate over all flags permutations
            //set values 
            g_GB.r.A = A_val;
            g_GB.r.F = flags;
            DAA_reference();

            gb_dword_t reference_result = g_GB.r.AF;

            g_GB.r.A = A_val;
            g_GB.r.F = flags;
            byte_t cycles = g_GB.ops[op.opcode].handler(op.opcode, 0);

            EXPECT_EQ(cycles, op.cpu_cycles);
            EXPECT_EQ(reference_result, g_GB.r.AF);
            if (reference_result != g_GB.r.AF) {
                printf("DebugData-Input A:0x%hhx,F:0x%hhx, REF AF:0x%hx, GB AF:0x%hx", 
                       A_val, flags, reference_result, g_GB.r.AF);
                
            }
        }
    }
    gb_destroy();
}




void adc_reference(gb_word_t * const R)
{
    gb_word_t x = *R;
    uint8_t flag_c = ((g_GB.r.F & f_c) == f_c) ? (1) : (0);
    uint16_t rh = g_GB.r.A + x + flag_c;
    uint16_t rl = (g_GB.r.A & 0x0f) + (x & 0x0f) + flag_c;
    g_GB.r.A = rh & 0xFF;

    g_GB.r.F = 0;
    g_GB.r.F |= ((uint8_t)rh == 0) ? f_z : 0;
    g_GB.r.F |= (rl > 0x0f) ? f_h : 0;
    g_GB.r.F |= (rh > 0xff) ? f_c : 0;

}


TEST(ALU8bit, ADC_compliance) {
    gb_initialize(NULL, 0);
    gbdbg_mute_print(g_GB.dbg, true);
    op_def_t oa[] = {

        { 136, "ADC A,B", 1, 4, "Z 0 H C" },
        { 137, "ADC A,C", 1, 4, "Z 0 H C" },
        { 138, "ADC A,D", 1, 4, "Z 0 H C" },
        { 139, "ADC A,E", 1, 4, "Z 0 H C" },
        { 140, "ADC A,H", 1, 4, "Z 0 H C" },
        { 141, "ADC A,L", 1, 4, "Z 0 H C" },
        { 142, "ADC A,{HL}", 1, 8, "Z 0 H C" },
        { 143, "ADC A,A", 1, 4, "Z 0 H C" },
    };
    gb_word_t * const reg[] = {
        &g_GB.r.B,
        &g_GB.r.C,
        &g_GB.r.D,
        &g_GB.r.E,
        &g_GB.r.H,
        &g_GB.r.L,
        NULL,
        &g_GB.r.A,
    };

    gb_word_t flags[] = { f_c, f_0 };

    for (int i = 0; i < COUNT_OF(oa); i++) {

        EXPECT_FALSE(g_GB.ops[oa[i].opcode].handler == NULL);
        EXPECT_EQ(g_GB.ops[oa[i].opcode].size, oa[i].op_size);
        
        gb_word_t * const reg_ptr = reg[i] ? reg[i] : gb_MMU_access_internal(g_GB.r.HL);
        


        for (int A = 0x0; A <= 0xFF; A++) {
            printf("%s", oa[i].op_name);
            for (int R = 0x0; R <= 0xFF; R++) {
                for (int f = 0; f < COUNT_OF(flags); f++) {

                    g_GB.r.F = flags[f];
                    g_GB.r.A = A;
                    *reg_ptr = R;
                    adc_reference(reg_ptr);

                    gb_word_t ref_result_A = g_GB.r.A;
                    gb_word_t ref_result_F = g_GB.r.F;

                    g_GB.r.F = flags[f];
                    g_GB.r.A = A;
                    *reg_ptr = R;

                    byte_t cycles = g_GB.ops[oa[i].opcode].handler(oa[i].opcode, 0);
                        
                    EXPECT_EQ(cycles, oa[i].cpu_cycles);
                    
                    EXPECT_EQ(ref_result_A, g_GB.r.A);
                    EXPECT_EQ(ref_result_F, g_GB.r.F);
                    if (ref_result_A != g_GB.r.A || ref_result_F != g_GB.r.F) {
                        
                        printf("INSTRUCTION FAIL:%s\n"
                               "Initial data A:0x%hhx, R:0x%hhx, F:0x%hhx, \n"
                               "referenceA:0x%hhx, referenceF:0x%hhx,\n"
                               "actualA:0x%hhx, actualF:0x%hhx\n\n",
                               oa[i].op_name, A, R, flags[f],
                               ref_result_A, ref_result_F,
                               g_GB.r.A, g_GB.r.F
                        );
                    }
                }
            }
        }
    }
    gb_destroy();



}


/*
//{0, "NOP", 1, 4, "- - - -"},
//{1, "LD BC,d16", 3, 12, "- - - -"},
{2, "LD {BC},A", 1, 8, "- - - -"},
//{3, "INC BC", 1, 8, "- - - -"},
{4, "INC B", 1, 4, "Z 0 H -"},
{5, "DEC B", 1, 4, "Z 1 H -"},
{6, "LD B,d8", 2, 8, "- - - -"},
//{7, "RLCA", 1, 4, "0 0 0 C"},
{8, "LD {a16},SP", 3, 20, "- - - -"},
//{9, "ADD HL,BC", 1, 8, "- 0 H C"},
{10, "LD A,{BC}", 1, 8, "- - - -"},
//{11, "DEC BC", 1, 8, "- - - -"},
{12, "INC C", 1, 4, "Z 0 H -"},
{13, "DEC C", 1, 4, "Z 1 H -"},
{14, "LD C,d8", 2, 8, "- - - -"},
//{15, "RRCA", 1, 4, "0 0 0 C"},
{16, "STOP 0", 2, 4, "- - - -"},
//{17, "LD DE,d16", 3, 12, "- - - -"},
{18, "LD {DE},A", 1, 8, "- - - -"},
//{19, "INC DE", 1, 8, "- - - -"},
{20, "INC D", 1, 4, "Z 0 H -"},
{21, "DEC D", 1, 4, "Z 1 H -"},
{22, "LD D,d8", 2, 8, "- - - -"},
//{23, "RLA", 1, 4, "0 0 0 C"},
{24, "JR r8", 2, 12, "- - - -"},
//{25, "ADD HL,DE", 1, 8, "- 0 H C"},
{26, "LD A,{DE}", 1, 8, "- - - -"},
//{27, "DEC DE", 1, 8, "- - - -"},
{28, "INC E", 1, 4, "Z 0 H -"},
{29, "DEC E", 1, 4, "Z 1 H -"},
{30, "LD E,d8", 2, 8, "- - - -"},
//{31, "RRA", 1, 4, "0 0 0 C"},
{32, "JR NZ,r8", 2, "12/8", "- - - -"},
//{33, "LD HL,d16", 3, 12, "- - - -"},
//{34, "LD {HL+},A", 1, 8, "- - - -"},
//{35, "INC HL", 1, 8, "- - - -"},
{36, "INC H", 1, 4, "Z 0 H -"},
{37, "DEC H", 1, 4, "Z 1 H -"},
{38, "LD H,d8", 2, 8, "- - - -"},
{39, "DAA", 1, 4, "Z - 0 C"},
{40, "JR Z,r8", 2, "12/8", "- - - -"},
//{41, "ADD HL,HL", 1, 8, "- 0 H C"},
//{42, "LD A,{HL+}", 1, 8, "- - - -"},
{43, "DEC HL", 1, 8, "- - - -"},
{44, "INC L", 1, 4, "Z 0 H -"},
{45, "DEC L", 1, 4, "Z 1 H -"},
{46, "LD L,d8", 2, 8, "- - - -"},
{47, "CPL", 1, 4, "- 1 1 -"},
{48, "JR NC,r8", 2, "12/8", "- - - -"},
//{49, "LD SP,d16", 3, 12, "- - - -"},
//{50, "LD {HL-},A", 1, 8, "- - - -"},
//{51, "INC SP", 1, 8, "- - - -"},
{52, "INC {HL}", 1, 12, "Z 0 H -"},
{53, "DEC {HL}", 1, 12, "Z 1 H -"},
{54, "LD {HL},d8", 2, 12, "- - - -"},
{55, "SCF", 1, 4, "- 0 0 1"},
{56, "JR C,r8", 2, "12/8", "- - - -"},
//{57, "ADD HL,SP", 1, 8, "- 0 H C"},
//{58, "LD A,{HL-}", 1, 8, "- - - -"},
{59, "DEC SP", 1, 8, "- - - -"},
{60, "INC A", 1, 4, "Z 0 H -"},
{61, "DEC A", 1, 4, "Z 1 H -"},
{62, "LD A,d8", 2, 8, "- - - -"},
{63, "CCF", 1, 4, "- 0 0 C"},
//{64, "LD B,B", 1, 4, "- - - -"},
//{65, "LD B,C", 1, 4, "- - - -"},
//{66, "LD B,D", 1, 4, "- - - -"},
//{67, "LD B,E", 1, 4, "- - - -"},
//{68, "LD B,H", 1, 4, "- - - -"},
//{69, "LD B,L", 1, 4, "- - - -"},
//{70, "LD B,{HL}", 1, 8, "- - - -"},
//{71, "LD B,A", 1, 4, "- - - -"},
//{72, "LD C,B", 1, 4, "- - - -"},
//{73, "LD C,C", 1, 4, "- - - -"},
//{74, "LD C,D", 1, 4, "- - - -"},
//{75, "LD C,E", 1, 4, "- - - -"},
//{76, "LD C,H", 1, 4, "- - - -"},
//{77, "LD C,L", 1, 4, "- - - -"},
//{78, "LD C,{HL}", 1, 8, "- - - -"},
//{79, "LD C,A", 1, 4, "- - - -"},
//{80, "LD D,B", 1, 4, "- - - -"},
//{81, "LD D,C", 1, 4, "- - - -"},
//{82, "LD D,D", 1, 4, "- - - -"},
//{83, "LD D,E", 1, 4, "- - - -"},
//{84, "LD D,H", 1, 4, "- - - -"},
//{85, "LD D,L", 1, 4, "- - - -"},
//{86, "LD D,{HL}", 1, 8, "- - - -"},
//{87, "LD D,A", 1, 4, "- - - -"},
//{88, "LD E,B", 1, 4, "- - - -"},
//{89, "LD E,C", 1, 4, "- - - -"},
//{90, "LD E,D", 1, 4, "- - - -"},
//{91, "LD E,E", 1, 4, "- - - -"},
//{92, "LD E,H", 1, 4, "- - - -"},
//{93, "LD E,L", 1, 4, "- - - -"},
//{94, "LD E,{HL}", 1, 8, "- - - -"},
//{95, "LD E,A", 1, 4, "- - - -"},
//{96, "LD H,B", 1, 4, "- - - -"},
//{97, "LD H,C", 1, 4, "- - - -"},
//{98, "LD H,D", 1, 4, "- - - -"},
//{99, "LD H,E", 1, 4, "- - - -"},
//{100, "LD H,H", 1, 4, "- - - -"},
//{101, "LD H,L", 1, 4, "- - - -"},
//{102, "LD H,{HL}", 1, 8, "- - - -"},
//{103, "LD H,A", 1, 4, "- - - -"},
//{104, "LD L,B", 1, 4, "- - - -"},
//{105, "LD L,C", 1, 4, "- - - -"},
//{106, "LD L,D", 1, 4, "- - - -"},
//{107, "LD L,E", 1, 4, "- - - -"},
//{108, "LD L,H", 1, 4, "- - - -"},
//{109, "LD L,L", 1, 4, "- - - -"},
//{110, "LD L,{HL}", 1, 8, "- - - -"},
//{111, "LD L,A", 1, 4, "- - - -"},
//{112, "LD {HL},B", 1, 8, "- - - -"},
//{113, "LD {HL},C", 1, 8, "- - - -"},
//{114, "LD {HL},D", 1, 8, "- - - -"},
//{115, "LD {HL},E", 1, 8, "- - - -"},
//{116, "LD {HL},H", 1, 8, "- - - -"},
//{117, "LD {HL},L", 1, 8, "- - - -"},
{118, "HALT", 1, 4, "- - - -"},
//{119, "LD {HL},A", 1, 8, "- - - -"},
//{120, "LD A,B", 1, 4, "- - - -"},
//{121, "LD A,C", 1, 4, "- - - -"},
//{122, "LD A,D", 1, 4, "- - - -"},
//{123, "LD A,E", 1, 4, "- - - -"},
//{124, "LD A,H", 1, 4, "- - - -"},
//{125, "LD A,L", 1, 4, "- - - -"},
//{126, "LD A,{HL}", 1, 8, "- - - -"},
//{127, "LD A,A", 1, 4, "- - - -"},
{128, "ADD A,B", 1, 4, "Z 0 H C"},
{129, "ADD A,C", 1, 4, "Z 0 H C"},
{130, "ADD A,D", 1, 4, "Z 0 H C"},
{131, "ADD A,E", 1, 4, "Z 0 H C"},
{132, "ADD A,H", 1, 4, "Z 0 H C"},
{133, "ADD A,L", 1, 4, "Z 0 H C"},
{134, "ADD A,{HL}", 1, 8, "Z 0 H C"},
{135, "ADD A,A", 1, 4, "Z 0 H C"},
{136, "ADC A,B", 1, 4, "Z 0 H C"},
{137, "ADC A,C", 1, 4, "Z 0 H C"},
{138, "ADC A,D", 1, 4, "Z 0 H C"},
{139, "ADC A,E", 1, 4, "Z 0 H C"},
{140, "ADC A,H", 1, 4, "Z 0 H C"},
{141, "ADC A,L", 1, 4, "Z 0 H C"},
{142, "ADC A,{HL}", 1, 8, "Z 0 H C"},
{143, "ADC A,A", 1, 4, "Z 0 H C"},
{144, "SUB B", 1, 4, "Z 1 H C"},
{145, "SUB C", 1, 4, "Z 1 H C"},
{146, "SUB D", 1, 4, "Z 1 H C"},
{147, "SUB E", 1, 4, "Z 1 H C"},
{148, "SUB H", 1, 4, "Z 1 H C"},
{149, "SUB L", 1, 4, "Z 1 H C"},
{150, "SUB {HL}", 1, 8, "Z 1 H C"},
{151, "SUB A", 1, 4, "Z 1 H C"},
{152, "SBC A,B", 1, 4, "Z 1 H C"},
{153, "SBC A,C", 1, 4, "Z 1 H C"},
{154, "SBC A,D", 1, 4, "Z 1 H C"},
{155, "SBC A,E", 1, 4, "Z 1 H C"},
{156, "SBC A,H", 1, 4, "Z 1 H C"},
{157, "SBC A,L", 1, 4, "Z 1 H C"},
{158, "SBC A,{HL}", 1, 8, "Z 1 H C"},
{159, "SBC A,A", 1, 4, "Z 1 H C"},
{160, "AND B", 1, 4, "Z 0 1 0"},
{161, "AND C", 1, 4, "Z 0 1 0"},
{162, "AND D", 1, 4, "Z 0 1 0"},
{163, "AND E", 1, 4, "Z 0 1 0"},
{164, "AND H", 1, 4, "Z 0 1 0"},
{165, "AND L", 1, 4, "Z 0 1 0"},
{166, "AND {HL}", 1, 8, "Z 0 1 0"},
{167, "AND A", 1, 4, "Z 0 1 0"},
{168, "XOR B", 1, 4, "Z 0 0 0"},
{169, "XOR C", 1, 4, "Z 0 0 0"},
{170, "XOR D", 1, 4, "Z 0 0 0"},
{171, "XOR E", 1, 4, "Z 0 0 0"},
{172, "XOR H", 1, 4, "Z 0 0 0"},
{173, "XOR L", 1, 4, "Z 0 0 0"},
{174, "XOR {HL}", 1, 8, "Z 0 0 0"},
{175, "XOR A", 1, 4, "Z 0 0 0"},
{176, "OR B", 1, 4, "Z 0 0 0"},
{177, "OR C", 1, 4, "Z 0 0 0"},
{178, "OR D", 1, 4, "Z 0 0 0"},
{179, "OR E", 1, 4, "Z 0 0 0"},
{180, "OR H", 1, 4, "Z 0 0 0"},
{181, "OR L", 1, 4, "Z 0 0 0"},
{182, "OR {HL}", 1, 8, "Z 0 0 0"},
{183, "OR A", 1, 4, "Z 0 0 0"},
{184, "CP B", 1, 4, "Z 1 H C"},
{185, "CP C", 1, 4, "Z 1 H C"},
{186, "CP D", 1, 4, "Z 1 H C"},
{187, "CP E", 1, 4, "Z 1 H C"},
{188, "CP H", 1, 4, "Z 1 H C"},
{189, "CP L", 1, 4, "Z 1 H C"},
{190, "CP {HL}", 1, 8, "Z 1 H C"},
{191, "CP A", 1, 4, "Z 1 H C"},
{192, "RET NZ", 1, "20/8", "- - - -"},
{193, "POP BC", 1, 12, "- - - -"},
{194, "JP NZ,a16", 3, "16/12", "- - - -"},
{195, "JP a16", 3, 16, "- - - -"},
{196, "CALL NZ,a16", 3, "24/12", "- - - -"},
{197, "PUSH BC", 1, 16, "- - - -"},
{198, "ADD A,d8", 2, 8, "Z 0 H C"},
{199, "RST 00H", 1, 16, "- - - -"},
{200, "RET Z", 1, "20/8", "- - - -"},
{201, "RET", 1, 16, "- - - -"},
{202, "JP Z,a16", 3, "16/12", "- - - -"},
{203, "PREFIX CB", 1, 4, "- - - -"},
{204, "CALL Z,a16", 3, "24/12", "- - - -"},
{205, "CALL a16", 3, 24, "- - - -"},
{206, "ADC A,d8", 2, 8, "Z 0 H C"},
{207, "RST 08H", 1, 16, "- - - -"},
{208, "RET NC", 1, "20/8", "- - - -"},
{209, "POP DE", 1, 12, "- - - -"},
{210, "JP NC,a16", 3, "16/12", "- - - -"},
{212, "CALL NC,a16", 3, "24/12", "- - - -"},
{213, "PUSH DE", 1, 16, "- - - -"},
{214, "SUB d8", 2, 8, "Z 1 H C"},
{215, "RST 10H", 1, 16, "- - - -"},
{216, "RET C", 1, "20/8", "- - - -"},
{217, "RETI", 1, 16, "- - - -"},
{218, "JP C,a16", 3, "16/12", "- - - -"},
{220, "CALL C,a16", 3, "24/12", "- - - -"},
{222, "SBC A,d8", 2, 8, "Z 1 H C"},
{223, "RST 18H", 1, 16, "- - - -"},
{224, "LDH {a8},A", 2, 12, "- - - -"},
{225, "POP HL", 1, 12, "- - - -"},
{226, "LD {C},A", 2, 8, "- - - -"},
{229, "PUSH HL", 1, 16, "- - - -"},
{230, "AND d8", 2, 8, "Z 0 1 0"},
{231, "RST 20H", 1, 16, "- - - -"},
{232, "ADD SP,r8", 2, 16, "0 0 H C"},
{233, "JP {HL}", 1, 4, "- - - -"},
{234, "LD {a16},A", 3, 16, "- - - -"},
{238, "XOR d8", 2, 8, "Z 0 0 0"},
{239, "RST 28H", 1, 16, "- - - -"},
{240, "LDH A,{a8}", 2, 12, "- - - -"},
{241, "POP AF", 1, 12, "Z N H C"},
{242, "LD A,{C}", 2, 8, "- - - -"},
{243, "DI", 1, 4, "- - - -"},
{245, "PUSH AF", 1, 16, "- - - -"},
{246, "OR d8", 2, 8, "Z 0 0 0"},
{247, "RST 30H", 1, 16, "- - - -"},
{248, "LD HL,SP+r8", 2, 12, "0 0 H C"},
{249, "LD SP,HL", 1, 8, "- - - -"},
{250, "LD A,{a16}", 3, 16, "- - - -"},
{251, "EI", 1, 4, "- - - -"},
{254, "CP d8", 2, 8, "Z 1 H C"},
{255, "RST 38H", 1, 16, "- - - -"},
*/


//
//TEST(MemorySection, RangeTest)
//{
//    MMU::MemorySection<uint32_t, 0, 100> ms1;
//    MMU::MemorySection<uint32_t, 100, 200> ms2;
//
//    for (uint32_t i = 0; i < 100; i++) {
//        EXPECT_EQ(0, ms1.read<uint8_t>(i));
//    }
//    for (uint32_t i = 100; i < 200; i++) {
//        EXPECT_EQ(0, ms2.read<uint8_t>(i));
//    }
//
//    for (uint32_t i = 0; i < 100; i+=sizeof(uint16_t)) {
//
//        EXPECT_EQ(0, ms1.read<uint16_t>(i));
//    }
//    for (uint32_t i = 100; i < 200; i += sizeof(uint16_t)) {
//        EXPECT_EQ(0, ms2.read<uint16_t>(i));
//    }
//
//    for (uint32_t i = 0; i < 100; i += sizeof(uint32_t)) {
//        EXPECT_EQ(0, ms1.read<uint32_t>(i));
//    }
//    for (uint32_t i = 100; i < 200; i += sizeof(uint32_t)) {
//        EXPECT_EQ(0, ms2.read<uint32_t>(i));
//    }
//}
//
//TEST(MemorySection, RangeReadWriteTest)
//{
//    MMU::MemorySection<uint32_t, 0, 100> ms1;
//    MMU::MemorySection<uint32_t, 100, 200> ms2;
//
//    for (uint32_t i = 0; i < 100; i++) {
//        ms1.write<uint8_t>(i,i);
//        EXPECT_EQ(i, ms1.read<uint8_t>(i));
//    }
//    for (uint32_t i = 100; i < 200; i++) {
//        ms2.write<uint8_t>(i, i);
//        EXPECT_EQ(i, ms2.read<uint8_t>(i));
//    }
//
//    for (uint32_t i = 0; i < 100; i += sizeof(uint16_t)) {
//        ms1.write<uint16_t>(i, 0xFF+i);
//        EXPECT_EQ(0xFF + i, ms1.read<uint16_t>(i));
//    }
//    for (uint32_t i = 100; i < 200; i += sizeof(uint16_t)) {
//        ms2.write<uint16_t>(i, 0xFF+i);
//        EXPECT_EQ(0xFF + i, ms2.read<uint16_t>(i));
//    }
//
//    for (uint32_t i = 0; i < 100; i += sizeof(uint32_t)) {
//        ms1.write<uint32_t>(i, 0xFFFFFF + i);
//        EXPECT_EQ(0xFFFFFF + i, ms1.read<uint32_t>(i));
//    }
//    for (uint32_t i = 100; i < 200; i += sizeof(uint32_t)) {
//        ms2.write<uint32_t>(i, 0xFFFFFF + i);
//        EXPECT_EQ(0xFFFFFF + i, ms2.read<uint32_t>(i));
//    }
//}
//
//
//TEST(MemorySection, RangeTestXchg)
//{
//    MMU::MemorySection<uint32_t, 0, 100> ms1;
//    MMU::MemorySection<uint32_t, 100, 200> ms2;
//
//    for (uint32_t i = 0; i < 100; i++) {
//        EXPECT_EQ(0, ms1.xchg<uint8_t>(i, i));
//        EXPECT_EQ(i, ms1.read<uint8_t>(i));
//    }
//    for (uint32_t i = 100; i < 200; i++) {
//        EXPECT_EQ(0, ms2.xchg<uint8_t>(i,i));
//        EXPECT_EQ(i, ms2.read<uint8_t>(i));
//    }
//
//    for (uint32_t i = 0; i < 100; i += sizeof(uint16_t)) {
//        uint16_t expected = ((i+1) << 8) + i;
//        EXPECT_EQ(expected, ms1.xchg<uint16_t>(i, 0xFF+i));
//        EXPECT_EQ(0xFF+i, ms1.read<uint16_t>(i));
//    }
//    for (uint32_t i = 100; i < 200; i += sizeof(uint16_t)) {
//        uint16_t expected = ((i + 1) << 8) + i;
//        EXPECT_EQ(expected, ms2.xchg<uint16_t>(i, 0xFF + i));
//        EXPECT_EQ(0xFF + i, ms2.read<uint16_t>(i));
//    }
//
//    for (uint32_t i = 0; i < 100; i += sizeof(uint32_t)) {
//        uint32_t expected = ((0xFF + i+2) << 16) + (i + 0xFF);
//        EXPECT_EQ(expected, ms1.xchg<uint32_t>(i, 0xFFFFFF + i));
//        EXPECT_EQ(0xFFFFFF + i, ms1.read<uint32_t>(i));
//    }
//    for (uint32_t i = 100; i < 200; i += sizeof(uint32_t)) {
//        uint32_t expected = ((0xFF + i + 2) << 16) + (i + 0xFF);
//        EXPECT_EQ(expected, ms2.xchg<uint32_t>(i, 0xFFFFFF + i));
//        EXPECT_EQ(0xFFFFFF + i, ms2.read<uint32_t>(i));
//    }
//}
//
//
//
//
//
//
//TEST(CPU, RegisterRWCheck)
//{
//    using namespace CPU;
//    SharpLR35902 sh;
//    sh.write_reg(reg8::A, 0x1);
//    sh.write_reg(reg8::B, 0x2);
//    sh.write_reg(reg8::C, 0x3);
//    sh.write_reg(reg8::D, 0x4);
//    sh.write_reg(reg8::E, 0x5);
//    sh.write_reg(reg8::F, 0x6);
//    sh.write_reg(reg8::H, 0x7);
//    sh.write_reg(reg8::L, 0x8);
//    sh.write_reg(reg16::PC, 0xF0F0);
//    sh.write_reg(reg16::SP, 0x0F0F);
//
//    EXPECT_EQ(0x1, sh.read_reg(reg8::A));
//    EXPECT_EQ(0x2, sh.read_reg(reg8::B));
//    EXPECT_EQ(0x3, sh.read_reg(reg8::C));
//    EXPECT_EQ(0x4, sh.read_reg(reg8::D));
//    EXPECT_EQ(0x5, sh.read_reg(reg8::E));
//    EXPECT_EQ(0x6, sh.read_reg(reg8::F));
//    EXPECT_EQ(0x7, sh.read_reg(reg8::H));
//    EXPECT_EQ(0x8, sh.read_reg(reg8::L));
//    EXPECT_EQ(0xF0F0, sh.read_reg(reg16::PC));
//    EXPECT_EQ(0x0F0F, sh.read_reg(reg16::SP));
//}
//
//TEST(CPU, RegisterEndianCheck)
//{
//    using namespace CPU;
//    SharpLR35902 sh;
//    /*
//    Register Layout
//    +-------+------+
//    | 15..8 | 7..0 |
//    +-------+------+
//    |   A   |  F   |
//    +-------+------+
//    |   B   |  C   |
//    +-------+------+
//    |   D   |  E   |
//    +-------+------+
//    |   H   |  L   |
//    +-------+------+
//    |       SP     |
//    +--------------+
//    |       PC     |
//    +--------------+
//    */
//    sh.write_reg(reg8::A, 0x1);
//    sh.write_reg(reg8::B, 0x2);
//    sh.write_reg(reg8::C, 0x3);
//    sh.write_reg(reg8::D, 0x4);
//    sh.write_reg(reg8::E, 0x5);
//    sh.write_reg(reg8::F, 0x6);
//    sh.write_reg(reg8::H, 0x7);
//    sh.write_reg(reg8::L, 0x8);
//    sh.write_reg(reg16::PC, 0xF0F0);
//    sh.write_reg(reg16::SP, 0x0F0F);
//
//    EXPECT_EQ( (0x1 << 8) + 0x6, sh.read_reg(reg16::AF));
//    EXPECT_EQ( (0x2 << 8) + 0x3, sh.read_reg(reg16::BC));
//    EXPECT_EQ( (0x4 << 8) + 0x5, sh.read_reg(reg16::DE));
//    EXPECT_EQ( (0x7 << 8) + 0x8, sh.read_reg(reg16::HL));
//    
//    sh.write_reg(reg16::HL, 0x00FF);
//    EXPECT_EQ(0x00, sh.read_reg(reg8::H));
//    EXPECT_EQ(0xFF, sh.read_reg(reg8::L));
//
//    sh.write_reg(reg16::BC, 0x00FF);
//    EXPECT_EQ(0x00, sh.read_reg(reg8::B));
//    EXPECT_EQ(0xFF, sh.read_reg(reg8::C));
//
//    sh.write_reg(reg16::DE, 0x00FF);
//    EXPECT_EQ(0x00, sh.read_reg(reg8::D));
//    EXPECT_EQ(0xFF, sh.read_reg(reg8::E));
//
//    sh.write_reg(reg16::AF, 0x00FF);
//    EXPECT_EQ(0x00, sh.read_reg(reg8::A));
//    EXPECT_EQ(0xFF, sh.read_reg(reg8::F));
// 
//}
//
//
//TEST(CPU, RegisterXchgCheck)
//{
//    using namespace CPU;
//    SharpLR35902 sh;
//    //W->X->R
//
//    //
//    // 8-bit registers
//    //
//    sh.write_reg(reg8::A, 0x1);
//    EXPECT_EQ(0x1, sh.xchg_reg(reg8::A, 0xAA));
//    EXPECT_EQ(0xAA, sh.read_reg(reg8::A));
//
//    sh.write_reg(reg8::B, 0x2);
//    EXPECT_EQ(0x2, sh.xchg_reg(reg8::B, 0xBB));
//    EXPECT_EQ(0xBB, sh.read_reg(reg8::B));
//
//    sh.write_reg(reg8::C, 0x3);
//    EXPECT_EQ(0x3, sh.xchg_reg(reg8::C, 0xCC));
//    EXPECT_EQ(0xCC, sh.read_reg(reg8::C));
//
//    sh.write_reg(reg8::D, 0x4);
//    EXPECT_EQ(0x4, sh.xchg_reg(reg8::D, 0xDD));
//    EXPECT_EQ(0xDD, sh.read_reg(reg8::D));
//
//    sh.write_reg(reg8::E, 0x5);
//    EXPECT_EQ(0x5, sh.xchg_reg(reg8::E, 0xEE));
//    EXPECT_EQ(0xEE, sh.read_reg(reg8::E));
//
//    sh.write_reg(reg8::F, 0x6);
//    EXPECT_EQ(0x6, sh.xchg_reg(reg8::F, 0xFF));
//    EXPECT_EQ(0xFF, sh.read_reg(reg8::F));
//
//    sh.write_reg(reg8::F, 0x6);
//    EXPECT_EQ(0x6, sh.xchg_reg(reg8::F, 0xFF));
//    EXPECT_EQ(0xFF, sh.read_reg(reg8::F));
//
//    sh.write_reg(reg8::H, 0x7);
//    EXPECT_EQ(0x7, sh.xchg_reg(reg8::H, 0x11));
//    EXPECT_EQ(0x11, sh.read_reg(reg8::H));
//
//    sh.write_reg(reg8::L, 0x8);
//    EXPECT_EQ(0x8, sh.xchg_reg(reg8::L, 0x22));
//    EXPECT_EQ(0x22, sh.read_reg(reg8::L));
//    
//    //
//    // 16 bit registers
//    //
//    sh.write_reg(reg16::AF, 0xBEEF);
//    EXPECT_EQ(0xBEEF, sh.xchg_reg(reg16::AF, 0xFEED));
//    EXPECT_EQ(0xFEED, sh.read_reg(reg16::AF));
//
//    sh.write_reg(reg16::BC, 0xBEEF);
//    EXPECT_EQ(0xBEEF, sh.xchg_reg(reg16::BC, 0xFEED));
//    EXPECT_EQ(0xFEED, sh.read_reg(reg16::BC));
//
//    sh.write_reg(reg16::DE, 0xBEEF);
//    EXPECT_EQ(0xBEEF, sh.xchg_reg(reg16::DE, 0xFEED));
//    EXPECT_EQ(0xFEED, sh.read_reg(reg16::DE));
//
//
//    sh.write_reg(reg16::HL, 0xBEEF);
//    EXPECT_EQ(0xBEEF, sh.xchg_reg(reg16::HL, 0xFEED));
//    EXPECT_EQ(0xFEED, sh.read_reg(reg16::HL));
//
//    sh.write_reg(reg16::SP, 0xBEEF);
//    EXPECT_EQ(0xBEEF, sh.xchg_reg(reg16::SP, 0xFEED));
//    EXPECT_EQ(0xFEED, sh.read_reg(reg16::SP));
//
//    sh.write_reg(reg16::PC, 0xBEEF);
//    EXPECT_EQ(0xBEEF, sh.xchg_reg(reg16::PC, 0xFEED));
//    EXPECT_EQ(0xFEED, sh.read_reg(reg16::PC));
//}
//
//TEST(CPU, RegisterIncrTest)
//{
//    using namespace CPU;
//    SharpLR35902 sh;
//    //W->X->R
//    sh.write_reg(reg8::A, 0);
//    EXPECT_EQ(1, sh.incr_reg(reg8::A, 1));
//
//    sh.write_reg(reg8::B, 0);
//    EXPECT_EQ(1, sh.incr_reg(reg8::B, 1));
//
//    sh.write_reg(reg8::C, 0);
//    EXPECT_EQ(1, sh.incr_reg(reg8::C, 1));
//
//    sh.write_reg(reg8::A, 0);
//    EXPECT_EQ(1, sh.incr_reg(reg8::A, 1));
//
//    sh.write_reg(reg8::A, 0);
//    EXPECT_EQ(1, sh.incr_reg(reg8::A, 1));
//
//    sh.write_reg(reg8::H, 0);
//    EXPECT_EQ(1, sh.incr_reg(reg8::H, 1));
//
//    sh.write_reg(reg8::L, 0);
//    EXPECT_EQ(1, sh.incr_reg(reg8::L, 1));
//
//    sh.write_reg(reg16::SP, 0);
//    EXPECT_EQ(1, sh.incr_reg(reg16::SP, 1));
//
//    sh.write_reg(reg16::PC, 0);
//    EXPECT_EQ(1, sh.incr_reg(reg16::PC, 1));
//}


int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
    std::getchar(); // keep console window open until Return keystroke
}

