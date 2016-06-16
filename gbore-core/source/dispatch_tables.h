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


