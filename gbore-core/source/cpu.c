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
#include "gamebore.h"


#include "cpu_dsl.h"

void gb_CPU_initialize(void)/*Initialize CPU Registers*/
{
    switch (g_GB.gb_model) { //different init of AF register depending on device model
    case gb_dev_Uninitialized:
        //memory has to be initialized first
        assert(false);
        break;
    case gb_dev_DMG:
    case gb_dev_SGB:  AF = LOAD16(0x0001); break;
    case gb_dev_GBP:  AF = LOAD16(0x00FF); break;
    case gb_dev_GBC:  AF = LOAD16(0x0011); break;
    }
    F  = LOAD8(0x00B8);
    BC = LOAD16(0x0013);
    DE = LOAD16(0x00D8);
    HL = LOAD16(0x014D);
    SP = 0xFFFE;
    PC = 0x0100;
}


byte_t gb_CPU_step(void)
{
    byte_t    cycles;
    gb_word_t   opcode = LOAD8(PC);
    gb_dword_t  d16    = LOAD16(PC + 1);
    
    gbdbg_CPU_dumpregs(g_GB.dbg);
    gbdbg_CPU_trace(g_GB.dbg, opcode, d16, PC);

    assert(g_GB.ops[opcode].handler);
    PC += g_GB.ops[opcode].size;
    cycles = g_GB.ops[opcode].handler(opcode, d16);
    F &= 0xF0; //Always clear LS nibble of F register
    return cycles;
}

byte_t gb_CPU_interrupt(gb_addr_t vector)
{
    StopIf(vector < 0x0040 || vector > 0x0060, abort(), "Invalid interrupt vector 0x%x!", vector);

    gdbg_trace(g_GB.dbg, "!!!INTERRUPT!!! @0x%hX\n", vector);
    SP -= 2;         //Push address of next instruction onto stack and thenjump to address nn.
    STOR16(SP, PC); //PC is already moved 
    PC = vector;
    return 12;
}

////////////////////////////////////////////////
//GMB 8bit - Loadcommands
////////////////////////////////////////////////

byte_t gb_LD_D_D(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16); 
    REG8_WRITE(Y(op), REG8_READ(Z(op)));
    return (Y(op) == HL_INDIRECT || Z(op) == HL_INDIRECT) ? (12) : (4);
}

byte_t gb_LD_D_N(gb_opcode_t op, uint16_t d16) {
    REG8_WRITE(Y(op), (d16 & 0xFF));
    return Y(op) == HL_INDIRECT ? (12) : (8);
}

byte_t gb_LD_INDIRECT_NN_A(gb_opcode_t op, uint16_t d16) {
    UNUSED(op);
    STOR8(d16, A); return 16;
}

byte_t gb_LD_A_INDIRECT_NN(gb_opcode_t op, uint16_t d16) {
    UNUSED(op); 
    A = LOAD8(d16); return 16;
}

byte_t gb_LD_A_INDIRECT_R(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    A = LOAD8(*g_regmap_R[P(op)]); return 8;
}

byte_t gb_LD_INDIRECT_R_A(gb_opcode_t op, uint16_t d16) {
    assert(P(op) == (P(op) & 0x1)); //only bit 4 can be set (registers BC, DE)
    UNUSED(d16);
    STOR8(*g_regmap_R[P(op)], A); return 8;
}

byte_t gb_LDH_A_N(gb_opcode_t op, uint16_t d16) { //-----------Read from IO port
    UNUSED(op);
    A = LOAD8(0xFF00 + (d16 & 0xFF)); return 12;
}

byte_t gb_LDH_N_A(gb_opcode_t op, uint16_t d16) { //------------Write to IO port
    UNUSED(op);
    STOR8(0xFF00 + (d16 & 0xFF), A); return 12;
}

byte_t gb_LD_A_INDIRECT_C(gb_opcode_t op, uint16_t d16) { //-----------Read from IO port
    UNUSED(op, d16);
    A = LOAD8(0xFF00 + C); return 8;
}

byte_t gb_LD_INDIRECT_C_A(gb_opcode_t op, uint16_t d16) { //------------Write to IO port
    UNUSED(op, d16);
    STOR8(0xFF00 + C, A); return 8;
}

//byte_t gb_LDH_A_NN(gb_opcode_t op, uint16_t d16) { //-----------XXX
//    UNUSED(op);
//    A = LOAD8(0xFF00 + (d16 & 0xFF)); return 8;
//}

//byte_t gb_LDH_NN_A(gb_opcode_t op, uint16_t d16) { //------------XXX
//    UNUSED(op);
//    STORE8(0xFF00 + (d16 & 0xFF)) = A; return 8;
//}

byte_t gb_LDI_INDIRECT_HL_A(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    STOR8(HL++, A); return 8;
}

byte_t gb_LDI_A_INDIRECT_HL(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    A = LOAD8(HL++); return 8;
}

byte_t gb_LDD_INDIRECT_HL_A(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    STOR8(HL--, A); return 8;
}

byte_t gb_LDD_A_INDIRECT_HL(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    A = LOAD8(HL--); return 8;
}

////////////////////////////////////////////////
//GMB 16bit - Loadcommands
////////////////////////////////////////////////

byte_t gb_PUSH_R(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16); 
    SP -= 2; 
    STOR16(SP, (*g_regmap_R2[P(op)])); return 16;
}

byte_t gb_POP_R(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    (*g_regmap_R2[P(op)]) = LOAD16(SP);
    SP += 2; return 12;
}

byte_t gb_LD_R_NN(gb_opcode_t op, uint16_t d16) {
    *g_regmap_R[P(op)] = d16; return 12;
}

byte_t gb_LD_INDIRECT_NN_SP(gb_opcode_t op, uint16_t d16) {
    UNUSED(op);
    STOR16(d16, SP); return 20; //pandoc says 12?
}

byte_t gb_LD_SP_HL(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    SP = HL;  return 8;
}
////////////////////////////////////////////////
//GMB 8bit - ALU
////////////////////////////////////////////////

byte_t gb_INC_D(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    byte_t v = REG8_READ(Y(op));
    REG8_WRITE(Y(op), v + 1);

    FLAGS(.ZR = (v == 0xFF),  
          .NG = 0, 
          .HC = HC_ADD(v, 1)); 

    return (Y(op) == HL_INDIRECT) ? 12 : 4;
}

byte_t gb_DEC_D(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    byte_t v = REG8_READ(Y(op));
    REG8_WRITE(Y(op), v - 1);
    FLAGS(.ZR = (v == 0x1),
          .NG = 1,
          .HC = HC_SUB(v, 1));

    return (Y(op) == HL_INDIRECT) ? 12 : 4;
} 

byte_t gb_ALU_A_N(gb_opcode_t op, uint16_t d16) {
    gb_alu_mode_e alu = Y(op);
    gb_word_t  n      = (gb_word_t)d16 & 0xFF;
    gb_word_t  r      = 0;
    gb_word_t  c      = GET_C();

    switch (alu) {
    //   ALU MODE   |execute                  |Z            |N        |H         |C
    case gb_ALU_AND: r = A & n;      FLAGS_ALL(!r, 0, 1, 0);  break;
    case gb_ALU_XOR: r = A ^ n;      FLAGS_ALL(!r, 0, 0, 0);  break;
    case gb_ALU_OR:  r = A | n;      FLAGS_ALL(!r, 0, 0, 0);  break;
    case gb_ALU_ADD: r = A + n;      FLAGS_ALL(!r, 0, HC_ADD(A, n), (r < A));break;
    case gb_ALU_CP:                 // fallthrough;
    case gb_ALU_SUB: r = A - n;      FLAGS_ALL(!r, 1, HC_SUB(A, n), (n > A));break;
    case gb_ALU_SBC: r = A - n - c;  FLAGS_ALL(!r, 1, HC_SBC(A, n, c), (n+c > A)); break;
    case gb_ALU_ADC: r = A + n + c;  FLAGS_ALL(!r, 0, HC_ADC(A, n, c), (A+n+c > r)); break;
    }
    //for ALU CP - A remains unchanged: 
    A = (gb_ALU_CP == alu) ? A : r;
    
    return 4; //wrong i think 8?
}

byte_t gb_ALU_A_D(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    gb_ALU_A_N(op, REG8_READ(Z(op))); //wrap gb_ALU_A_N
    return (Z(op) == HL_INDIRECT) ? 8 : 4;
}

byte_t gb_DAA(gb_opcode_t op, uint16_t d16) { //Decimal Adjust register A
    UNUSED(op, d16);
    // Note:
    // DAA inspects flags and will only work properly if invoked after ADD or SUB.
    // And not after e.g. LOAD A,x
    /*
        --------------------------------------------------------------------------------
        |           | C Flag  | HEX value in | H Flag | HEX value in | Number  | C flag|
        | Operation | Before  | upper digit  | Before | lower digit  | added   | After |
        |           | DAA     | (bit 7-4)    | DAA    | (bit 3-0)    | to byte | DAA   |
        |------------------------------------------------------------------------------|
        |           |    0    |     0-9      |   0    |     0-9      |   00    |   0   |
        |   ADD     |    0    |     0-8      |   0    |     A-F      |   06    |   0   |
        |           |    0    |     0-9      |   1    |     0-3      |   06    |   0   |
        |   ADC     |    0    |     A-F      |   0    |     0-9      |   60    |   1   |
        |           |    0    |     9-F      |   0    |     A-F      |   66    |   1   |
        |   INC     |    0    |     A-F      |   1    |     0-3      |   66    |   1   |
        |           |    1    |     0-2      |   0    |     0-9      |   60    |   1   |
        |           |    1    |     0-2      |   0    |     A-F      |   66    |   1   |
        |           |    1    |     0-3      |   1    |     0-3      |   66    |   1   |
        |------------------------------------------------------------------------------|
        |   SUB     |    0    |     0-9      |   0    |     0-9      |   00    |   0   |
        |   SBC     |    0    |     0-8      |   1    |     6-F      |   FA    |   0   |
        |   DEC     |    1    |     7-F      |   0    |     0-9      |   A0    |   1   |
        |   NEG     |    1    |     6-F      |   1    |     6-F      |   9A    |   1   |
        |------------------------------------------------------------------------------|
        Unfortunately implementation had to be made uglier because of undefined
        behavior which was not emulated properly (i.e. values out of the scope in table above).
    */
    gb_word_t const v = A;
    gb_word_t addend = 0;
    bool carry = false;
    if (GET_N()) { //DAA invoked after SUB or SBC

        addend  = GET_H() ? 0xFA : 0x00; //aka -0x06
        addend += GET_C() ? 0xA0 : 0x00; //aka -0x60
        carry   = GET_C();

    } else { //DAA invoked after ADD or ADC

        addend  = ((v & 0x0F)  > 0x09  || GET_H()) ? 0x06 : 0x00; // +0x06
        addend += ((v + addend) > 0x9F || GET_C()) ? 0x60 : 0x00; // +0x60

        carry = GET_C() || 0x100 == (0x100 & ((uint16_t)v + (uint16_t)addend));
    }
    A = v + addend;

    FLAGS(.ZR = !A,
          .HC = 0,
          .CR = carry);
    return 4;
}

byte_t gb_CPL(gb_opcode_t op, uint16_t d16) { //Complement A
    UNUSED(op, d16);
    A = ~A;
    FLAGS(.NG = 1, .HC = 1); return 4;
}

////////////////////////////////////////////////
//GMB Singlebit Operation Commands
//GMB Rotate & Shift
////////////////////////////////////////////////

byte_t gb_ROT_A(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    gb_word_t const v    = A;
    gb_word_t const bit4 = BIT_GET_N(op, 4); // bit4: 0-RdCA 1-RdA
    uint8_t  rotated_bit;
    if(Q(op) == DIR_LEFT){ // bit3: direction

        if (!bit4) A = BIT_RL_8(v);         // RLCA
        else       A = BIT_RLCARRY_8(v, F); // RLA

        rotated_bit = 7;
    }else{ // DIR_RIGHT
        if (!bit4) A = BIT_RR_8(v);         // RRCA
        else       A = BIT_RRCARRY_8(v, F); // RRA

        rotated_bit = 0;
    }
    FLAGS_ALL(0, 0, 0, BIT_GET_N(v, rotated_bit));
    return 4;
}

////////////////////////////////////////////////
// All 256 0xCB prefixed opcodes implementation:
////////////////////////////////////////////////

byte_t gb_PREFIX_CB(gb_opcode_t op, uint16_t d16) {
    //
    // Actual op-code is the second word after 0xCB, override to avoid typos:
    //
    assert(op == 0xCB);
    op = d16 & 0x00FF;   

    bool      const rot_left    = Q(op) == DIR_LEFT;
    uint8_t   const rotated_bit = rot_left ? 7 : 0;  // Bit N to preserve in Carry
    gb_word_t const n           = Y(op);
    gb_word_t const v = REG8_READ(Z(op)); // Current register value
    gb_word_t       r = v;                // Scratch for result
    switch (X(op)) {
    case 0b00:
        switch (P(op)) {

        case 0b00: // RLC D, RRC D  (Rotate)

            if (rot_left) r = BIT_RL_8(v);
            else          r = BIT_RR_8(v); 
            FLAGS_ALL(!r, 0, 0, BIT_GET_N(v, rotated_bit));
            break;

        case 0b01: // RL D, RR D (Rotate thru Carry)

            if (rot_left) r = BIT_RLCARRY_8(v, F);
            else          r = BIT_RRCARRY_8(v, F);
            FLAGS_ALL(!r, 0, 0, BIT_GET_N(v, rotated_bit));
            break;

        case 0b10: // SLA D, SRA D (Shift R/L into carry)

            if (rot_left) {
                r = v << 1;              //LSB := 0 
                FLAGS_ALL(!r, 0, 0, BIT_GET_N(v, 7));
            }
            else {
                r = (v >> 1) | (v & 0x80); //MSB := intact 
                FLAGS_ALL(!r, 0, 0, BIT_GET_N(v, 0));
            }
            break;
        case 0b11: // SWAP D, SRL D (Swap nibbles of D, Shift R into carry)

            if (Q(op) == 0) {  //-> SWAP
                r = ((v << 0x4) & 0xF0) | (v >> 0x4); 
                FLAGS_ALL(!r, 0, 0, 0);
            }
            else {  
                r = v >> 1;  //MSB := 0 
                FLAGS_ALL(!r, 0, 0, BIT_GET_N(v, 0));
            }
            break;
        }
        break;

    case 0b01:  //BIT N, D -- just set the Zero flag to bit value
        FLAGS(.ZR = !BIT_GET_N(v, n), 
              .NG = 0, 
              .HC = 1);
        break;

    case 0b10: // RES N,D -- Reset bit N in register D
        r = BIT_RES_N(v, n);
        break;

    case 0b11: //SET N,D -- Set bit N in register D
        r = BIT_SET_N(v, n); 
        break;
    }
    REG8_WRITE(Z(op), r); // Save result to destination register
    return (Z(op) == HL_INDIRECT) ? 16 : 8;
}

////////////////////////////////////////////////
//GMB 16bit - ALU
////////////////////////////////////////////////

byte_t gb_INC_R(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    (*g_regmap_R[P(op)])++; return 8;
}

byte_t gb_DEC_R(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    (*g_regmap_R[P(op)])--; return 8;
}

byte_t gb_ADD_HL_R(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    gb_dword_t const v = (*g_regmap_R[P(op)]);
    gb_dword_t const r = HL + v;
    FLAGS(.NG=0, 
          .HC=HC_ADD16(HL, v), 
          .CR=(r < HL));
    HL = r; return 8;
}

byte_t gb_ADD_SP_N(gb_opcode_t op, uint16_t d16) {
    UNUSED(op);
    uint8_t    const u = (uint8_t)d16;
    int8_t     const n = (int8_t)d16; //signed 8bit immediate
    gb_dword_t const r = SP + n;
    // C & H flags are calculated like when adding 8 bit values:
    // add immediate byte (as unsigned) to SP's lower byte.
    FLAGS(.ZR = 0,
          .NG = 0,
          .HC = HC_ADD(SP, u),
          .CR = ((SP + u) & 0xFF) < (SP & 0xFF));
    SP = r; return 16;
}

byte_t gb_LD_HL_SP_N(gb_opcode_t op, uint16_t d16) {
    UNUSED(op);
    uint8_t    const u = (uint8_t)d16; 
    int8_t     const n = (int8_t)d16; //signed 8bit
    gb_dword_t const r = SP + n;
    // C & H flags are calculated like when adding 8 bit values:
    // add immediate byte (as unsigned) to SP's lower byte.
    FLAGS(.ZR = 0,
          .NG = 0,
          .HC = HC_ADD(SP, u),
          .CR = ((SP+u) & 0xFF) < (SP & 0xFF));
    HL = r; return 12;
}

////////////////////////////////////////////////
// Jump Commands / Flow Control
////////////////////////////////////////////////
byte_t gb_JP_N(gb_opcode_t op, uint16_t d16) {
    UNUSED(op);
    PC = d16; return 16;
}

byte_t gb_JR_N(gb_opcode_t op, uint16_t d16) {
    UNUSED(op);
    int8_t n = (int8_t)d16; //signed 8bit
    PC += n; return 12;
}

byte_t gb_JP_INDIRECT_HL(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    PC = HL; return 4;
}

//
// Helper functions for conditional jumps, calls returns
//
__forceinline
bool gb_jmpcondition_check(gb_opcode_t op) {
    bool make_jump = false;

    switch (BIT_RANGE_GET(op, 3, 4)) {
    case gb_jump_NZ:  if (!GET_Z()) make_jump = true; break;
    case gb_jump_Z:   if (GET_Z())  make_jump = true; break;
    case gb_jump_NC:  if (!GET_C()) make_jump = true; break;
    case gb_jump_C:   if (GET_C())  make_jump = true; break;
    default:          assert(0);
    }
    return make_jump;
}

byte_t gb_JR_F_N(gb_opcode_t op, uint16_t d16) {
    if (gb_jmpcondition_check(op)) {
        int8_t n = (int8_t)d16; //signed 8bit
        PC += n; 
        return 16;
    } else return 12;
}

byte_t gb_JP_F_NN(gb_opcode_t op, uint16_t d16) {
    if (gb_jmpcondition_check(op)) {
        PC = d16; return 16;
    }else return 12;
}

byte_t gb_CALL_F_N(gb_opcode_t op, uint16_t d16) {
    if (gb_jmpcondition_check(op)) {
        SP -= 2;
        STOR16(SP, PC); //PC is already moved 
        PC = d16; return 24;
    }else return 12;
}

byte_t gb_CALL_N(gb_opcode_t op, uint16_t d16) {
    UNUSED(op); 
    SP -= 2;         //Push address of next instruction onto stack and thenjump to address nn.
    STOR16(SP, PC); //PC is already moved 
    PC = d16; return 12;
} 

byte_t gb_RET(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    PC = LOAD16(SP); //PC is already moved 
    SP += 2; 
    
    if (BIT_GET_N(op, 4)) { // RETI instruction 
        g_GB.interrupts.IME = true;
    }
    return 16;
}

byte_t gb_RET_F(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    if (gb_jmpcondition_check(op)) {
        PC = LOAD16(SP); //PC is already moved 
        SP += 2;
        return 20;
    }
    return 8;
}

byte_t gb_RST(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    byte_t const N = Y(op) << 3;
    SP -= 2;
    STOR16(SP, PC);
    PC = N; return 16; //other sources say 32..
}

////////////////////////////////////////////////
// CPU Control Commands
////////////////////////////////////////////////

byte_t gb_CCF(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    FLAGS(.NG = 0, .HC = 0, .CR = GET_C()?0:1); return 4;
}

byte_t gb_SCF(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    FLAGS(.NG = 0, .HC = 0, .CR = 1); return 4;
}

byte_t gb_NOP(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16); 
    return 4;
}

byte_t gb_DI(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    g_GB.interrupts.IME = false;
    /*  This instruction disables interrupts but not
    immediately.Interrupts are disabled when 
    next instruction after DI is executed. */
    return 4;
}

byte_t gb_EI(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    g_GB.interrupts.IME = true;
    gb_INTERRUPT_request(0);
    return 4;
}

byte_t gb_HALT(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16, op);
    /*
    This is a very special operation, it halts the CPU until next interrupt fires.

    Upon first invocation of HALT opcode, two flags are set to TRUE:
        * `g_GB.interrupts.HALT`
        * `g_GB.interrupts.HALT_is_waiting_for_ISR`
    
    The program counter will stay at the HALT instruction (see HALT's OP_DEF)
    until the flag `HALT_is_waiting_for_ISR` is cleared by any interrupt request,
    regardless if IME is enabled or not.
    */
    if (false == g_GB.interrupts.HALT) {
        // We enter halted state:
        g_GB.interrupts.HALT = true;
        g_GB.interrupts.HALT_is_waiting_for_ISR = true;

    } else {
        // We are in HALTED state, the Program Counter will not move
        // until an ISR is executed.
        if (false == g_GB.interrupts.HALT_is_waiting_for_ISR) {

            // ISR was invoked, we can resume the CPU:
            PC += 1;
            g_GB.interrupts.HALT = false;;
        }
    }
    return 4;
}

byte_t gb_STOP(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    assert((d16 & 0xFF) == 0x00);

    /*
    Draft implementation - seems to be broken!
    */
    if (false == g_GB.interrupts.STOP) {
        // We enter halted state:
        g_GB.interrupts.STOP = true;
        g_GB.interrupts.STOP_is_waiting_for_JOYP = true;

    } else {
        // We are in HALTED state, the Program Counter will not move
        // until an ISR is executed.
        if (false == g_GB.interrupts.STOP_is_waiting_for_JOYP) {

            // ISR was invoked, we can resume the CPU:
            PC += 2;
            g_GB.interrupts.STOP = false;;
        }
    }
    //PC += 2;
    return 4;
}
