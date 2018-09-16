#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "gbz80.h"
#include "mmu.h"
#include "gpu.h"

gbz80_type z80 = {.af = 0, .bc = 0, .de = 0, .hl = 0,
        .sp = 0, .pc = 0, .m = 0, .t = 0,
        .halt = false, .stop = false, .ime = false, .new_ime = false,
        .int_f = 0, .int_en = 0, .clock = { /*.m = 0,
        .t = 0,*/ .main = 0, .sub = 0, .div_c = 0,
        .tima = 0, .tma = 0, .tac = 0}}, *z80_p;

static unsigned char op;
void printerr(unsigned char);


void init_z80() {
        z80_p = &z80;

        reset();
}

void scramble_z80() {
        unsigned short addr = 0;
        for (addr = 0; addr < 0xFFFF; addr++) {
                wb(addr, (unsigned char)rand());
        }
}

void reset() {
        z80.af = 0; z80.bc = 0;
        z80.de = 0; z80.hl = 0;
        z80.sp = 0;
        z80.pc = 0;

        z80.m = 0; z80.t = 0;

        // z80.clock.m = 0; z80.clock.t = 0;
        z80.stop = z80.halt = false;
        z80.ime = false;
}

void printerr(unsigned char opcode) {
        fprintf(stderr, "Oops! Something went wrong with opcode %c", opcode);
}

void fetch_dispatch_execute_loop() {
        while (!z80.halt) {
                fetch_dispatch_execute();
        }
}

int fetch_dispatch_execute() {

        int cycles = 0;

        if (!z80.halt) {
        op = rb(z80.pc++);
        unsigned char prefix = 0;

        if (op == 0xCB || op == 0xDD || op == 0xED || op == 0xFD) {
                prefix = op;
                op = rb(z80.pc++);
        }




        unsigned char n;
        unsigned short nn;

        unsigned char x, y, z, p, q;

        x = op >> 6;
        y = (op >> 3) & 0x7;
        z = op & 0x7;
        p = y >> 1;
        q = y % 2;

        if (prefix) {
                switch (x)
                {
                        case 0:
                                if (table_r[z] == NULL) {
                                        table_rot_HL[y]();
                                } else {
                                        table_rot[y](table_r[z]);
                                }
                                break;
                        case 1:
                                if (table_r[z] == NULL) {
                                        BIT_b_HL(y);
                                } else {
                                        BIT_b_r(y, *table_r[z]);
                                }
                                break;
                        case 2:
                                if (table_r[z] == NULL) {
                                        RES_b_HL(y);
                                } else {
                                        RES_b_r(y, table_r[z]);
                                }
                                break;
                        case 3:
                                if (table_r[z] == NULL) {
                                        SET_b_HL(y);
                                } else {
                                        SET_b_r(y, table_r[z]);
                                }
                                break;
                        default:
                                printerr(op);
                }
        } else {

        switch (x)
        {
                case 0:
                        switch (z)
                        {
                                case 0:
                                        switch (y)
                                        {
                                                case 0:
                                                        NOP();
                                                        break;
                                                case 1:
                                                        nn = rw(z80.pc);
                                                        z80.pc += 2;
                                                        LD_nn_SP(nn);
                                                        break;
                                                case 2:
                                                        STOP();
                                                        break;
                                                case 3:
                                                        n = rb(z80.pc);
                                                        if (n == 0xFE) {
                                                                // IE jumping in place
                                                                return 0;
                                                        }
                                                        z80.pc++;
                                                        JR_n((signed char)n);
                                                        break;
                                                case 4:
                                                case 5:
                                                case 6:
                                                case 7:
                                                        n = rb(z80.pc);
                                                        z80.pc++;
                                                        JR_cc_n(table_cc[y-4](), (signed char)n);
                                                        break;
                                                default:
                                                        printerr(op);
                                        };
                                        break;
                                case 1:
                                        if (!q) {
                                                nn = rw(z80.pc);
                                                z80.pc += 2;
                                                LD_n_nn(table_rp[p], nn);
                                        } else {
                                                ADD_HL_n(table_rp[p]);
                                        }
                                        break;
                                case 2:
                                        if (!q) {
                                                if (p == 0) {
                                                        LD_wb_A(z80.bc, 2);
                                                        /* LD (BC), A */
                                                } else if (p == 1) {
                                                        LD_wb_A(z80.de, 2);
                                                        /* LD (DE), A */
                                                } else if (p == 2) {
                                                        LDI_HL_A();
                                                } else if (p == 3) {
                                                        LDD_HL_A();
                                                } else { printerr(op); }
                                        } else {
                                                if (p == 0) {
                                                        LD_A_rb(z80.bc, 2);
                                                } else if (p == 1) {
                                                        LD_A_rb(z80.de, 2);
                                                } else if (p == 2) {
                                                        LDI_A_HL();
                                                } else if (p == 3) {
                                                        LDD_A_HL();
                                                } else { printerr(op); }
                                        }
                                        break;
                                case 3:
                                        if (!q) {
                                                INC_nn(table_rp[p]);
                                        } else {
                                                DEC_nn(table_rp[p]);
                                        }
                                        break;
                                case 4:
                                        if (table_r[y] == NULL) {
                                                INC_HL();
                                        } else {
                                                INC_n(table_r[y]);
                                        }
                                        break;
                                case 5:
                                        if (table_r[y] == NULL) {
                                                DEC_HL();
                                        } else {
                                                DEC_n(table_r[y]);
                                        }
                                        break;
                                case 6:
                                        n = rb(z80.pc);
                                        z80.pc++;
                                        if (table_r[y] == NULL) {
                                                LD_HL_n(n);
                                        } else {
                                                LD_nn_n(table_r[y], n);
                                        }
                                        break;
                                case 7:
                                        switch (y)
                                        {
                                                case 0:
                                                        RLCA();
                                                        break;
                                                case 1:
                                                        RRCA();
                                                        break;
                                                case 2:
                                                        RLA();
                                                        break;
                                                case 3:
                                                        RRA();
                                                        break;
                                                case 4:
                                                        DAA();
                                                        break;
                                                case 5:
                                                        CPL();
                                                        break;
                                                case 6:
                                                        SCF();
                                                        break;
                                                case 7:
                                                        CCF();
                                                        break;
                                                default:
                                                        printerr(op);
                                        };
                                        break;
                                default:
                                        printerr(op);
                        };
                        break;
                case 1:
                        if (z == 6 && y == 6) {
                                HALT();
                        } else {
                                if (table_r[y] == NULL) {
                                        LD_HL_r2(*table_r[z]);
                                } else if (table_r[z] == NULL) {
                                        LD_r1_r2(table_r[y], rb(z80.hl), 2);
                                } else {
                                        LD_r1_r2(table_r[y], *table_r[z], 1);
                                }
                        }
                        break;
                case 2:
                        if (table_r[z] == NULL) {
                                table_alu[y](rb(z80.hl), 2);
                        } else {
                                table_alu[y](*table_r[z], 1);
                        }
                        break;
                case 3:
                        switch (z)
                        {
                                case 0:
                                        switch (y)
                                        {
                                                case 0:
                                                case 1:
                                                case 2:
                                                case 3:
                                                        RET_cc(table_cc[y]());
                                                        break;
                                                case 4:
                                                        n = rb(z80.pc);
                                                        z80.pc++;
                                                        LDH_n_A(n);
                                                        break;
                                                case 5:
                                                        n = rb(z80.pc);
                                                        z80.pc++;
                                                        ADD_SP_n((signed char)n);
                                                        break;
                                                case 6:
                                                        n = rb(z80.pc);
                                                        z80.pc++;
                                                        LDH_A_n(n);
                                                        break;
                                                case 7:
                                                        n = rb(z80.pc);
                                                        z80.pc++;
                                                        LDHL_SP_n((signed char)n);
                                                        break;
                                                default:
                                                        printerr(op);
                                        }
                                        break;
                                case 1:
                                        if (!q) {
                                                POP_nn(table_rp2[p]);
                                        } else {
                                                if (p == 0) {
                                                        RET();
                                                } else if (p == 1) {
                                                        RETI();
                                                } else if (p == 2) {
                                                        JP_HL();
                                                } else if (p == 3) {
                                                        LD_SP_HL();
                                                }
                                        }
                                        break;
                                case 2:
                                        switch (y)
                                        {
                                                case 0:
                                                case 1:
                                                case 2:
                                                case 3:
                                                        nn = rw(z80.pc);
                                                        z80.pc += 2;
                                                        JP_cc_nn(table_cc[y](), nn);
                                                        break;
                                                case 4:
                                                        LD_C_A();
                                                        break;
                                                case 5:
                                                        nn = rw(z80.pc);
                                                        z80.pc += 2;
                                                        LD_wb_A(nn, 4);
                                                        break;
                                                case 6:
                                                        LD_A_C();
                                                        break;
                                                case 7:
                                                        nn = rw(z80.pc);
                                                        z80.pc += 2;
                                                        LD_A_rb(nn, 4);
                                                        break;
                                                default:
                                                        printerr(op);
                                        }
                                        break;
                                case 3:
                                        if (y == 0) {
                                                nn = rw(z80.pc);
                                                z80.pc += 2;
                                                JP_nn(nn);
                                        } else if (y == 6) {
                                                DI();
                                        } else if (y == 7) {
                                                EI();
                                        }
                                        break;
                                case 4:
                                        if (y >= 0 && y <= 3) {
                                                nn = rw(z80.pc);
                                                z80.pc += 2;
                                                CALL_cc_nn(table_cc[y](), nn);
                                        }
                                        break;
                                case 5:
                                        if (q == 0) {
                                                PUSH_nn(table_rp2[p]);
                                        } else if (p == 0) {
                                                nn = rw(z80.pc);
                                                z80.pc += 2;
                                                CALL_nn(nn);
                                        }
                                        break;
                                case 6:
                                        n = rb(z80.pc);
                                        z80.pc++;
                                        table_alu[y](n, 2);
                                        break;
                                case 7:
                                        RST_n(y*8);
                                        break;
                                default:
                                        printerr(op);
                        }
                        break;
                default:
                        printerr(op);
        }
        }
        } else {
                z80.m  = 1; z80.t = 4;
        }

        /* do other things i guess */

        z80.clock.m += z80.m;
        // z80.clock.t += z80.t;
        cycles += z80.m;
        update_clock();
        gpu_step();

        z80.m = z80.t = 0;

        unsigned char ints = z80.int_en & z80.int_f & 0x1F;

        if (ints) { z80.halt = false; }
        
        if (ints && z80.ime) {
                z80.ime = false;
                z80.new_ime = false;
                if (GET_BIT(ints, 0)) {
                        z80.int_f &= ~0x1;
                        RST40();
                        // do V-Blank
                } else if (GET_BIT(ints, 1)) {
                        z80.int_f &= ~0x2;
                        RST48();
                        // do LCD STAT
                } else if (GET_BIT(ints, 2)) {
                        z80.int_f &= ~0x4;
                        RST50();
                } else if (GET_BIT(ints, 3)) {
                        z80.int_f &= ~0x8;
                        RST58();
                } else if (GET_BIT(ints, 4)) {
                        z80.int_f &= ~0x10;
                        RST60();
                        // handle keypad interaction
                }
                z80.clock.m += z80.m;
                // z80.clock.t += z80.t;
                cycles += z80.m;
                update_clock();
        }

        if (z80.new_ime) z80.ime = z80.new_ime;

        return cycles;

}


void update_clock() {
        z80.clock.sub += z80.m;

        while (z80.clock.sub >= 4) {
                z80.clock.main++;
                z80.clock.sub -= 4;

                z80.clock.div_c++;
                if (z80.clock.div_c == 16) {
                        z80.clock.div++;
                        z80.clock.div_c = 0;
                }
                check_timer();
        }

        check_timer();
}

void check_timer() {
        unsigned char threshold = 0xFF;
        if (!(z80.clock.old_tac & 4) && (z80.clock.tac & 4)) {
                z80.clock.old_tac = z80.clock.tac;
        } else if (z80.clock.tac & 4) {
                switch (z80.clock.tac & 3)
                {
                        case 0: threshold = 64; break;
                        case 1: threshold = 1; break;
                        case 2: threshold = 4; break;
                        case 3: threshold = 16; break;
                }
                if (z80.clock.main >= threshold) {
                        z80.clock.main = 0;
                        z80.clock.tima++;
                        if (!z80.clock.tima) {
                                z80.clock.tima = z80.clock.tma;
                                z80.int_f |= 4;
                        }
                }
        }
}





/* Opcode definitions */

/* 8-bit loads */
void LD_nn_n(unsigned char *r, unsigned char n) {
        PRINT_ASM(n);
        *r = n;
        
        z80.m = 2;
        z80.t = 8;
}

void LD_r1_r2(unsigned char *r1, unsigned char r2, unsigned char m_time) {
        PRINT_ASM();
        *r1 = r2;
        
        z80.m = m_time;
        z80.t = 4 * m_time;
}

/* void LD_r1_HL(unsigned char *r1) {
        *r1 = rb(z80.hl);
        
        z80.m = 2;
        z80.t = 8;
} */

void LD_HL_r2(unsigned char r2) {
        PRINT_ASM();
        wb(z80.hl, r2);
        
        z80.m = 2;
        z80.t = 8;
}

void LD_HL_n(unsigned char n) {
        PRINT_ASM(n);
        wb(z80.hl, n);
        
        z80.m = 3;
        z80.t = 12;
}

/*void LD_A_n(unsigned char *n, unsigned char m_time) {
        PRINT_ASM(n);
        z80.a = *n;
        
        z80.m = m_time;
        z80.t = 4 * m_time;
}*/

void LD_A_rb(unsigned short nn, unsigned char m_time) {
        PRINT_ASM(nn);
        z80.a = rb(nn);

        z80.m = m_time;
        z80.t = 4 * m_time;
}

/*void LD_n_A(unsigned char *n, unsigned char m_time) {
        PRINT_ASM(n);
        *n = z80.a;
        
        z80.m = m_time;
        z80.t = 4 * m_time;
}*/

void LD_wb_A(unsigned short n, unsigned char m_time) {
        PRINT_ASM(n);
        wb(n, z80.a);
        
        z80.m = m_time;
        z80.t = 4 * m_time;
}

void LD_A_C() {
        PRINT_ASM(0xFF00 + z80.c);
        z80.a = rb(0xFF00 + z80.c);
        
        z80.m = 2;
        z80.t = 8;
}

void LD_C_A() {
        PRINT_ASM(0xFF00 + z80.c);
        wb(0xFF00 + z80.c, z80.a);

        // assert(z80.a == rb(0xFF00 + z80.c));
        
        z80.m = 2;
        z80.t = 8;
}

void LDD_A_HL() {
        PRINT_ASM();
        z80.a = rb(z80.hl);
        z80.hl--;
        
        z80.m = 2;
        z80.t = 8;
}

void LDD_HL_A() {
        PRINT_ASM();
        wb(z80.hl, z80.a);
        z80.hl--;
        
        z80.m = 2;
        z80.t = 8;
}

void LDI_A_HL() {
        PRINT_ASM();
        z80.a = rb(z80.hl);
        z80.hl++;
        
        z80.m = 2;
        z80.t = 8;
}

void LDI_HL_A() {
        PRINT_ASM();
        wb(z80.hl, z80.a);
        z80.hl++;
        
        z80.m = 2;
        z80.t = 8;
}

void LDH_n_A(unsigned char n) {
        PRINT_ASM(n);
        wb(0xFF00 + n, z80.a);

        // assert(z80.a == rb(0xFF00 + n));

        z80.m = 3;
        z80.t = 12;
}

void LDH_A_n(unsigned char n) {
        z80.a = rb(0xFF00 + n);
        PRINT_ASM(n,z80.a);
        z80.m = 3;
        z80.t = 12;
}


/* 16-bit loads */
void LD_n_nn(unsigned short *n, unsigned short nn) {
        PRINT_ASM(nn);
        *n = nn;
        z80.m = 3;
        z80.t = 12;
}

void LD_SP_HL() {
        PRINT_ASM();
        z80.sp = z80.hl;
        z80.m = 2;
        z80.t = 8;
}

void LDHL_SP_n(signed char n) {
        PRINT_ASM(n);
        int tmp = z80.sp;
        tmp += (int)n;

        z80.hl = (unsigned short)tmp;

        BIT_CLEAR(z80.f, ZERO);
        BIT_CLEAR(z80.f, SUBTRACT);

        SET_HC_ADD(z80.sp, n);
        SET_C_ADD(z80.sp, n);

        z80.m = 3;
        z80.t = 12;
}

void LD_nn_SP(unsigned short nn) {
        PRINT_ASM(nn);
        ww(nn, z80.sp);
        z80.m = 5;
        z80.t = 20;
}

void PUSH_nn(const unsigned short *nn) {
        PRINT_ASM();
        wb(--z80.sp, (*nn >> 8)  & 0xFF);
        wb(--z80.sp, *nn & 0xFF);
        z80.m = 4;
        z80.t = 16;
}

void POP_nn(unsigned short *nn) {
        PRINT_ASM();

        *nn = rw(z80.sp);
        if (nn == &z80.af) *nn &= (0xFFF0);

        z80.sp += 2;
        
        z80.m = 3;
        z80.t = 12;
}


/* 8-bit ALU */
void ADD_A_n(unsigned char n, unsigned char m_time) {
        PRINT_ASM(n);
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_HC_ADD(z80.a, n);
        SET_C_ADD(z80.a, n);


        z80.a += n;
        SET_Z_RES(z80.a);

        z80.m = m_time;
        z80.t = 4 * m_time;
}

void ADC_A_n(unsigned char n, unsigned char m_time) {
        unsigned short tmp = n;
        PRINT_ASM(n);
        tmp += GET_BIT(z80.f, CARRY); // This might come back to bite me
        SET_HC_ADD(z80.a, tmp);
        SET_C_ADD(z80.a, tmp);

        z80.a += tmp;
        BIT_CLEAR(z80.f, SUBTRACT);

        SET_Z_RES(z80.a);

        z80.m = m_time;
        z80.t = 4 * m_time;
}

void SUB_A_n(unsigned char n, unsigned char m_time) {
        PRINT_ASM(n);
        BIT_SET(z80.f, SUBTRACT);
        SET_HC_SUB(z80.a, n);
        SET_C_SUB(z80.a, n);
        
        z80.a -= n;

        SET_Z_RES(z80.a);
        
        z80.m = m_time;
        z80.t = 4 * m_time;
}
void SBC_A_n(unsigned char n, unsigned char m_time) {
        unsigned short tmp = n;
        PRINT_ASM(n);
        tmp += GET_BIT(z80.f, CARRY);
        
        BIT_SET(z80.f, SUBTRACT);
        SET_HC_SUB(z80.a, tmp);
        SET_C_SUB(z80.a, tmp);

        z80.a -= tmp;
        SET_Z_RES(z80.a);
        
        z80.m = m_time;
        z80.t = 4 * m_time;
}

void AND_n(unsigned char n, unsigned char m_time) {
        PRINT_ASM(n);
        z80.a &= n;

        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, CARRY);
        
        SET_Z_RES(z80.a);
        BIT_SET(z80.f, HALF_CARRY);

        z80.m = m_time;
        z80.t = 4 * m_time;
}

void OR_n(unsigned char n, unsigned char m_time) {
        PRINT_ASM(n);
        z80.a |= n;

        z80.f &= ~0x7F;
        SET_Z_RES(z80.a);

        z80.m = m_time;
        z80.t = 4 * m_time;
}

void XOR_n(unsigned char n, unsigned char m_time) {
        PRINT_ASM(n);
        z80.a ^= n;
        BIT_CLEAR(z80.f, CARRY);
        BIT_CLEAR(z80.f, HALF_CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);

        SET_Z_RES(z80.a);


        z80.m = m_time;
        z80.t = 4 * m_time;
}

void CP_n(unsigned char n, unsigned char m_time) {
        PRINT_ASM(n);
        SET_C_SUB(z80.a, n);
        SET_HC_SUB(z80.a, n);
        BIT_SET(z80.f, SUBTRACT);

        z80.a -= n;
        SET_Z_RES(z80.a);
        z80.a += n;

        z80.m = m_time;
        z80.t = 4 * m_time;
}

void INC_n(unsigned char *n) {
        PRINT_ASM(*n);

        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_EQUAL(z80.f, HALF_CARRY, ((*n) & 0xF) == (0xF));
        // SET_HC_ADD(*n, 1);
        (*n)++;
        SET_Z_RES(*n);

        z80.m = 1;
        z80.t = 4;
}

void INC_HL() {
        unsigned char tmp = rb(z80.hl);
        PRINT_ASM(tmp);

        SET_HC_ADD(tmp, 1);
        tmp += 1;

        wb(z80.hl, tmp);
        
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_Z_RES(tmp);

        z80.m = 3;
        z80.t = 12;
}

void DEC_n(unsigned char *n) {
        PRINT_ASM(*n);
        BIT_SET(z80.f, SUBTRACT);
        BIT_EQUAL(z80.f, HALF_CARRY, ((*n) & 0xF) == 0);
        // SET_HC_SUB(*n, 1);
        (*n)--;
        SET_Z_RES(*n);

        z80.m = 1;
        z80.t = 4;
}

void DEC_HL() {
        unsigned char tmp = rb(z80.hl);
        PRINT_ASM(tmp);

        SET_HC_SUB(tmp, 1);
        tmp -= 1;
        wb(z80.hl, tmp);

        BIT_SET(z80.f, SUBTRACT);
        SET_Z_RES(tmp);

        z80.m = 3;
        z80.t = 12;
}


/* 16-bit ALU */
void ADD_HL_n(const unsigned short *n) {
        PRINT_ASM();
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_HC_ADD_16(z80.hl, *n);
        SET_C_ADD_16(z80.hl, *n);

        z80.hl += *n;

        z80.m = 2;
        z80.t = 8;
}

void ADD_SP_n(signed char n) {
        PRINT_ASM(n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, ZERO);
        SET_HC_ADD(z80.sp, n);
        SET_C_ADD(z80.sp, n);

        z80.sp += n;

        z80.m = 4;
        z80.t = 16;
}

void INC_nn(unsigned short *nn) {
        PRINT_ASM();
        (*nn)++;

        z80.m = 2;
        z80.t = 8;
}

void DEC_nn(unsigned short *nn) {
        PRINT_ASM();
        (*nn)--;

        z80.m = 2;
        z80.t = 8;
}

/* Misc */
void SWAP_n(unsigned char *n) {
        PRINT_ASM_CB();
        unsigned char tmp = 0;
        tmp |= ((*n) >> 4) & 0x0F;
        tmp |= ((*n) << 4) & 0xF0;

        *n = tmp;

        BIT_CLEAR(z80.f, CARRY);
        BIT_CLEAR(z80.f, HALF_CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_Z_RES(tmp);

        z80.m = 2;
        z80.t = 8;
}

void SWAP_HL() {
        PRINT_ASM_CB();
        unsigned char tmp = 0, val;
        val = rb(z80.hl);

        tmp |= (val >> 4) & 0x0F;
        tmp |= (val << 4) & 0xF0;

        wb(z80.hl, tmp);

        BIT_CLEAR(z80.f, CARRY);
        BIT_CLEAR(z80.f, HALF_CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_Z_RES(tmp);

        z80.m = 4;
        z80.t = 16;
}


void DAA() {
        unsigned short s = z80.a;
        PRINT_ASM();
        if (!GET_BIT(z80.f, SUBTRACT)) {
                if (GET_BIT(z80.f, HALF_CARRY) || (z80.a & 0x0F) > 0x09) {
                        s += 0x06;
                }
                if (GET_BIT(z80.f, CARRY) || z80.a > 0x9F) {
                        s += 0x60;
                }
        } else {
                if (GET_BIT(z80.f, HALF_CARRY)) {
                        s = (s - 0x06) & (0xFF);
                }
                if (GET_BIT(z80.f, CARRY)) {
                        s -= 0x60;
                        z80.a -= 0x60;
                }
        }

        z80.a = (unsigned char)(s & 0xFF);
        SET_Z_RES(z80.a);
        BIT_CLEAR(z80.f, HALF_CARRY);

        if (s >= 0x100) BIT_SET(z80.f, CARRY);

        z80.m = 1;
        z80.t = 4;
}

void CPL() {
        PRINT_ASM();
        z80.a = ~z80.a;
        BIT_SET(z80.f, SUBTRACT);
        BIT_SET(z80.f, HALF_CARRY);

        z80.m = 1;
        z80.t = 4;
}

void CCF() {
        PRINT_ASM();
        BIT_FLIP(z80.f, CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 1;
        z80.t = 4;
}

void SCF() {
        PRINT_ASM();
        BIT_SET(z80.f, CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 1;
        z80.t = 4;
}

void NOP() {
        PRINT_ASM();
        z80.m = 1;
        z80.t = 4;
}

void HALT() {
        PRINT_ASM();
        z80.halt = true;

        z80.m = 1;
        z80.t = 4;
}

void STOP() {
        PRINT_ASM();
        z80.stop = true;

        z80.m = 1;
        z80.t = 4;
}

void DI() {
        PRINT_ASM();
        z80.ime = false;
        z80.new_ime = false;

        z80.m = 1;
        z80.t = 4;
}

void EI() {
        PRINT_ASM();
        z80.new_ime = true;

        z80.m = 1;
        z80.t = 4;
}

/* Rotates & Shifts */
void RLCA() {
        PRINT_ASM();
        z80.a = (unsigned char)((z80.a << 1) | (z80.a >> 7));
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z80.a, 0));

        BIT_CLEAR(z80.f, ZERO);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);
        
        z80.m = 1;
        z80.t = 4;
}

void RLA() {
        PRINT_ASM();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z80.a, 7));
        z80.a = (0xFF & (z80.a << 1)) | old_carry;

        BIT_CLEAR(z80.f, ZERO);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 1;
        z80.t = 4;
}

void RRCA() {
        PRINT_ASM();
        z80.a = (unsigned char)((z80.a >> 1) | (z80.a << 7));
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z80.a, 7));

        BIT_CLEAR(z80.f, ZERO);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 1;
        z80.t = 4;
}

void RRA() {
        PRINT_ASM();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z80.a, 0));
        z80.a = (unsigned char)((z80.a >> 1) | (old_carry << 7));

        BIT_CLEAR(z80.f, ZERO);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 1;
        z80.t = 4;
}

void RLC_n(unsigned char *n) {
        PRINT_ASM_CB();
        *n = (unsigned char)((*n << 1) | (*n >> 7));
        BIT_EQUAL(z80.f, CARRY, GET_BIT(*n, 0));

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);
        
        z80.m = 2;
        z80.t = 8;
}

void RLC_HL() {
        PRINT_ASM_CB();
        unsigned char val = rb(z80.hl);
        val = (unsigned char)((val << 1) | (val >> 7));
        wb(z80.hl, val);

        BIT_EQUAL(z80.f, CARRY, GET_BIT(val, 0));

        SET_Z_RES(val);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);
        
        z80.m = 4;
        z80.t = 16;
}

void RL_n(unsigned char *n) {
        PRINT_ASM_CB();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        BIT_EQUAL(z80.f, CARRY, GET_BIT(*n, 7));
        *n = (unsigned char)((0xFF & (*n << 1)) | old_carry);

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 2;
        z80.t = 8;
}

void RL_HL() {
        PRINT_ASM_CB();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        unsigned char val = rb(z80.hl);

        BIT_EQUAL(z80.f, CARRY, GET_BIT(val, 7));
        val = (unsigned char)((0xFF & (val << 1)) | old_carry);
        wb(z80.hl, val);

        SET_Z_RES(val);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 4;
        z80.t = 16;
}

void RRC_n(unsigned char *n) {
        PRINT_ASM_CB();
        *n = (unsigned char)((*n >> 1) | (*n << 7));
        BIT_EQUAL(z80.f, CARRY, GET_BIT(*n, 7));

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 2;
        z80.t = 8;
}

void RRC_HL() {
        PRINT_ASM_CB();
        unsigned char val = rb(z80.hl);

        val = (unsigned char)((val >> 1) | (val << 7));
        BIT_EQUAL(z80.f, CARRY, GET_BIT(val, 7));
        wb(z80.hl, val);

        SET_Z_RES(val);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 4;
        z80.t = 16;
}

void RR_n(unsigned char *n) {
        PRINT_ASM_CB();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        BIT_EQUAL(z80.f, CARRY, (*n)&1);
        *n = (unsigned char)((*n >> 1) | (old_carry << 7));

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 2;
        z80.t = 8;
}

void RR_HL() {
        PRINT_ASM_CB();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        unsigned char val = rb(z80.hl);
        BIT_EQUAL(z80.f, CARRY, val&1);
        val = (unsigned char)((val >> 1) | (old_carry << 7));
        wb(z80.hl, val);

        SET_Z_RES(val);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 4;
        z80.t = 16;
}

void SLA_n(unsigned char *n) {
        PRINT_ASM_CB();
        BIT_EQUAL(z80.f, CARRY, GET_BIT(*n, 7));
        *n = (unsigned char)(*n << 1);

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 2;
        z80.t = 8;
}

void SLA_HL() {
        PRINT_ASM_CB();
        unsigned char val = rb(z80.hl);

        BIT_EQUAL(z80.f, CARRY, GET_BIT(val, 7));
        val = (unsigned char)(val << 1);
        wb(z80.hl, val);

        SET_Z_RES(val);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 4;
        z80.t = 16;
}

void SRA_n(unsigned char *n) {
        PRINT_ASM_CB();
        BIT_EQUAL(z80.f, CARRY, GET_BIT(*n, 0));
        *n = (unsigned char) (((signed char) *n) >> 1);

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 2;
        z80.t = 8;
}

void SRA_HL() {
        PRINT_ASM_CB();
        unsigned char val = rb(z80.hl);
        BIT_EQUAL(z80.f, CARRY, GET_BIT(val, 0));
        val = (unsigned char) (((signed char) val) >> 1);
        wb(z80.hl, val);

        SET_Z_RES(val);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 4;
        z80.t = 16;
}

void SRL_n(unsigned char *n) {
        PRINT_ASM_CB();
        BIT_EQUAL(z80.f, CARRY, (*n) & 0x1);
        *n =  (unsigned char)((*n) >> 1);

        assert(GET_BIT(*n, 7) == 0);

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 2;
        z80.t = 8;
}

void SRL_HL() {
        PRINT_ASM_CB();
        unsigned char val = rb(z80.hl);
        BIT_EQUAL(z80.f, CARRY, val & 0x1);
        val =  (unsigned char)(val >> 1);

        assert(GET_BIT(val, 7) == 0);
        wb(z80.hl, val);

        SET_Z_RES(val);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.m = 4;
        z80.t = 16;
}


/* Bit Opcodes */
void BIT_b_r(unsigned char b, unsigned char r) {
        PRINT_ASM_CB(b);
        BIT_EQUAL(z80.f, ZERO, !BIT_CHECK(r, b));
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_SET(z80.f, HALF_CARRY);

        z80.m = 2;
        z80.t = 8;
}

void BIT_b_HL(unsigned char b) {
        PRINT_ASM_CB(b);
        unsigned char r = rb(z80.hl);
        BIT_EQUAL(z80.f, ZERO, !BIT_CHECK(r, b));
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_SET(z80.f, HALF_CARRY);

        z80.m = 3;
        z80.t = 12;
}

void SET_b_r(unsigned char b, unsigned char *r) {
        PRINT_ASM_CB(b);
        BIT_SET(*r, b);

        z80.m = 2;
        z80.t = 8;
}

void SET_b_HL(unsigned char b) {
        PRINT_ASM_CB(b);
        unsigned char r = rb(z80.hl);
        BIT_SET(r, b);
        wb(z80.hl, r);

        z80.m = 4;
        z80.t = 16;
}

void RES_b_r(unsigned char b, unsigned char *r) {
        PRINT_ASM_CB(b);
        BIT_CLEAR(*r, b);

        z80.m = 2;
        z80.t = 8;
}

void RES_b_HL(unsigned char b) {
        PRINT_ASM_CB(b);
        unsigned char r = rb(z80.hl);
        BIT_CLEAR(r, b);
        wb(z80.hl, r);

        z80.m = 4;
        z80.t = 16;
}


/* Jumps */
void JP_nn(unsigned short nn) {
        PRINT_ASM(nn);
        z80.pc = nn;

        z80.m = 4;
        z80.t = 16;
        /* maybe takes another m-cycle to jump. same for JP_cc_nn */
}

void JP_cc_nn(bool cc, unsigned short nn) {
        PRINT_ASM(nn);

        z80.m = 3;
        z80.t = 12;
        if (cc) {z80.pc = nn; z80.m += 1; z80.t += 4;}
}

void JP_HL() {
        PRINT_ASM();
        z80.pc = z80.hl;

        z80.m = 1;
        z80.t = 4;
}

void JR_n(signed char n) {
        PRINT_ASM(n);
        z80.pc += n;

        z80.m = 3;
        z80.t = 12;
}

void JR_cc_n(bool cc, signed char n) {
        PRINT_ASM(n);

        z80.m = 2;
        z80.t = 8;
        if (cc) {z80.pc += n; z80.m += 1; z80.t += 4;}
}


/* Calls */
void CALL_nn(unsigned short nn) {
        PRINT_ASM(nn);
        z80.sp -= 2;
        ww(z80.sp, z80.pc);
        z80.pc = nn;

        z80.m = 6;
        z80.t = 24;
}

void CALL_cc_nn(bool cc, unsigned short nn) {
        PRINT_ASM(nn);
        if (cc) {
                z80.sp -= 2;
                ww(z80.sp, z80.pc);
                z80.pc = nn;

                z80.m = 6;
                z80.t = 24;
                return;
        }

        z80.m = 3;
        z80.t = 12;
}


/* Restarts */
void RST_n(unsigned char n) {
        PRINT_ASM(n);
        z80.sp -= 2;
        ww(z80.sp, z80.pc);
        z80.pc = n;

        z80.m = 4;
        z80.t = 16;
}

void RST40() {
        // printf("0x%04x RST 40\n", z80.pc);
        z80.sp -= 2;
        ww(z80.sp, z80.pc);

        z80.pc = 0x40;
        z80.m = 4;
        z80.t = 16;
}

void RST48() {
        // printf("0x%04x RST 48\n", z80.pc);
        z80.sp -= 2;
        ww(z80.sp, z80.pc);

        z80.pc = 0x48;
        z80.m = 4;
        z80.t = 16;
}

void RST50() {
        // printf("0x%04x RST 50\n", z80.pc);
        z80.sp -= 2;
        ww(z80.sp, z80.pc);

        z80.pc = 0x50;
        z80.m = 4;
        z80.t = 16;
}

void RST58() {
        // printf("0x%04x RST 58\n", z80.pc);
        z80.sp -= 2;
        ww(z80.sp, z80.pc);

        z80.pc = 0x58;
        z80.m = 4;
        z80.t = 16;
}

void RST60() {
        // printf("0x%04x RST 60\n", z80.pc);
        z80.sp -= 2;
        ww(z80.sp, z80.pc);

        z80.pc = 0x60;
        z80.m = 4;
        z80.t = 16;
}


/* Returns */
void RET() {
        PRINT_ASM();
        z80.pc = rw(z80.sp);
        z80.sp += 2;

        z80.m = 4;
        z80.t = 16;
}

void RET_cc(bool cc) {
        PRINT_ASM();
        if (cc) {
                z80.pc = rw(z80.sp);
                z80.sp += 2;

                z80.m = 5;
                z80.t = 20;
                return;
        }

        z80.m = 2;
        z80.t = 8;
}


void RETI() {
        PRINT_ASM();
        z80.pc = rw(z80.sp);
        z80.sp += 2;
        z80.ime = true;

        z80.m = 4;
        z80.t = 16;
}

static bool (*table_cc[])(void) = {
        cc_nz, cc_z, cc_nc, cc_c
};

static void (*table_alu[])(unsigned char, unsigned char) = {
        ADD_A_n, ADC_A_n, SUB_A_n, SBC_A_n, AND_n, XOR_n, OR_n, CP_n
};

static void (*table_rot[])(unsigned char*) = {
        RLC_n, RRC_n, RL_n, RR_n, SLA_n, SRA_n, SWAP_n, SRL_n
};
static void (*table_rot_HL[])(void) = {
        RLC_HL, RRC_HL, RL_HL, RR_HL, SLA_HL, SRA_HL, SWAP_HL, SRL_HL
};


bool cc_nz() {
        return !BIT_CHECK(z80.f, ZERO);
}

bool cc_z() {
        return BIT_CHECK(z80.f, ZERO);
}

bool cc_nc() {
        return !BIT_CHECK(z80.f, CARRY);
}

bool cc_c() {
        return BIT_CHECK(z80.f, CARRY);
}

static unsigned char *table_r[] = {
        &z80.b, &z80.c, &z80.d, &z80.e, &z80.h, &z80.l, NULL, &z80.a
};

static unsigned short *table_rp[] = {
        &z80.bc, &z80.de, &z80.hl, &z80.sp
};

static unsigned short *table_rp2[] = {
        &z80.bc, &z80.de, &z80.hl, &z80.af
};
