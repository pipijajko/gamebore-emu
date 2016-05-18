#pragma once

#define OP_SIGMATCH(sig, opcode) \
    ((sig.n > 0)&&(((opcode) & sig.r) == (sig.m)))


gb_instruction_bits_s
gb_parse_opcode_pattern(
    const char  *op_pattrn,
    const size_t op_pattrn_len
);


void
gb_populate_opcode_slots(
    gb_instruction_bits_s     op_bits,
    gb_opcode_handler_pfn     handler,
    byte_t               op_size,
    struct gb_instruction_s slots[],
    const size_t         slots_n
);

void
gb_build_decoder_table(
    struct gb_instruction_s slots[],
    const size_t slots_n
);


