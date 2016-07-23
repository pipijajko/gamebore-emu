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
#include <assert.h>
#include <stdio.h>
#include "types.h"
#include "dispatch_tables.h"

#define GBDEBUG


//
// Some include-programming (XMACROS) follows.
// Close your eyes and scroll down.
//


// Create function declarations
#define OP_DEF(pattern, bytes, opname)  byte_t gb_##opname (gb_opcode_t, uint16_t);
#include "opcodes.inc"
#undef OP_DEF



// Fill dispatch table

void
gb_build_decoder_table(struct gb_instruction_s slots[], const size_t slots_n) {

    #define OP_DEF(sigpattern, bytes, opname) \
        gb_populate_opcode_slots(\
            gb_parse_opcode_pattern(sigpattern, sizeof(sigpattern)),\
            gb_##opname,\
            bytes,\
            slots, \
            slots_n);      

    #include "opcodes.inc"
    #undef  OP_DEF

}


//#define OP_DEF(NAME, MASK) void op_##NAME(gb_opcode_t, uint16_t)


gb_instruction_bits_s
gb_parse_opcode_pattern(
    const char  *op_pattrn,
    const size_t op_pattrn_len
) {
    int        bit_n = sizeof(gb_word_t) * 8 - 1; //start with most significant bit of machine word
    gb_word_t  rel_bits = 0;
    gb_word_t  match = 0;
    gb_word_t  permutation_n = 1;
    gb_instruction_bits_s sig = { 0 };
    size_t    i;

    if (!op_pattrn || op_pattrn_len == 0) goto cleanup_generic;

    for (i = 0; i < op_pattrn_len && bit_n >= 0; i++) {

        switch (op_pattrn[i]) {
        case '0':
            rel_bits |= (1 << bit_n);
            bit_n--;
            break;

        case '1':
            rel_bits |= (1 << bit_n);
            match |= (1 << bit_n);
            bit_n--;
            break;

        case '-':
            rel_bits |= (0 << bit_n); //just for readability
            permutation_n <<= 1;
            bit_n--;
            break;

        default:
            break;
        }
    }

    assert((rel_bits & match) == match);
    assert(permutation_n <= 0xFF);

    if (bit_n < 0 && rel_bits) {
        sig.m = match;
        sig.r = rel_bits;
        sig.n = permutation_n;
    }

cleanup_generic:
    return sig;
}



void
gb_populate_opcode_slots(
    gb_instruction_bits_s         op_bits,
    gb_opcode_handler_pfn         handler,
    byte_t                   op_size,
    struct gb_instruction_s slots[],
    const size_t             slots_n
){
    size_t   i;
    int matching_n = op_bits.n;

    if (matching_n == 1) {

        assert(op_bits.m < slots_n);
        assert(!slots[op_bits.m].handler);
        assert(!slots[op_bits.m].size);

        slots[op_bits.m].handler = handler;
        slots[op_bits.m].size = op_size;

    }
    else if (matching_n > 0) {

        for (i = 0; i < slots_n; i++) {

            if (OP_SIGMATCH(op_bits, i)) {

                assert(!slots[i].handler );
                assert(!slots[i].size);


                slots[i].handler = handler;
                slots[i].size    = op_size;
                if (--matching_n == 0) break; //no more 
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// for debug only
/////////////////////////////////////////////////////////////////////////////



#ifdef GBDEBUG
#define OP_DEF(pattern, bytes, opname) \
    byte_t gbdbg_##opname(gb_opcode_t o, uint16_t imm, void* dbg){\
        gb_bytebuf_t* bb= (gb_bytebuf_t*)dbg;   \
        int b = bytes;          \
        if(b == 1)       sprintf_s((char*)bb->buffer, bb->size,"----\t(" pattern ")->0x%x "#opname"\n",o);\
        else if(b == 2)  sprintf_s((char*)bb->buffer, bb->size,"0x%02x\t(" pattern ")->0x%x "#opname"\n", imm&0xFF, o);\
        else if (b == 3) sprintf_s((char*)bb->buffer, bb->size,"0x%04x\t(" pattern ")->0x%x "#opname"\n", imm, 0);\
        return 0;\
    }
#include "opcodes.inc"
#undef OP_DEF

void
gbdbg_build_decoder_table(struct gb_instruction_s slots[], const size_t slots_n) {
    // WARNING: HAX
    // Functions given here to populate_opcode_slots actually don't have signature gb_opcode_handler_pfn!
    // They have a debug signature
    
#define OP_DEF(sigpattern, bytes, opname) \
        gb_populate_opcode_slots(\
            gb_parse_opcode_pattern(sigpattern, sizeof(sigpattern)),\
            (gb_opcode_handler_pfn)gbdbg_##opname,\
            bytes,\
            slots,\
            slots_n);      

#include "opcodes.inc"
#undef  OP_DEF
}

#endif