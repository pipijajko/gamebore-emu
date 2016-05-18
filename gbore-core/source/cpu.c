
#include "stdafx.h"
#include "gamebore.h"


typedef
enum gb_jump_cond_e {
    gb_jump_NZ = 0b00, //NotZero
    gb_jump_Z  = 0b01, //Zero
    gb_jump_NC = 0b10, //NotCarry
    gb_jump_C  = 0b11, //Carry
} gb_jump_cond_e;

typedef 
enum gb_alu_mode_e {
    gb_ALU_ADD = 0b000,
    gb_ALU_ADC = 0b001,
    gb_ALU_SUB = 0b010,
    gb_ALU_SBC = 0b011,
    gb_ALU_AND = 0b100,
    gb_ALU_XOR = 0b101,
    gb_ALU_OR  = 0b110,
    gb_ALU_CP  = 0b111,
} gb_alu_mode_e;

#define DIR_LEFT  0
#define DIR_RIGHT 1

/*
Z80 Opcode Parsing Macros

X(OP) - 1st octal digit, bits 7-6
Y(OP) - 2nd octal digit, bits 5-3
Z(OP) - 3rd octal digit, bits 2-0
P(OP) - Y(OP) >> 1, i.e. bits 5-4
Q(OP) - Y(OP) % 2, i.e. bit 3
*/

#define X(OP) (((OP) & 0b11000000) >> 6) //X(OP) - 1st octal digit, bits 7-6
#define Y(OP) (((OP) & 0b00111000) >> 3) //Y(OP) - 2nd octal digit, bits 5-3
#define Z(OP) (((OP) & 0b00000111) >> 0) //Z(OP) - 3rd octal digit, bits 2-0
#define P(OP) (((OP) & 0b00110000) >> 4) //P(OP) - Y(OP) >> 1, i.e. bits 5-4
#define Q(OP) (((OP) & 0b00001000) >> 3) //Q(OP) - Y(OP) % 2, i.e. bit 3

/*
Convenience macros
*/

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


#define STOR8(ADDR) (*gb_MMU_store8(&g_GB, (ADDR)))
#define LOAD8(ADDR)  (*gb_MMU_load8(&g_GB, (ADDR)))

#define STOR16(ADDR) (*gb_MMU_store16(&g_GB, (ADDR)))
#define LOAD16(ADDR)  (*gb_MMU_load16(&g_GB, (ADDR)))


//Zero
#define Z_FLAG(cond) SETCLR((cond), F, GB_FLAG_Z
//#define SETCLR_Z(cond) SETCLR((cond), F, GB_FLAG_Z)
//#define SET_Z() ((F) |= GB_FLAG_Z)
//#define CLR_Z() ((F) &= ~GB_FLAG_Z)
#define GET_Z() (((F) & GB_FLAG_Z) == GB_FLAG_Z)

//Substract FLAG
#define N_FLAG(cond) SETCLR((cond), F, GB_FLAG_N)
//#define SETCLR_N(cond) SETCLR((cond), F, GB_FLAG_N)
//#define SET_N()  ((F) |= GB_FLAG_N)
//#define CLR_N()  ((F) &= ~GB_FLAG_N)
#define GET_N() (((F) & GB_FLAG_N) == GB_FLAG_N)

//Half-carry flag

#define H_FLAG(cond) SETCLR((cond), F, GB_FLAG_H)
//#define SETCLR_H(cond) SETCLR((cond), F, GB_FLAG_H)
//#define SET_H() ((F) |= GB_FLAG_H)
//#define CLR_H() ((F) &= ~GB_FLAG_H) 
#define GET_H() (((F) & GB_FLAG_H) == GB_FLAG_H)

//Carry flag
#define C_FLAG(cond) SETCLR((cond), F, GB_FLAG_C)
//#define SETCLR_C(cond) SETCLR((cond), F, GB_FLAG_C)
//#define SET_C() ((F) |= GB_FLAG_C) 
//#define CLR_C() ((F) &= ~GB_FLAG_C) 
#define GET_C() (((F) & GB_FLAG_C) == GB_FLAG_C)



typedef 
struct flagsetter_s {
    int8_t ZR, NG, HC, CR;
}flagsetter_s;


#define FLAG_OMIT ((int8_t)(-127)) //This is kind of hacky :/

void 
flags_setter(flagsetter_s f) {

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
                                


//Half Carry Calculators
//HC_ADD: Set if carry from bit 3
//HC_SUB: Set if no borrow from bit 4.

#define HC_ADD(VAL, ADDEND)     (((((VAL) & 0xF) + ((ADDEND)    & 0xF)) & 0x10) == 0x10)
#define HC_SUB(VAL, SUBTRAHEND)   (((VAL) & 0xF) < ((SUBTRAHEND)& 0xF)) 
#define HC_ADD16(VAL, ADDEND) (((((VAL) & 0xFFF) + ((ADDEND) & 0xFFF)) & 0x1000) == 0x1000)

//
//Register maps
//
static gb_dword_t * const g_regmap_R[]  = { &BC, &DE, &HL, &SP }; //2bit
static gb_dword_t * const g_regmap_R2[] = { &BC, &DE, &HL, &AF }; //2bit'
static gb_word_t  * const g_regmap_D[]  = { &B, &C, &D, &E, &H,&L, NULL, &A }; //3bit
#define HL_INDIRECT 0x6

//
//Selectors for 8-bit registers, in case when `d` is 0x6, we need to load/store to memory.
//
#define REG8_WRITE(d) (*( ((d) != HL_INDIRECT) ? g_regmap_D[(d)] : gb_MMU_store8(&g_GB, HL) ))
#define REG8_READ(d)  (*( ((d) != HL_INDIRECT) ? g_regmap_D[(d)] : gb_MMU_load8(&g_GB, HL)  ))


//
//
// Selectors for 16-bit registers
//
#define RM1 1 // RegMap #1 selects one of: BC,DE,HL,SP registers
#define RM2 2 // RegMap #2 selects one of: BC,DE,HL,AF registers

#define REG16_RW(r, rmap) (*((rmap == RM1) ? g_regmap_R[(r)] : g_regmap_R2[(r)]))

#define REG16_READ  REG16_RW
#define REG16_WRITE REG16_RW



void
gb_CPU_init(void)
{
    /*
    Initiaze CPU Registers
    */

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


    if (PC == 0x2803 || PC == 0x27EB) {
        gbdbg_mute_print(g_GB.dbg, true);
    }
    gbdbg_CPU_dumpregs(g_GB.dbg);
    gbdbg_CPU_trace(g_GB.dbg, opcode, d16, PC);

    assert(g_GB.ops[opcode].handler);

    PC += g_GB.ops[opcode].size;
    cycles = g_GB.ops[opcode].handler(opcode, d16);

    return cycles;
}

byte_t
gb_CPU_interrupt(gb_addr_t vector)
{
    StopIf(vector < 0x0040 || vector > 0x0060, 
           abort(), 
           "Invalid interrupt vector 0x%x!", vector);
    
    gdbg_trace(g_GB.dbg, "!!!INTERRUPT!!! @0x%hX", vector);
    

    SP -= 2;         //Push address of next instruction onto stack and thenjump to address nn.
    STOR16(SP) = PC; //PC is already moved 
    PC = vector;
    return 12;
    
}


////////////////////////////////////////////////
//GMB 8bit - Loadcommands
////////////////////////////////////////////////

byte_t gb_LD_D_D(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16); 
    REG8_WRITE(Y(op)) = REG8_READ(Z(op));
    return (Y(op) == HL_INDIRECT || Z(op) == HL_INDIRECT) ? (12) : (4);
}

byte_t gb_LD_D_N(gb_opcode_t op, uint16_t d16) {
    REG8_WRITE(Y(op)) = (d16 & 0xFF);
    return Y(op) == HL_INDIRECT ? (12) : (8);
}

byte_t gb_LD_INDIRECT_NN_A(gb_opcode_t op, uint16_t d16) {
    UNUSED(op);
    STOR8(d16) = A; return 16;
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
    STOR8(*g_regmap_R[P(op)]) = A; return 8;
}



byte_t gb_LDH_A_N(gb_opcode_t op, uint16_t d16) { //-----------Read from IO port
    UNUSED(op);
    A = LOAD8(0xFF00 + (d16 & 0xFF)); return 12;
}


byte_t gb_LDH_N_A(gb_opcode_t op, uint16_t d16) { //------------Write to IO port
    UNUSED(op);
    STOR8(0xFF00 + (d16 & 0xFF)) = A; return 12;
}


byte_t gb_LD_A_INDIRECT_C(gb_opcode_t op, uint16_t d16) { //-----------Read from IO port
    UNUSED(op, d16);
    A = LOAD8(0xFF00 + C); return 8;
}


byte_t gb_LD_INDIRECT_C_A(gb_opcode_t op, uint16_t d16) { //------------Write to IO port
    UNUSED(op, d16);
    STOR8(0xFF00 + C) = A; return 8;
}


//Not implemented yet:
//byte_t gb_LDH_A_NN(gb_opcode_t op, uint16_t d16) { //-----------Read from IO port
//    UNUSED(op);
//    A = LOAD8(0xFF00 + (d16 & 0xFF)); return 8;
//}


//byte_t gb_LDH_NN_A(gb_opcode_t op, uint16_t d16) { //------------Write to IO port
//    UNUSED(op);
//    STORE8(0xFF00 + (d16 & 0xFF)) = A; return 8;
//}

byte_t gb_LDI_INDIRECT_HL_A(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    STOR8(HL++) = A; return 8;
}

byte_t gb_LDI_A_INDIRECT_HL(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    A = LOAD8(HL++); return 8;
}

byte_t gb_LDD_INDIRECT_HL_A(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    STOR8(HL--) = A; return 8;
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
    STOR16(SP) = (*g_regmap_R2[P(op)]);
    return 16;
}

byte_t gb_POP_R(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    (*g_regmap_R2[P(op)]) = LOAD16(SP);
    SP += 2;
    return 12;
}

byte_t gb_LD_R_NN(gb_opcode_t op, uint16_t d16) {
    *g_regmap_R[P(op)] = d16; return 12;
}


byte_t gb_LD_INDIRECT_NN_SP(gb_opcode_t op, uint16_t d16) {
    UNUSED(op);
    STOR16(d16) = SP; return 20; //pandoc says 12?
}

////////////////////////////////////////////////
//GMB 8bit - ALU
////////////////////////////////////////////////

byte_t gb_INC_D(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    byte_t v = REG8_WRITE( Y(op) )++;
    FLAGS(.ZR = (v == 0xFF),  
          .NG = 0, 
          .HC = HC_ADD(v, 1)); 

    return (Y(op) == HL_INDIRECT) ? 12 : 4;
}

byte_t gb_DEC_D(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    byte_t v = REG8_WRITE( Y(op) )--;
    
    FLAGS(.ZR = (v == 0x1),
          .NG = 1,
          .HC = HC_SUB(v, 1));

    return (Y(op) == HL_INDIRECT) ? 12 : 4;
} 

byte_t gb_ALU_A_N(gb_opcode_t op, uint16_t d16) {
    gb_alu_mode_e alu = Y(op);
    gb_word_t  n      = (gb_word_t)d16 & 0xFF;
    gb_word_t  r      = 0;

    switch (alu) {
    //   ALU MODE   |execute                 |Z            |N        |H         |C
    case gb_ALU_AND: r = A & n;    FLAGS_ALL(!r, 0, 1, 0);  break;
    case gb_ALU_XOR: r = A ^ n;    FLAGS_ALL(!r, 0, 0, 0);  break;
    case gb_ALU_OR:  r = A | n;    FLAGS_ALL(!r, 0, 0, 0);  break;

    case gb_ALU_ADC: n += GET_C();  // fallthrough;
    case gb_ALU_ADD: r = A + n;    FLAGS_ALL(!r, 0, HC_ADD(A, n), (r < A));break;
        break;

    case gb_ALU_SBC: n += GET_C();  // fallthrough;
    case gb_ALU_CP:                 // fallthrough;
    case gb_ALU_SUB: r = A - n;    FLAGS_ALL(!r, 1, HC_SUB(A, n), (n > A));
        break;
    }
    
    //for ALU CP - A remains unchanged: 
    A = (gb_ALU_CP == alu) ? A : r;
    
    return 4; //wrong i think
}

byte_t gb_ALU_A_D(gb_opcode_t op, uint16_t d16) {
    UNUSED(d16);
    gb_ALU_A_N(op, REG8_READ(Z(op))); //wrap gb_ALU_A_N
    return (Z(op) == HL_INDIRECT) ? 8 : 4;
}


byte_t gb_CPL(gb_opcode_t op, uint16_t d16) { //Complement A
    UNUSED(op, d16);
    A = ~A;

    FLAGS(.NG = 1,
          .HC = 1);
    return 4;
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


byte_t gb_PREFIX_CB(gb_opcode_t op, uint16_t d16) {
    //
    // Actual op-code is the second word after 0xCB, override to avoid typos:
    //
    assert(op == 0xCB);
    op = d16 & 0x00FF;   

    bool      const rot_left    = Q((op) == DIR_LEFT);
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
                r = v >> 1 | (v & 0x80); //MSB := intact 
                FLAGS_ALL(!r, 0, 0, 0);
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
    REG8_WRITE(Z(op)) = r; // Save result to destination register
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
    
    gb_dword_t v = (*g_regmap_R[P(op)]);
    gb_dword_t r = HL + v;
    FLAGS(.NG=0, 
          .HC=HC_ADD16(HL, v), 
          .CR=(r < HL));
    HL = r;
    return 8;
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
    PC += n; 
    return 12;
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
        STOR16(SP) = PC; //PC is already moved 
        PC = d16;
        return 24;
    }else return 12;
}

byte_t gb_CALL_N(gb_opcode_t op, uint16_t d16) {
    UNUSED(op); 
    SP -= 2;         //Push address of next instruction onto stack and thenjump to address nn.
    STOR16(SP) = PC; //PC is already moved 
    PC = d16;
    return 12;
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

    // I'm at conflict with the documentation here.
    // Which states I should put "present address" on stack,
    // instead of "push address of the next instruction" (i.e. like CALL).
    // For now I'm ignoring it and pretending this works like CALL,
    // I don't get 

    byte_t const N = Y(op) << 3;
    SP -= 2;
    STOR16(SP) = PC;
    PC = N;
    return 16; //other sources say 32..
}




////////////////////////////////////////////////
// CPU Control Commands
////////////////////////////////////////////////




byte_t gb_NOP(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16); 
    return 4;
}

byte_t gb_DI(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    g_GB.interrupts.IME = false;
    /*  TODO:
    This instruction disables interrupts but not
    immediately.Interrupts are disabled after
    instruction after DI is executed. */
    return 4;
}

byte_t gb_EI(gb_opcode_t op, uint16_t d16) {
    UNUSED(op, d16);
    g_GB.interrupts.IME = true;
    gb_INTERRUPT_request(0);
    return 4;
}