#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "gbz80.h"
#include "mmu.h"
#include "gpu.h"

gbz80_type z80 = {
               .af = 0,
               .bc = 0,
               .de = 0,
               .hl = 0,
               .sp = 0,
               .pc = 0,
               .m = 0,
               .t = 0,
               .halt = false,
               .stop = false,
               .ime = false,
               .new_ime = false,
               .int_f = 0,
               .int_en = 0,
               .clock = {
                   .m = 0,
                   .tima = 0,
                   .tma = 0,
                   .tac = 0,
                   .old_tac = 0,
                   .long_time = 0},
               .ir = 0},
           *z80_p;

unsigned char z, w;
unsigned short wz;
signed char z_s;

void printerr(unsigned char);

void init_z80()
{
        z80_p = &z80;

        reset();
}

void scramble_z80()
{
        unsigned short addr = 0;
        for (addr = 0; addr < 0xFFFF; addr++)
        {
                wb(addr, (unsigned char)rand());
        }
}

void reset()
{
        z80.af = 0;
        z80.bc = 0;
        z80.de = 0;
        z80.hl = 0;
        z80.sp = 0;
        z80.pc = 0;

        z80.m = 0;
        z80.t = 0;

        // z80.clock.m = 0; z80.clock.t = 0;
        z80.stop = z80.halt = false;
        z80.ime = false;
}

void printerr(unsigned char opcode)
{
        fprintf(stderr, "Oops! Something went wrong with opcode %c", opcode);
}

int cpu_tick()
{
        unsigned char prefix = 0;
        unsigned char n;
        unsigned short nn;
        unsigned char x, y, z, p, q;
        // need IR
        // based on IR, do something
        if (!z80.halt)
        {
                if (z80.ir == 0xCB)
                {
                        prefix = 0xCB;
                        // tick
                        clock_m_tick();
                        gpu_m_tick();
                        // fetch next opcode
                        z80.ir = rb_cpu(z80.pc++);
                }
                x = z80.ir >> 6;
                y = (z80.ir >> 3) & 0x7;
                z = z80.ir & 0x7;
                p = y >> 1;
                q = y % 2;
                if (prefix)
                {
                        switch (x)
                        {
                        case 0:
                                if (table_r[z] == NULL)
                                {
                                        table_rot_HL_t[y]();
                                }
                                else
                                {
                                        table_rot_t[y](table_r[z]);
                                }
                                break;
                        case 1:
                                if (table_r[z] == NULL)
                                {
                                        BIT_b_HL_t(y);
                                }
                                else
                                {
                                        BIT_b_r_t(y, *table_r[z]);
                                }
                                break;
                        case 2:
                                if (table_r[z] == NULL)
                                {
                                        RES_b_HL_t(y);
                                }
                                else
                                {
                                        RES_b_r_t(y, table_r[z]);
                                }
                                break;
                        case 3:
                                if (table_r[z] == NULL)
                                {
                                        SET_b_HL_t(y);
                                }
                                else
                                {
                                        SET_b_r_t(y, table_r[z]);
                                }
                                break;
                        default:
                                printerr(z80.ir);
                        }
                }
                else
                {

                        switch (x)
                        {
                        case 0:
                                switch (z)
                                {
                                case 0:
                                        switch (y)
                                        {
                                        case 0:
                                                NOP_t();
                                                break;
                                        case 1:
                                                LD_nn_SP_t();
                                                break;
                                        case 2:
                                                STOP_t();
                                                break;
                                        case 3:
                                                JR_n_t();
                                                break;
                                        case 4:
                                        case 5:
                                        case 6:
                                        case 7:
                                                JR_cc_n_t(table_cc[y - 4]());
                                                break;
                                        default:
                                                printerr(z80.ir);
                                        };
                                        break;
                                case 1:
                                        if (!q)
                                        {
                                                LD_n_nn_t(table_rp[p]);
                                        }
                                        else
                                        {
                                                ADD_HL_n_t(table_rp[p]);
                                        }
                                        break;
                                case 2:
                                        if (!q)
                                        {
                                                if (p == 0)
                                                {
                                                        LD_rp_A_t(z80.bc);
                                                        /* LD (BC), A */
                                                }
                                                else if (p == 1)
                                                {
                                                        LD_rp_A_t(z80.de);
                                                        /* LD (DE), A */
                                                }
                                                else if (p == 2)
                                                {
                                                        LDI_HL_A_t();
                                                }
                                                else if (p == 3)
                                                {
                                                        LDD_HL_A_t();
                                                }
                                                else
                                                {
                                                        printerr(z80.ir);
                                                }
                                        }
                                        else
                                        {
                                                if (p == 0)
                                                {
                                                        LD_A_rp_t(z80.bc);
                                                }
                                                else if (p == 1)
                                                {
                                                        LD_A_rp_t(z80.de);
                                                }
                                                else if (p == 2)
                                                {
                                                        LDI_A_HL_t();
                                                }
                                                else if (p == 3)
                                                {
                                                        LDD_A_HL_t();
                                                }
                                                else
                                                {
                                                        printerr(z80.ir);
                                                }
                                        }
                                        break;
                                case 3:
                                        if (!q)
                                        {
                                                INC_nn_t(table_rp[p]);
                                        }
                                        else
                                        {
                                                DEC_nn_t(table_rp[p]);
                                        }
                                        break;
                                case 4:
                                        if (table_r[y] == NULL)
                                        {
                                                INC_HL_t();
                                        }
                                        else
                                        {
                                                INC_n_t(table_r[y]);
                                        }
                                        break;
                                case 5:
                                        if (table_r[y] == NULL)
                                        {
                                                DEC_HL_t();
                                        }
                                        else
                                        {
                                                DEC_n_t(table_r[y]);
                                        }
                                        break;
                                case 6:
                                        if (table_r[y] == NULL)
                                        {
                                                LD_HL_n_t();
                                        }
                                        else
                                        {
                                                LD_r1_n_t(table_r[y]);
                                        }
                                        break;
                                case 7:
                                        switch (y)
                                        {
                                        case 0:
                                                RLCA_t();
                                                break;
                                        case 1:
                                                RRCA_t();
                                                break;
                                        case 2:
                                                RLA_t();
                                                break;
                                        case 3:
                                                RRA_t();
                                                break;
                                        case 4:
                                                DAA_t();
                                                break;
                                        case 5:
                                                CPL_t();
                                                break;
                                        case 6:
                                                SCF_t();
                                                break;
                                        case 7:
                                                CCF_t();
                                                break;
                                        default:
                                                printerr(z80.ir);
                                        };
                                        break;
                                default:
                                        printerr(z80.ir);
                                };
                                break;
                        case 1:
                                if (z == 6 && y == 6)
                                {
                                        HALT_t();
                                }
                                else
                                {
                                        if (table_r[y] == NULL)
                                        {
                                                LD_HL_r2_t(*table_r[z]);
                                        }
                                        else if (table_r[z] == NULL)
                                        {
                                                LD_r1_HL_t(table_r[y]);
                                        }
                                        else
                                        {
                                                LD_r1_r2_t(table_r[y], *table_r[z]);
                                        }
                                }
                                break;
                        case 2:
                                if (table_r[z] == NULL)
                                {
                                        table_alu_hl_t[y]();
                                }
                                else
                                {
                                        table_alu_r_t[y](*table_r[z]);
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
                                                RET_cc_t(table_cc[y]());
                                                break;
                                        case 4:
                                                LDH_n_A_t();
                                                break;
                                        case 5:
                                                ADD_SP_n_t();
                                                break;
                                        case 6:
                                                LDH_A_n_t();
                                                break;
                                        case 7:
                                                LDHL_SP_n_t();
                                                break;
                                        default:
                                                printerr(z80.ir);
                                        }
                                        break;
                                case 1:
                                        if (!q)
                                        {
                                                POP_nn_t(table_rp2[p]);
                                        }
                                        else
                                        {
                                                if (p == 0)
                                                {
                                                        RET_t();
                                                }
                                                else if (p == 1)
                                                {
                                                        RETI_t();
                                                }
                                                else if (p == 2)
                                                {
                                                        JP_HL_t();
                                                }
                                                else if (p == 3)
                                                {
                                                        LD_SP_HL_t();
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
                                                JP_cc_nn_t(table_cc[y]());
                                                break;
                                        case 4:
                                                LD_C_A_t();
                                                break;
                                        case 5:
                                                LD_nn_A_t();
                                                break;
                                        case 6:
                                                LD_A_C_t();
                                                break;
                                        case 7:
                                                LD_A_nn_t();
                                                break;
                                        default:
                                                printerr(z80.ir);
                                        }
                                        break;
                                case 3:
                                        if (y == 0)
                                        {
                                                JP_nn_t();
                                        }
                                        else if (y == 6)
                                        {
                                                DI_t();
                                        }
                                        else if (y == 7)
                                        {
                                                EI_t();
                                        }
                                        break;
                                case 4:
                                        if (y >= 0 && y <= 3)
                                        {
                                                CALL_cc_nn_t(table_cc[y]());
                                        }
                                        break;
                                case 5:
                                        if (q == 0)
                                        {
                                                PUSH_nn_t(table_rp2[p]);
                                        }
                                        else if (p == 0)
                                        {
                                                CALL_nn_t();
                                        }
                                        break;
                                case 6:
                                        table_alu_n_t[y]();
                                        break;
                                case 7:
                                        RST_n_t(y * 8);
                                        break;
                                default:
                                        printerr(z80.ir);
                                }
                                break;
                        default:
                                printerr(z80.ir);
                        }
                }
        }
        else
        {
                clock_m_tick();
                gpu_m_tick();
        }

        unsigned char ints = z80.int_en & z80.int_f & 0x1F;

        if (ints)
        {
                z80.halt = false;
        }

        if (ints && z80.ime)
        {
                clock_m_tick();
                gpu_m_tick();
                z80.ime = false;
                z80.new_ime = false;
                z80.pc--;

                clock_m_tick();
                gpu_m_tick();
                z80.sp--;

                clock_m_tick();
                gpu_m_tick();
                wb_cpu(z80.sp, z80.pc >> 8);
                z80.sp--;

                ints = z80.int_en & z80.int_f & 0x1F;
                if (GET_BIT(ints, 0))
                {
                        z80.int_f &= ~0x1;
                        RST_IRQ_n_t(0x40);
                        // do V-Blank
                }
                else if (GET_BIT(ints, 1))
                {
                        z80.int_f &= ~0x2;
                        RST_IRQ_n_t(0x48);
                        // do LCD STAT
                }
                else if (GET_BIT(ints, 2))
                {
                        z80.int_f &= ~0x4;
                        RST_IRQ_n_t(0x50);
                }
                else if (GET_BIT(ints, 3))
                {
                        z80.int_f &= ~0x8;
                        RST_IRQ_n_t(0x58);
                }
                else if (GET_BIT(ints, 4))
                {
                        z80.int_f &= ~0x10;
                        RST_IRQ_n_t(0x60);
                        // printf("keypad IRQ\n");
                        // handle keypad interaction
                }
                else
                {
                        RST_IRQ_n_t(0x00);
                }
                // z80.clock.m += z80.m;
                // z80.clock.t += z80.t;
        }

        if (z80.new_ime)
                z80.ime = z80.new_ime;

        return 1;
}

void clock_m_tick()
{
        unsigned short old_m = z80.clock.m;
        unsigned short threshold = 1 << (3 + 2 * (((z80.clock.tac & 3) - 1) & 3));

        z80.clock.long_time++;

        z80.clock.tima_reloaded = false;
        if ((z80.clock.tac & 4) && (!z80.clock.tima))
        {
                z80.clock.tima = z80.clock.tma;
                z80.int_f |= 4;
                z80.clock.tima_reloaded = true;
        }

        if (z80.clock.tac & 4)
        {
                // do it 4 times
                z80.clock.m += 4;

                if ((old_m & threshold) && !(z80.clock.m & threshold))
                {
                        z80.clock.tima++;
                        if (!z80.clock.tima)
                        {
                                // tima should hold 0 for 4 cycles before being reloaded
                                // z80.clock.tima = z80.clock.tma;
                                // z80.int_f |= 4;
                        }
                }
                old_m = z80.clock.m;
        }
        else
        {
                z80.clock.m += 4;
        }
        mmu->rtc.m_cycles++;
        if ((mmu->rtc.m_cycles >= 32) && (!mmu->rtc.halt_flag))
        {
                mmu->rtc.m_cycles -= 32;
                mmu->rtc.ticks++;
                if (mmu->rtc.ticks >= 0x8000)
                {
                        mmu->rtc.ticks -= 0x8000;
                        mmu->rtc.seconds = (mmu->rtc.seconds + 1) & 0x3F;
                        if (mmu->rtc.seconds == 60)
                        {
                                mmu->rtc.seconds -= 60;
                                mmu->rtc.minutes = (mmu->rtc.minutes + 1) & 0x3F;
                                if (mmu->rtc.minutes == 60)
                                {
                                        mmu->rtc.minutes -= 60;
                                        mmu->rtc.hours = (mmu->rtc.hours + 1) & 0x1F;
                                        if (mmu->rtc.hours == 24)
                                        {
                                                mmu->rtc.hours -= 24;
                                                mmu->rtc.day_counter = (mmu->rtc.day_counter + 1) & 0x3FF;
                                                if (mmu->rtc.day_counter == 512)
                                                {
                                                        mmu->rtc.day_counter_carry = true;
                                                        mmu->rtc.day_counter -= 512;
                                                }
                                        }
                                }
                        }
                }
        }
}

/* Opcode definitions */

/* 8-bit loads */
void LD_r1_n_t(unsigned char *r1)
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        *r1 = z;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
        clock_m_tick();
        gpu_m_tick();
}

void LD_r1_r2_t(unsigned char *r1, unsigned char r2)
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        *r1 = r2;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LD_r1_HL_t(unsigned char *r1)
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        *r1 = z;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/* void LD_r1_HL(unsigned char *r1) {
        *r1 = rb_cpu(z80.hl);

        z80.m = 2;
        z80.t = 8;
} */

void LD_HL_r2_t(unsigned char r2)
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.hl, r2);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LD_HL_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.hl, z);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/*void LD_A_n(unsigned char *n, unsigned char m_time) {
        PRINT_ASM(n);
        z80.a = *n;

        z80.m = m_time;
        z80.t = 4 * m_time;
}*/

void LD_A_nn_t()
{
        PRINT_ASM(rw(z80.pc));
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu((w << 8) | z);

        clock_m_tick();
        gpu_m_tick();
        z80.a = z;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LD_A_rp_t(unsigned short n)
{
        clock_m_tick();
        gpu_m_tick();
        PRINT_ASM(n);
        z = rb_cpu(n);

        clock_m_tick();
        gpu_m_tick();
        z80.a = z;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/*void LD_n_A(unsigned char *n, unsigned char m_time) {
        PRINT_ASM(n);
        *n = z80.a;

        z80.m = m_time;
        z80.t = 4 * m_time;
}*/

void LD_nn_A_t()
{
        PRINT_ASM(rw(z80.pc));
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        wb_cpu((w << 8) | z, z80.a);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LD_rp_A_t(unsigned short n)
{
        PRINT_ASM(n);
        clock_m_tick();
        gpu_m_tick();
        wb_cpu(n, z80.a);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LD_A_C_t()
{
        PRINT_ASM(0xFF00 + z80.c);
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(0xFF00 + z80.c);

        clock_m_tick();
        gpu_m_tick();
        z80.a = z;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LD_C_A_t()
{
        PRINT_ASM(0xFF00 + z80.c);
        clock_m_tick();
        gpu_m_tick();
        wb_cpu(0xFF00 + z80.c, z80.a);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LDD_A_HL_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        z80.hl--;

        clock_m_tick();
        gpu_m_tick();
        z80.a = z;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LDD_HL_A_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.hl, z80.a);
        z80.hl--;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LDI_A_HL_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        z80.hl++;

        clock_m_tick();
        gpu_m_tick();
        z80.a = z;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LDI_HL_A_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.hl, z80.a);
        z80.hl++;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LDH_n_A_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z, z80.a);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        wb_cpu(0xFF00 + z, z80.a);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        // printf("PC: %04hx\t IR: %02hhx\n", z80.pc, z80.ir);
        z80.pc++;
}

void LDH_A_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z, rb(0xFF00 + z));
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(0xFF00 + z);

        clock_m_tick();
        gpu_m_tick();
        z80.a = z;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/* 16-bit loads */

void LD_n_nn_t(unsigned short *n)
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(rw(z80.pc));
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
        *n = (w << 8) | z;
}

void LD_SP_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        PRINT_ASM();
        z80.sp = z80.hl;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LDHL_SP_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z_s = rb_cpu(z80.pc);
        PRINT_ASM(z_s);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        BIT_CLEAR(z80.f, ZERO);
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_HC_ADD(z80.sp & 0xFF, z_s);
        SET_C_ADD(z80.sp & 0xFF, z_s);
        // z80.l = (z80.sp & 0xFF) + z_s;

        clock_m_tick();
        gpu_m_tick();
        // z80.h = (z80.sp >> 8) + GET_BIT(z80.f, CARRY);
        z80.hl = z80.sp + z_s;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void LD_nn_SP_t()
{
        PRINT_ASM(rw(z80.pc));
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        wz = (w << 8) | z;
        wb_cpu(wz, z80.sp & 0xFF);
        wz++;

        clock_m_tick();
        gpu_m_tick();
        wb_cpu(wz, z80.sp >> 8);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void PUSH_nn_t(const unsigned short *nn)
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z80.sp--;

        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.sp, (*nn >> 8) & 0xFF);
        z80.sp--;

        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.sp, *nn & 0xFF);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void POP_nn_t(unsigned short *nn)
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.sp);
        z80.sp++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.sp);
        z80.sp++;

        clock_m_tick();
        gpu_m_tick();
        *nn = (w << 8) + z;
        if (nn == &z80.af)
                *nn &= (0xFFF0);
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/* 8-bit ALU */
void ADD_A_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_HC_ADD(z80.a, z);
        SET_C_ADD(z80.a, z);

        z80.a += z;
        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void ADD_A_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        PRINT_ASM(z);

        clock_m_tick();
        gpu_m_tick();
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_HC_ADD(z80.a, z);
        SET_C_ADD(z80.a, z);

        z80.a += z;
        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void ADD_A_r_t(unsigned char n)
{
        PRINT_ASM(n);
        clock_m_tick();
        gpu_m_tick();
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_HC_ADD(z80.a, n);
        SET_C_ADD(z80.a, n);

        z80.a += n;
        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void ADC_A_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        unsigned char carry = GET_BIT(z80.f, CARRY);
        SET_HC_ADC(z80.a, z, carry);
        SET_C_ADC(z80.a, z, carry);

        z80.a = 0xFF & (z80.a + z + carry);
        BIT_CLEAR(z80.f, SUBTRACT);

        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void ADC_A_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        PRINT_ASM(z);

        clock_m_tick();
        gpu_m_tick();
        unsigned char carry = GET_BIT(z80.f, CARRY);
        SET_HC_ADC(z80.a, z, carry);
        SET_C_ADC(z80.a, z, carry);

        z80.a = 0xFF & (z80.a + z + carry);
        BIT_CLEAR(z80.f, SUBTRACT);

        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void ADC_A_r_t(unsigned char n)
{
        PRINT_ASM(n);
        clock_m_tick();
        gpu_m_tick();

        unsigned char carry = GET_BIT(z80.f, CARRY);
        SET_HC_ADC(z80.a, n, carry);
        SET_C_ADC(z80.a, n, carry);

        z80.a = 0xFF & (z80.a + n + carry);
        BIT_CLEAR(z80.f, SUBTRACT);

        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SUB_A_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        BIT_SET(z80.f, SUBTRACT);
        SET_HC_SUB(z80.a, z);
        SET_C_SUB(z80.a, z);

        z80.a -= z;

        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SUB_A_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        PRINT_ASM(z);

        clock_m_tick();
        gpu_m_tick();
        BIT_SET(z80.f, SUBTRACT);
        SET_HC_SUB(z80.a, z);
        SET_C_SUB(z80.a, z);

        z80.a -= z;

        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SUB_A_r_t(unsigned char n)
{
        PRINT_ASM(n);
        clock_m_tick();
        gpu_m_tick();

        BIT_SET(z80.f, SUBTRACT);
        SET_HC_SUB(z80.a, n);
        SET_C_SUB(z80.a, n);

        z80.a -= n;

        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SBC_A_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z);
        z80.pc++;
        unsigned char carry = GET_BIT(z80.f, CARRY);

        clock_m_tick();
        gpu_m_tick();
        BIT_SET(z80.f, SUBTRACT);
        SET_HC_SBC(z80.a, z, carry);
        SET_C_SBC(z80.a, z, carry);

        z80.a = 0xFF & (z80.a - z - carry);
        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SBC_A_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        PRINT_ASM(z);
        unsigned char carry = GET_BIT(z80.f, CARRY);

        clock_m_tick();
        gpu_m_tick();
        BIT_SET(z80.f, SUBTRACT);
        SET_HC_SBC(z80.a, z, carry);
        SET_C_SBC(z80.a, z, carry);

        z80.a = 0xFF & (z80.a - z - carry);
        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SBC_A_r_t(unsigned char n)
{
        PRINT_ASM(n);
        clock_m_tick();
        gpu_m_tick();
        unsigned char carry = GET_BIT(z80.f, CARRY);

        BIT_SET(z80.f, SUBTRACT);
        SET_HC_SBC(z80.a, n, carry);
        SET_C_SBC(z80.a, n, carry);

        z80.a = 0xFF & (z80.a - n - carry);
        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void AND_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        z80.a &= z;

        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, CARRY);

        SET_Z_RES(z80.a);
        BIT_SET(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void AND_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        PRINT_ASM(z);

        clock_m_tick();
        gpu_m_tick();
        z80.a &= z;

        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, CARRY);

        SET_Z_RES(z80.a);
        BIT_SET(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void AND_r_t(unsigned char n)
{
        PRINT_ASM(n);
        clock_m_tick();
        gpu_m_tick();

        z80.a &= n;

        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, CARRY);

        SET_Z_RES(z80.a);
        BIT_SET(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void OR_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        z80.a |= z;

        z80.f &= ~0x7F;
        SET_Z_RES(z80.a);
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void OR_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        PRINT_ASM(z);

        clock_m_tick();
        gpu_m_tick();
        z80.a |= z;

        z80.f &= ~0x7F;
        SET_Z_RES(z80.a);
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void OR_r_t(unsigned char n)
{
        PRINT_ASM(n);
        clock_m_tick();
        gpu_m_tick();

        z80.a |= n;

        z80.f &= ~0x7F;
        SET_Z_RES(z80.a);
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void XOR_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        z80.a ^= z;
        BIT_CLEAR(z80.f, CARRY);
        BIT_CLEAR(z80.f, HALF_CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);

        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void XOR_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        PRINT_ASM(z);

        clock_m_tick();
        gpu_m_tick();
        z80.a ^= z;
        BIT_CLEAR(z80.f, CARRY);
        BIT_CLEAR(z80.f, HALF_CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);

        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void XOR_r_t(unsigned char n)
{
        PRINT_ASM(n);
        clock_m_tick();
        gpu_m_tick();

        z80.a ^= n;
        BIT_CLEAR(z80.f, CARRY);
        BIT_CLEAR(z80.f, HALF_CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);

        SET_Z_RES(z80.a);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void CP_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        PRINT_ASM(z);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        SET_C_SUB(z80.a, z);
        SET_HC_SUB(z80.a, z);
        BIT_SET(z80.f, SUBTRACT);

        z80.a -= z;
        SET_Z_RES(z80.a);
        z80.a += z;

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void CP_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        PRINT_ASM(z);

        clock_m_tick();
        gpu_m_tick();
        SET_C_SUB(z80.a, z);
        SET_HC_SUB(z80.a, z);
        BIT_SET(z80.f, SUBTRACT);

        z80.a -= z;
        SET_Z_RES(z80.a);
        z80.a += z;

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void CP_r_t(unsigned char n)
{
        PRINT_ASM(n);
        clock_m_tick();
        gpu_m_tick();

        SET_C_SUB(z80.a, n);
        SET_HC_SUB(z80.a, n);
        BIT_SET(z80.f, SUBTRACT);

        z80.a -= n;
        SET_Z_RES(z80.a);
        z80.a += n;

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void INC_n_t(unsigned char *n)
{
        PRINT_ASM(*n);
        clock_m_tick();
        gpu_m_tick();

        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_EQUAL(z80.f, HALF_CARRY, ((*n) & 0xF) == (0xF));
        // SET_HC_ADD(*n, 1);
        (*n)++;
        SET_Z_RES(*n);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void INC_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.hl);
        PRINT_ASM(z);

        clock_m_tick();
        gpu_m_tick();
        SET_HC_ADD(z, 1);
        z++;
        wb_cpu(z80.hl, z);
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_Z_RES(z);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void DEC_n_t(unsigned char *n)
{
        PRINT_ASM(*n);
        clock_m_tick();
        gpu_m_tick();
        BIT_SET(z80.f, SUBTRACT);
        BIT_EQUAL(z80.f, HALF_CARRY, ((*n) & 0xF) == 0);
        // SET_HC_SUB(*n, 1);
        (*n)--;
        SET_Z_RES(*n);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void DEC_HL_t()
{
        clock_m_tick();
        gpu_m_tick();
        unsigned char z = rb_cpu(z80.hl);
        PRINT_ASM(z);

        clock_m_tick();
        gpu_m_tick();
        SET_HC_SUB(z, 1);
        z--;
        wb_cpu(z80.hl, z);
        BIT_SET(z80.f, SUBTRACT);
        SET_Z_RES(z);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/* 16-bit ALU */
void ADD_HL_n_t(const unsigned short *n)
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();

        BIT_CLEAR(z80.f, SUBTRACT);
        SET_HC_ADD(z80.l, *n & 0xFF);
        SET_C_ADD(z80.l, *n & 0xFF);
        // z80.l += (*n & 0xFF);

        clock_m_tick();
        gpu_m_tick();
        z = z80.h + (*n >> 8) + GET_BIT(z80.f, CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_HC_ADD_16(z80.hl, *n);
        SET_C_ADD_16(z80.hl, *n);
        // SET_HC_ADD(z80.h, (*n >> 8) + GET_BIT(z80.f, CARRY));
        // SET_C_ADD(z80.h, (*n >> 8) + GET_BIT(z80.f, CARRY));
        // z80.h = z;
        z80.hl += *n;

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void ADD_SP_n_t()
{
        clock_m_tick();
        gpu_m_tick();
        z_s = rb_cpu(z80.pc);
        PRINT_ASM(z_s);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, ZERO);
        SET_HC_ADD(z80.sp, z_s);
        SET_C_ADD(z80.sp, z_s);

        clock_m_tick();
        gpu_m_tick();

        clock_m_tick();
        gpu_m_tick();
        z80.sp += z_s;
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void INC_nn_t(unsigned short *nn)
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        (*nn)++;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void DEC_nn_t(unsigned short *nn)
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        (*nn)--;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/* Misc */
void SWAP_n_t(unsigned char *n)
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char tmp = 0;
        tmp |= ((*n) >> 4) & 0x0F;
        tmp |= ((*n) << 4) & 0xF0;

        *n = tmp;

        BIT_CLEAR(z80.f, CARRY);
        BIT_CLEAR(z80.f, HALF_CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_Z_RES(tmp);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SWAP_HL_t()
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char tmp = 0, val;
        val = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        tmp |= (val >> 4) & 0x0F;
        tmp |= (val << 4) & 0xF0;
        wb_cpu(z80.hl, tmp);
        BIT_CLEAR(z80.f, CARRY);
        BIT_CLEAR(z80.f, HALF_CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);
        SET_Z_RES(tmp);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void DAA_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        if (!GET_BIT(z80.f, SUBTRACT))
        {
                if (GET_BIT(z80.f, CARRY) || z80.a > 0x99)
                {
                        z80.a += 0x60;
                        BIT_SET(z80.f, CARRY);
                }
                if (GET_BIT(z80.f, HALF_CARRY) || (z80.a & 0x0F) > 0x09)
                {
                        z80.a += 0x06;
                }
        }
        else
        {
                if (GET_BIT(z80.f, CARRY))
                {
                        z80.a -= 0x60;
                }
                if (GET_BIT(z80.f, HALF_CARRY))
                {
                        z80.a -= 0x06;
                }
        }

        SET_Z_RES(z80.a);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void CPL_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z80.a = ~z80.a;
        BIT_SET(z80.f, SUBTRACT);
        BIT_SET(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void CCF_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        BIT_FLIP(z80.f, CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SCF_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        BIT_SET(z80.f, CARRY);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void NOP_t()
{
        PRINT_ASM();
        gpu_m_tick();
        clock_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void HALT_t()
{
        PRINT_ASM();
        gpu_m_tick();
        clock_m_tick();
        z80.halt = true;

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void STOP_t()
{
        PRINT_ASM();
        gpu_m_tick();
        clock_m_tick();
        z80.stop = true;

        z80.pc++;
}

void DI_t()
{
        PRINT_ASM();
        gpu_m_tick();
        clock_m_tick();
        z80.ime = false;
        z80.new_ime = false;

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void EI_t()
{
        PRINT_ASM();
        gpu_m_tick();
        clock_m_tick();
        z80.new_ime = true;

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/* Rotates & Shifts */
void RLCA_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z80.a = (unsigned char)((z80.a << 1) | (z80.a >> 7));
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z80.a, 0));
        BIT_CLEAR(z80.f, ZERO);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RLA_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z80.a, 7));
        z80.a = (0xFF & (z80.a << 1)) | old_carry;

        BIT_CLEAR(z80.f, ZERO);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RRCA_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z80.a = (unsigned char)((z80.a >> 1) | (z80.a << 7));
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z80.a, 7));

        BIT_CLEAR(z80.f, ZERO);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RRA_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z80.a, 0));
        z80.a = (unsigned char)((z80.a >> 1) | (old_carry << 7));

        BIT_CLEAR(z80.f, ZERO);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RLC_n_t(unsigned char *n)
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        *n = (unsigned char)((*n << 1) | (*n >> 7));
        BIT_EQUAL(z80.f, CARRY, GET_BIT(*n, 0));

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RLC_HL_t()
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char z = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        z = (unsigned char)((z << 1) | (z >> 7));
        wb_cpu(z80.hl, z);
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z, 0));
        SET_Z_RES(z);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RL_n_t(unsigned char *n)
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        BIT_EQUAL(z80.f, CARRY, GET_BIT(*n, 7));
        *n = (unsigned char)((0xFF & (*n << 1)) | old_carry);

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RL_HL_t()
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        unsigned char z = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z, 7));
        z = (unsigned char)((0xFF & (z << 1)) | old_carry);
        wb_cpu(z80.hl, z);
        SET_Z_RES(z);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RRC_n_t(unsigned char *n)
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        *n = (unsigned char)((*n >> 1) | (*n << 7));
        BIT_EQUAL(z80.f, CARRY, GET_BIT(*n, 7));

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RRC_HL_t()
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char val = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        val = (unsigned char)((val >> 1) | (val << 7));
        BIT_EQUAL(z80.f, CARRY, GET_BIT(val, 7));
        wb_cpu(z80.hl, val);
        SET_Z_RES(val);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RR_n_t(unsigned char *n)
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        BIT_EQUAL(z80.f, CARRY, (*n) & 1);
        *n = (unsigned char)((*n >> 1) | (old_carry << 7));

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RR_HL_t()
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char old_carry = GET_BIT(z80.f, CARRY);
        unsigned char z = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        BIT_EQUAL(z80.f, CARRY, z & 1);
        z = (unsigned char)((z >> 1) | (old_carry << 7));
        wb_cpu(z80.hl, z);
        SET_Z_RES(z);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SLA_n_t(unsigned char *n)
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        BIT_EQUAL(z80.f, CARRY, GET_BIT(*n, 7));
        *n = (unsigned char)(*n << 1);

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SLA_HL_t()
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char z = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z, 7));
        z = (unsigned char)(z << 1);
        wb_cpu(z80.hl, z);
        SET_Z_RES(z);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SRA_n_t(unsigned char *n)
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        BIT_EQUAL(z80.f, CARRY, GET_BIT(*n, 0));
        *n = (unsigned char)(((signed char)*n) >> 1);

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SRA_HL_t()
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char z = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        BIT_EQUAL(z80.f, CARRY, GET_BIT(z, 0));
        z = (unsigned char)(((signed char)z) >> 1);
        wb_cpu(z80.hl, z);
        SET_Z_RES(z);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SRL_n_t(unsigned char *n)
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        BIT_EQUAL(z80.f, CARRY, (*n) & 0x1);
        *n = (unsigned char)((*n) >> 1);

        // assert(GET_BIT(*n, 7) == 0);

        SET_Z_RES(*n);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SRL_HL_t()
{
        PRINT_ASM_CB();
        clock_m_tick();
        gpu_m_tick();
        unsigned char val = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        BIT_EQUAL(z80.f, CARRY, val & 0x1);
        val = (unsigned char)(val >> 1);
        // assert(GET_BIT(val, 7) == 0);
        wb_cpu(z80.hl, val);
        SET_Z_RES(val);
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_CLEAR(z80.f, HALF_CARRY);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/* Bit Opcodes */
void BIT_b_r_t(unsigned char b, unsigned char r)
{
        PRINT_ASM_CB(b);
        clock_m_tick();
        gpu_m_tick();
        BIT_EQUAL(z80.f, ZERO, !BIT_CHECK(r, b));
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_SET(z80.f, HALF_CARRY);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void BIT_b_HL_t(unsigned char b)
{
        PRINT_ASM_CB(b);
        clock_m_tick();
        gpu_m_tick();
        unsigned char r = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        BIT_EQUAL(z80.f, ZERO, !BIT_CHECK(r, b));
        BIT_CLEAR(z80.f, SUBTRACT);
        BIT_SET(z80.f, HALF_CARRY);
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SET_b_r_t(unsigned char b, unsigned char *r)
{
        PRINT_ASM_CB(b);
        clock_m_tick();
        gpu_m_tick();
        BIT_SET(*r, b);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void SET_b_HL_t(unsigned char b)
{
        PRINT_ASM_CB(b);
        clock_m_tick();
        gpu_m_tick();
        unsigned char r = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        BIT_SET(r, b);
        wb_cpu(z80.hl, r);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RES_b_r_t(unsigned char b, unsigned char *r)
{
        PRINT_ASM_CB(b);
        clock_m_tick();
        gpu_m_tick();
        BIT_CLEAR(*r, b);

        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RES_b_HL_t(unsigned char b)
{
        PRINT_ASM_CB(b);
        clock_m_tick();
        gpu_m_tick();
        unsigned char r = rb_cpu(z80.hl);

        clock_m_tick();
        gpu_m_tick();
        BIT_CLEAR(r, b);
        wb_cpu(z80.hl, r);

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/* Jumps */
void JP_nn_t()
{
        PRINT_ASM(rw(z80.pc));
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        z80.pc = (w << 8) | z;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void JP_cc_nn_t(bool cc)
{
        PRINT_ASM(rw(z80.pc));
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.pc);
        z80.pc++;

        if (cc)
        {
                clock_m_tick();
                gpu_m_tick();
                z80.pc = (w << 8) | z;
        }
        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void JP_HL_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.hl);
        z80.pc = z80.hl + 1;
}

void JR_n_t()
{

        clock_m_tick();
        gpu_m_tick();
        z_s = rb_cpu(z80.pc);
        PRINT_ASM(z_s);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        wz = z80.pc + z_s;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(wz);
        z80.pc = wz + 1;
}

void JR_cc_n_t(bool cc)
{
        clock_m_tick();
        gpu_m_tick();
        z_s = rb_cpu(z80.pc);
        PRINT_ASM(z_s);
        z80.pc++;

        if (cc)
        {
                clock_m_tick();
                gpu_m_tick();
                wz = z80.pc + z_s;

                clock_m_tick();
                gpu_m_tick();
                z80.ir = rb_cpu(wz);
                z80.pc = wz + 1;
        }
        else
        {
                clock_m_tick();
                gpu_m_tick();
                z80.ir = rb_cpu(z80.pc);
                z80.pc++;
        }
}

/* Calls */
void CALL_nn_t()
{
        PRINT_ASM(rw(z80.pc));
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        z80.sp--;

        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.sp, z80.pc >> 8);
        z80.sp--;

        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.sp, z80.pc & 0xFF);
        z80.pc = (w << 8) | z;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void CALL_cc_nn_t(bool cc)
{
        PRINT_ASM(rw(z80.pc));
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.pc);
        z80.pc++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.pc);
        z80.pc++;

        if (cc)
        {
                clock_m_tick();
                gpu_m_tick();
                z80.sp--;

                clock_m_tick();
                gpu_m_tick();
                wb_cpu(z80.sp, z80.pc >> 8);
                z80.sp--;

                clock_m_tick();
                gpu_m_tick();
                wb_cpu(z80.sp, z80.pc & 0xFF);
                z80.pc = (w << 8) | z;
        }

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/* Restarts */
void RST_n_t(unsigned char n)
{
        PRINT_ASM(n);
        clock_m_tick();
        gpu_m_tick();
        z80.sp--;

        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.sp, z80.pc >> 8);
        z80.sp--;

        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.sp, z80.pc & 0xFF);
        z80.pc = n;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RST_IRQ_n_t(unsigned char irq)
{
#ifdef DEBUG
        printf("0x%04x RST %02x\n", z80.pc, irq);
#endif
        clock_m_tick();
        gpu_m_tick();
        wb_cpu(z80.sp, z80.pc & 0xFF);
        z80.pc = irq;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

/* Returns */
void RET_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.sp);
        z80.sp++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.sp);
        z80.sp++;

        clock_m_tick();
        gpu_m_tick();
        z80.pc = (w << 8) | z;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RET_cc_t(bool cc)
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        if (cc)
        {
                clock_m_tick();
                gpu_m_tick();
                z = rb_cpu(z80.sp);
                z80.sp++;

                clock_m_tick();
                gpu_m_tick();
                w = rb_cpu(z80.sp);
                z80.sp++;

                clock_m_tick();
                gpu_m_tick();
                z80.pc = (w << 8) | z;
        }
        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

void RETI_t()
{
        PRINT_ASM();
        clock_m_tick();
        gpu_m_tick();
        z = rb_cpu(z80.sp);
        z80.sp++;

        clock_m_tick();
        gpu_m_tick();
        w = rb_cpu(z80.sp);
        z80.sp++;

        clock_m_tick();
        gpu_m_tick();
        z80.pc = (w << 8) | z;
        z80.ime = true;

        clock_m_tick();
        gpu_m_tick();
        z80.ir = rb_cpu(z80.pc);
        z80.pc++;
}

static bool (*table_cc[])(void) = {
    cc_nz, cc_z, cc_nc, cc_c};

static void (*table_alu_r_t[])(unsigned char) = {
    ADD_A_r_t, ADC_A_r_t, SUB_A_r_t, SBC_A_r_t, AND_r_t, XOR_r_t, OR_r_t, CP_r_t};

static void (*table_alu_n_t[])(void) = {
    ADD_A_n_t, ADC_A_n_t, SUB_A_n_t, SBC_A_n_t, AND_n_t, XOR_n_t, OR_n_t, CP_n_t};

static void (*table_alu_hl_t[])(void) = {
    ADD_A_HL_t, ADC_A_HL_t, SUB_A_HL_t, SBC_A_HL_t, AND_HL_t, XOR_HL_t, OR_HL_t, CP_HL_t};

static void (*table_rot_t[])(unsigned char *) = {
    RLC_n_t, RRC_n_t, RL_n_t, RR_n_t, SLA_n_t, SRA_n_t, SWAP_n_t, SRL_n_t};
static void (*table_rot_HL_t[])(void) = {
    RLC_HL_t, RRC_HL_t, RL_HL_t, RR_HL_t, SLA_HL_t, SRA_HL_t, SWAP_HL_t, SRL_HL_t};

bool cc_nz()
{
        return !BIT_CHECK(z80.f, ZERO);
}

bool cc_z()
{
        return BIT_CHECK(z80.f, ZERO);
}

bool cc_nc()
{
        return !BIT_CHECK(z80.f, CARRY);
}

bool cc_c()
{
        return BIT_CHECK(z80.f, CARRY);
}

static unsigned char *table_r[] = {
    &z80.b, &z80.c, &z80.d, &z80.e, &z80.h, &z80.l, NULL, &z80.a};

static unsigned short *table_rp[] = {
    &z80.bc, &z80.de, &z80.hl, &z80.sp};

static unsigned short *table_rp2[] = {
    &z80.bc, &z80.de, &z80.hl, &z80.af};
