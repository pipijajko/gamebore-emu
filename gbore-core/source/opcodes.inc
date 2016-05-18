

//
// Opcode pattern syntax:
// 
// OP_DEF(matching_pattern, instruction_bytes, instruction_name)

// `matching_pattern` describes which bits need to be set/cleared for a given instruction type.
// When parsing an opcode, if '1's and '0's in the `matching_pattern` match,
// ('-' can be either '1' or '0') then the handler for `instruction_name` will be invoked.


OP_DEF("00 001 000", 3, LD_INDIRECT_NN_SP)
OP_DEF("00 --0 001", 3, LD_R_NN)
OP_DEF("01 --- ---", 1, LD_D_D)
OP_DEF("00 --- 110", 2, LD_D_N)
OP_DEF("00 0-0 010", 1, LD_INDIRECT_R_A)
OP_DEF("00 0-1 010", 1, LD_A_INDIRECT_R)
OP_DEF("00 --0 011", 1, INC_R)
OP_DEF("00 --1 011", 1, DEC_R)
OP_DEF("00 --1 001", 1, ADD_HL_R)


OP_DEF("11 101 010", 3, LD_INDIRECT_NN_A)
OP_DEF("11 111 010", 3, LD_A_INDIRECT_NN)
OP_DEF("11 100 000", 2, LDH_N_A)
OP_DEF("11 110 000", 2, LDH_A_N)

OP_DEF("11 100 010", 1, LD_INDIRECT_C_A)
OP_DEF("11 110 010", 1, LD_A_INDIRECT_C)

OP_DEF("00 100 010", 1, LDI_INDIRECT_HL_A)
OP_DEF("00 101 010", 1, LDI_A_INDIRECT_HL)
OP_DEF("00 110 010", 1, LDD_INDIRECT_HL_A)
OP_DEF("00 111 010", 1, LDD_A_INDIRECT_HL)

OP_DEF("00 --- 100", 1, INC_D)
OP_DEF("00 --- 101", 1, DEC_D)
OP_DEF("10 --- ---", 1, ALU_A_D)
OP_DEF("11 --- 110", 2, ALU_A_N)
OP_DEF("00 101 111", 1, CPL)

OP_DEF("00 0-- 111", 1, ROT_A)

OP_DEF("11 --0 001", 1, POP_R)
OP_DEF("11 --0 101", 1, PUSH_R)
OP_DEF("00 1-- 000", 2, JR_F_N)
OP_DEF("00 011 000", 2, JR_N)

OP_DEF("11 0-- 010",3, JP_F_NN)
OP_DEF("11 000 011", 3, JP_N)
OP_DEF("11 101 001", 1, JP_INDIRECT_HL)
OP_DEF("11 001 101", 3, CALL_N)
OP_DEF("11 0-- 100", 3, CALL_F_N)
OP_DEF("11 0-1 001", 1, RET)
OP_DEF("11 0-- 000", 1, RET_F)
OP_DEF("11 --- 111", 1, RST)


OP_DEF("00 000 000", 1, NOP)
OP_DEF("11 110 011", 1, DI)
OP_DEF("11 111 011", 1, EI)


OP_DEF("11 001 011", 2, PREFIX_CB)

// For two-bit ops there are several ways to continue:
// Create one big-ass handler for all the ops, which is not such a bad idea beacause all are bit-operations;
// Create possiblity of nesting dispatch tables, eg. by some flag/or null handler. 
//		Problem is how to do this without wasting too much memory or allocating lots of small crap
// I think, that since there's only one kw like that, it's not wise to write whole new dispatching code + function calls for set of very similar funcitons 