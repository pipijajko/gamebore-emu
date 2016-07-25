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
#include <stdlib.h>
#include "types.h"
#include "dispatch_tables.h"
#include "helpers.h"

#define GBDEBUG


//
// Some include-programming (XMACROS) follows.
// Close your eyes and scroll down.
//


// Create function declarations
#define OP_DEF(pattern, bytes, opname)  byte_t gb_##opname (gb_opcode_t, uint16_t);
#define OP_OVR OP_DEF
#include "opcodes.inc"
#undef OP_OVR
#undef OP_DEF


// Fill dispatch table
#define OPCODE_POPULATE(sigpattern, bytes, opname, is_override) \
            gb_populate_opcode_slots(\
            gb_parse_opcode_pattern(sigpattern, sizeof(sigpattern)),\
            gb_##opname,\
            bytes,\
            slots, \
            slots_n, \
            is_override);


void 
gb_build_decoder_table(struct gb_instruction_s slots[], const size_t slots_n) {
    // Generate `gb_populate_opcode_slots()` call for each OP_DEF macro
    #define OP_DEF(sigpattern, bytes, opname) \
            OPCODE_POPULATE(sigpattern, bytes, opname, false)
    #define OP_OVR(...)
    #include "opcodes.inc"
    #undef  OP_OVR
    #undef  OP_DEF

    // Generate `gb_populate_opcode_slots()` call for each OP_OVR macro
    #define OP_OVR(sigpattern, bytes, opname) \
                OPCODE_POPULATE(sigpattern, bytes, opname, true)
    #define OP_DEF(...)
    #include "opcodes.inc"
    #undef  OP_DEF
    #undef  OP_OVR

}


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



void gb_check_handler(
    gb_instruction_s const slot, 
    gb_word_t const opcode, 
    bool const is_override
){
    if (is_override) {
        StopIf(!slot.handler, abort(),
               "Instruction 0x%hhx has no handler - nothing to override!",
               opcode
        );
        StopIf(!slot.size, abort(),
               "Instruction 0x%hhx has no instruction size set - nothing to override!", 
               opcode
        );
    } else {

        StopIf(slot.handler, abort(),
               "Instruction 0x%hhx already has handler! Your OP_DEFs are incorrect!"
               "Use OP_OVR to explicitly override it.\n",
               opcode
        );
        StopIf(slot.size, abort(),
               "Instruction 0x%hhx already has instruction size set!"
               "Rather unlikely error!", 
               opcode
        );
    }
}


void
gb_populate_opcode_slots(
    gb_instruction_bits_s    op_bits,
    gb_opcode_handler_pfn    handler,
    byte_t                   op_size,
    struct gb_instruction_s  slots[],
    const size_t             slots_n,
    const bool               is_override
){
    size_t   i;
    int matching_n = op_bits.n;

    if (matching_n == 1) {
        assert(op_bits.m < slots_n);
        gb_word_t opcode = op_bits.m;

        gb_check_handler(slots[opcode], opcode, is_override);

        slots[op_bits.m].handler = handler;
        slots[op_bits.m].size = op_size;

    }
    else if (matching_n > 0) {

        for (i = 0; i < slots_n; i++) {

            if (OP_SIGMATCH(op_bits, i)) {

                gb_check_handler(slots[i], (gb_word_t)i, is_override);

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
        else             sprintf_s((char*)bb->buffer, bb->size,"----\t(" pattern ")->0x%x "#opname"\n",o);\
        return 0;\
    }
#define OP_OVR OP_DEF
#include "opcodes.inc"
#undef OP_OVR
#undef OP_DEF


// Fill dispatch table
// WARNING: HAX
// Functions given here to populate_opcode_slots actually don't have signature gb_opcode_handler_pfn!
// They have a debug signature
#define OPDECODER_DBG_POPULATE(sigpattern, bytes, opname, is_override) \
            gb_populate_opcode_slots(\
                gb_parse_opcode_pattern(sigpattern, sizeof(sigpattern)),\
                (gb_opcode_handler_pfn)gbdbg_##opname,\
                bytes,\
                slots, \
                slots_n, \
                is_override);

void
gbdbg_build_decoder_table(struct gb_instruction_s slots[], const size_t slots_n) {

    
#define OP_DEF(sigpattern, bytes, opname) \
        OPDECODER_DBG_POPULATE(sigpattern, bytes, opname, false)
#define OP_OVR(...) 
#include "opcodes.inc"
#undef  OP_OVR
#undef  OP_DEF

#define OP_OVR(sigpattern, bytes, opname) \
        OPDECODER_DBG_POPULATE(sigpattern, bytes, opname, true)
#define OP_DEF(...) 
#include "opcodes.inc"
#undef  OP_DEF
#undef  OP_OVR

}

#endif