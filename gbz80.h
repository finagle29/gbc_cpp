/* Header file for GB subset of Z80 processor */

#ifndef Z80_H_
#define Z80_H_

#include <stdbool.h>
#include <stdlib.h>


#define BIT_SET(a, b) ((a) |= (1ULL << (b)))
#define BIT_CLEAR(a, b) ((a) &= ~(1ULL << (b)))
#define BIT_FLIP(a, b) ((a) ^= (1ULL << (b)))
#define BIT_CHECK(a, b) ((a) & (1ULL << (b)))

#define GET_BIT(a, b) (((a) >> (b)) & 1ULL)
#define BIT_EQUAL(a, n, x) (((a) ^= (-(unsigned long)(x) ^ (a)) & (1ULL << (n))))

#define CARRY 4
#define HALF_CARRY 5
#define SUBTRACT 6
#define ZERO 7

#define SET_HC_ADD(a, b) BIT_EQUAL(z80.f, HALF_CARRY, (((a) & 0xF) + ((b) & 0xF)) >> 4)
#define SET_C_ADD(a, b) BIT_EQUAL(z80.f, CARRY, ((unsigned long)((a) & 0xFF) + (unsigned long)((b) & 0xFF)) > 0xFF)
#define SET_Z_RES(a) BIT_EQUAL(z80.f, ZERO, (a) == 0)

#define SET_C_SUB(a, b) BIT_EQUAL(z80.f, CARRY, b > a)
#define SET_HC_SUB(a, b) BIT_EQUAL(z80.f, HALF_CARRY, ((a) & 0xF) < ((b) & 0xF))

#define SET_HC_ADD_16(a, b) BIT_EQUAL(z80.f, HALF_CARRY, (((a) & 0xFFF) + ((b) & 0xFFF)) >> 12)
#define SET_C_ADD_16(a, b) BIT_EQUAL(z80.f, CARRY, ((unsigned long)(a) + (unsigned long)(b)) > 0xFFFF)

#ifdef DEBUG
        #define PRINT_ASM(...) if (!mmu->inbios) printf(op_table[op], z80.pc - 1, ##__VA_ARGS__)
        #define PRINT_ASM_CB(...) if (!mmu->inbios) printf(cb_op_table[op], z80.pc - 1, ##__VA_ARGS__)
#else
        #define PRINT_ASM(...)
        #define PRINT_ASM_CB(...)
#endif

void init_z80(void);
void reset(void);
int fetch_dispatch_execute(void);
void fetch_dispatch_execute_loop(void);
void scramble_z80(void);
void update_clock(void);
void check_timer(void);

typedef struct {
        unsigned short m; // 2 bytes
        unsigned char main, sub; // 2 bytes
        unsigned char div, div_c, tima, tma, tac, old_tac; // 6 bytes
} clock_type; // 10 bytes, aligned to 2

typedef struct {
        clock_type clock; // 10 bytes aligned to 2
        struct {
                union {
                        struct {
                                unsigned char f;
                                unsigned char a;
                        };
                        unsigned short af;
                };

                union {
                        struct {
                                unsigned char c;
                                unsigned char b;
                        };
                        unsigned short bc;
                };

                union {
                        struct {
                                unsigned char e;
                                unsigned char d;
                        };
                        unsigned short de;
                };

                union {
                        struct {
                                unsigned char l;
                                unsigned char h;
                        };
                        unsigned short hl;
                };

                unsigned short sp;
                unsigned short pc;
                unsigned char m;
                unsigned char t;

        }; // 14 bytes

        bool halt, stop, ime, new_ime; // 4 bytes
        unsigned char int_f, int_en; // 2 bytes aka IF and IE
        // 1 byte of padding
} gbz80_type;

extern gbz80_type z80;
extern gbz80_type *z80_p;

/* Function prototypes */

/* 8-bit loads */
void LD_nn_n(unsigned char *r, unsigned char n);
void LD_HL_n(unsigned char n);
void LD_r1_r2(unsigned char *r1, unsigned char r2, unsigned char m_time);
void LD_HL_r2(unsigned char r2);
void LD_A_n(unsigned char *n, unsigned char m_time);
void LD_A_rb(unsigned short nn, unsigned char m_time);
void LD_n_A(unsigned char *n, unsigned char m_time);
void LD_wb_A(unsigned short nn, unsigned char m_time);
void LD_A_C(void);
void LD_C_A(void);
void LDD_A_HL(void);
void LDD_HL_A(void);
void LDI_A_HL(void);
void LDI_HL_A(void);
void LDH_n_A(unsigned char n);
void LDH_A_n(unsigned char n);

/* 16-bit loads */
void LD_n_nn(unsigned short *n, unsigned short nn);
void LD_SP_HL(void);
void LDHL_SP_n(signed char n);
void LD_nn_SP(unsigned short nn);
void PUSH_nn(const unsigned short *nn);
void POP_nn(unsigned short *nn);

/* 8-bit ALU */
void ADD_A_n(unsigned char n, unsigned char m_time);
void ADC_A_n(unsigned char n, unsigned char m_time);
void SUB_A_n(unsigned char n, unsigned char m_time);
void SBC_A_n(unsigned char n, unsigned char m_time);
void AND_n(unsigned char n, unsigned char m_time);
void OR_n(unsigned char n, unsigned char m_time);
void XOR_n(unsigned char n, unsigned char m_time);
void CP_n(unsigned char n, unsigned char m_time);
void INC_n(unsigned char *n);
void INC_HL(void);
void DEC_n(unsigned char *n);
void DEC_HL(void);

/* 16-bit ALU */
void ADD_HL_n(const unsigned short *n);
void ADD_SP_n(signed char n);
void INC_nn(unsigned short *nn);
void DEC_nn(unsigned short *nn);

/* Misc */
void SWAP_n(unsigned char *n);
void SWAP_HL(void);
void DAA(void);
void CPL(void);
void CCF(void);
void SCF(void);
void NOP(void);
void HALT(void);
void STOP(void);
void DI(void);
void EI(void);

/* Rotates & Shifts */
void RLCA(void);
void RLA(void);
void RRCA(void);
void RRA(void);
void RLC_n(unsigned char *n);
void RLC_HL(void);
void RL_n(unsigned char *n);
void RL_HL(void);
void RRC_n(unsigned char *n);
void RRC_HL(void);
void RR_n(unsigned char *n);
void RR_HL(void);
void SLA_n(unsigned char *n);
void SLA_HL(void);
void SRA_n(unsigned char *n);
void SRA_HL(void);
void SRL_n(unsigned char *n);
void SRL_HL(void);

/* Bit Opcodes */
void BIT_b_r(unsigned char b, unsigned char r);
void BIT_b_HL(unsigned char b);
void SET_b_r(unsigned char b, unsigned char *r);
void SET_b_HL(unsigned char b);
void RES_b_r(unsigned char b, unsigned char *r);
void RES_b_HL(unsigned char b);

/* Jumps */
void JP_nn(unsigned short nn);
void JP_cc_nn(bool cc, unsigned short nn);
void JP_HL(void);
void JR_n(signed char n);
void JR_cc_n(bool cc, signed char n);

/* Calls */
void CALL_nn(unsigned short nn);
void CALL_cc_nn(bool cc, unsigned short nn);

/* Restarts */
void RST_n(unsigned char n);

void RST40(void);
void RST48(void);
void RST50(void);
void RST58(void);
void RST60(void);

/* Returns */
void RET(void);
void RET_cc(bool cc);
void RETI(void);


bool cc_nz(void);
bool cc_z(void);
bool cc_nc(void);
bool cc_c(void);

static unsigned char* table_r[8];
static unsigned short* table_rp[4];
static unsigned short* table_rp2[4];

static bool (*table_cc[4])(void);
static void (*table_alu[8])(unsigned char, unsigned char);
static void (*table_rot[8])(unsigned char*);
static void (*table_rot_HL[8])(void);

static const char op_table[0x100][30] = {
        "0x%04x NOP\n",
        "0x%04x LD BC, 0x%04x\n",
        "0x%04x LD (BC), A\n",
        "0x%04x INC BC\n",
        "0x%04x INC B 0x%02x\n",
        "0x%04x DEC B 0x%02x\n",
        "0x%04x LD B, 0x%02x\n",
        "0x%04x RLCA\n",
        "0x%04x LD (0x%04x), SP\n",
        "0x%04x ADD HL, BC\n",
        "0x%04x LD A, (BC)\n",
        "0x%04x DEC BC\n",
        "0x%04x INC C 0x%02x\n",
        "0x%04x DEC C 0x%02x\n",
        "0x%04x LD C, %02x\n",
        "0x%04x RRCA\n",
        "0x%04x STOP 0\n",
        "0x%04x LD DE, %04x\n",
        "0x%04x LD (DE), A\n",
        "0x%04x INC DE\n",
        "0x%04x INC D 0x%02x\n",
        "0x%04x DEC D\n",
        "0x%04x LD D, %02x\n",
        "0x%04x RLA\n",
        "0x%04x JR %02x\n",
        "0x%04x ADD HL, DE\n",
        "0x%04x LD A, (DE)\n",
        "0x%04x DEC DE\n",
        "0x%04x INC E 0x%02x\n",
        "0x%04x DEC E\n",
        "0x%04x LD E, %02x\n",
        "0x%04x RRA\n",
        "0x%04x JR NZ, %02x\n",
        "0x%04x LD HL, %04x\n",
        "0x%04x LD (HL+), A\n",
        "0x%04x INC HL\n",
        "0x%04x INC H\n",
        "0x%04x DEC H\n",
        "0x%04x LD H, %02x\n",
        "0x%04x DAA\n",
        "0x%04x JR Z, %02x\n",
        "0x%04x ADD HL, HL\n",
        "0x%04x LD A, (HL+)\n",
        "0x%04x DEC HL\n",
        "0x%04x INC L\n",
        "0x%04x DEC L\n",
        "0x%04x LD L, %02x\n",
        "0x%04x CPL\n",
        "0x%04x JR NC, %02x\n",
        "0x%04x LD SP, %04x\n",
        "0x%04x LD (HL-), A\n",
        "0x%04x INC SP\n",
        "0x%04x INC (HL)\n",
        "0x%04x DEC (HL)\n",
        "0x%04x LD (HL), %02x\n",
        "0x%04x SCF\n",
        "0x%04x JR C, %02x\n",
        "0x%04x ADD HL, SP\n",
        "0x%04x LD A, (HL-)\n",
        "0x%04x DEC SP\n",
        "0x%04x INC A\n",
        "0x%04x DEC A\n",
        "0x%04x LD A, %02x\n",
        "0x%04x CCF\n",
        "0x%04x LD B, B\n",
        "0x%04x LD B, C\n",
        "0x%04x LD B, D\n",
        "0x%04x LD B, E\n",
        "0x%04x LD B, H\n",
        "0x%04x LD B, L\n",
        "0x%04x LD B, (HL)\n",
        "0x%04x LD B, A\n",
        "0x%04x LD C, B\n",
        "0x%04x LD C, C\n",
        "0x%04x LD C, D\n",
        "0x%04x LD C, E\n",
        "0x%04x LD C, H\n",
        "0x%04x LD C, L\n",
        "0x%04x LD C, (HL)\n",
        "0x%04x LD C, A\n",
        "0x%04x LD D, B\n",
        "0x%04x LD D, C\n",
        "0x%04x LD D, D\n",
        "0x%04x LD D, E\n",
        "0x%04x LD D, H\n",
        "0x%04x LD D, L\n",
        "0x%04x LD D, (HL)\n",
        "0x%04x LD D, A\n",
        "0x%04x LD E, B\n",
        "0x%04x LD E, C\n",
        "0x%04x LD E, D\n",
        "0x%04x LD E, E\n",
        "0x%04x LD E, H\n",
        "0x%04x LD E, L\n",
        "0x%04x LD E, (HL)\n",
        "0x%04x LD E, A\n",
        "0x%04x LD H, B\n",
        "0x%04x LD H, C\n",
        "0x%04x LD H, D\n",
        "0x%04x LD H, E\n",
        "0x%04x LD H, H\n",
        "0x%04x LD H, L\n",
        "0x%04x LD H, (HL)\n",
        "0x%04x LD H, A\n",
        "0x%04x LD L, B\n",
        "0x%04x LD L, C\n",
        "0x%04x LD L, D\n",
        "0x%04x LD L, E\n",
        "0x%04x LD L, H\n",
        "0x%04x LD L, L\n",
        "0x%04x LD L, (HL)\n",
        "0x%04x LD L, A\n",
        "0x%04x LD (HL), B\n",
        "0x%04x LD (HL), C\n",
        "0x%04x LD (HL), D\n",
        "0x%04x LD (HL), E\n",
        "0x%04x LD (HL), H\n",
        "0x%04x LD (HL), L\n",
        "0x%04x HALT\n",
        "0x%04x LD (HL), A\n",
        "0x%04x LD A, B\n",
        "0x%04x LD A, C\n",
        "0x%04x LD A, D\n",
        "0x%04x LD A, E\n",
        "0x%04x LD A, H\n",
        "0x%04x LD A, L\n",
        "0x%04x LD A, (HL)\n",
        "0x%04x LD A, A\n",
        "0x%04x ADD A, B\n",
        "0x%04x ADD A, C\n",
        "0x%04x ADD A, D\n",
        "0x%04x ADD A, E\n",
        "0x%04x ADD A, H\n",
        "0x%04x ADD A, L\n",
        "0x%04x ADD A, (HL)\n",
        "0x%04x ADD A, A\n",
        "0x%04x ADC A, B\n",
        "0x%04x ADC A, C\n",
        "0x%04x ADC A, D\n",
        "0x%04x ADC A, E\n",
        "0x%04x ADC A, H\n",
        "0x%04x ADC A, L\n",
        "0x%04x ADC A, (HL)\n",
        "0x%04x ADC A, A\n",
        "0x%04x SUB A, B\n",
        "0x%04x SUB A, C\n",
        "0x%04x SUB A, D\n",
        "0x%04x SUB A, E\n",
        "0x%04x SUB A, H\n",
        "0x%04x SUB A, L\n",
        "0x%04x SUB A, (HL)\n",
        "0x%04x SUB A, A\n",
        "0x%04x SBC A, B\n",
        "0x%04x SBC A, C\n",
        "0x%04x SBC A, D\n",
        "0x%04x SBC A, E\n",
        "0x%04x SBC A, H\n",
        "0x%04x SBC A, L\n",
        "0x%04x SBC A, (HL)\n",
        "0x%04x SBC A, A\n",
        "0x%04x AND B\n",
        "0x%04x AND C\n",
        "0x%04x AND D\n",
        "0x%04x AND E\n",
        "0x%04x AND H\n",
        "0x%04x AND L\n",
        "0x%04x AND (HL)\n",
        "0x%04x AND A\n",
        "0x%04x XOR B\n",
        "0x%04x XOR C\n",
        "0x%04x XOR D\n",
        "0x%04x XOR E\n",
        "0x%04x XOR H\n",
        "0x%04x XOR L\n",
        "0x%04x XOR (HL)\n",
        "0x%04x XOR A\n",
        "0x%04x OR B\n",
        "0x%04x OR C\n",
        "0x%04x OR D\n",
        "0x%04x OR E\n",
        "0x%04x OR H\n",
        "0x%04x OR L\n",
        "0x%04x OR (HL)\n",
        "0x%04x OR A\n",
        "0x%04x CP B\n",
        "0x%04x CP C\n",
        "0x%04x CP D\n",
        "0x%04x CP E\n",
        "0x%04x CP H\n",
        "0x%04x CP L\n",
        "0x%04x CP (HL)\n",
        "0x%04x CP A\n",
        "0x%04x RET NZ\n",
        "0x%04x POP BC\n",
        "0x%04x JP NZ, %04x\n",
        "0x%04x JP %04x\n",
        "0x%04x CALL NZ, %04x\n",
        "0x%04x PUSH BC\n",
        "0x%04x ADD A, %02x\n",
        "0x%04x RST 00\n",
        "0x%04x RET Z\n",
        "0x%04x RET\n",
        "0x%04x JP Z, %04x\n",
        "0x%04x \n",
        "0x%04x CALL Z, %04x\n",
        "0x%04x CALL %04x\n",
        "0x%04x ADC A, %02x\n",
        "0x%04x RST 08\n",
        "0x%04x RET NC\n",
        "0x%04x POP DE\n",
        "0x%04x JP NC, %04x\n",
        "0x%04x \n",
        "0x%04x CALL NC, %04x\n",
        "0x%04x PUSH DE\n",
        "0x%04x SUB %02x\n",
        "0x%04x RST 10\n",
        "0x%04x RET C\n",
        "0x%04x RETI\n",
        "0x%04x JP C, %04x\n",
        "0x%04x \n",
        "0x%04x CALL C, %04x\n",
        "0x%04x \n",
        "0x%04x SBC A, %02x\n",
        "0x%04x RST 18\n",
        "0x%04x LDH (%02x), A\t%02x\n",
        "0x%04x POP HL\n",
        "0x%04x LD (C), A\n",
        "0x%04x \n",
        "0x%04x \n",
        "0x%04x PUSH HL\n",
        "0x%04x AND %02x\n",
        "0x%04x RST 20\n",
        "0x%04x ADD SP, %02x\n",
        "0x%04x JP (HL)\n",
        "0x%04x LD (%04x), A\n",
        "0x%04x \n",
        "0x%04x \n",
        "0x%04x \n",
        "0x%04x XOR %02x\n",
        "0x%04x RST 28\n",
        "0x%04x LDH A, (%02x)\t%02x\n",
        "0x%04x POP AF\n",
        "0x%04x LD A, (C)\n",
        "0x%04x DI\n",
        "0x%04x \n",
        "0x%04x PUSH AF\n",
        "0x%04x OR %02x\n",
        "0x%04x RST 30\n",
        "0x%04x LD HL, SP+%02x\n",
        "0x%04x LD SP, HL\n",
        "0x%04x LD A, (%04x)\n",
        "0x%04x EI\n",
        "0x%04x \n",
        "0x%04x \n",
        "0x%04x CP %02x\n",
        "0x%04x RST 38\n"
};

static const char cb_op_table[256][19] = {
        "0x%04x RLC B\n",
        "0x%04x RLC C\n",
        "0x%04x RLC D\n",
        "0x%04x RLC E\n",
        "0x%04x RLC H\n",
        "0x%04x RLC L\n",
        "0x%04x RLC (HL)\n",
        "0x%04x RLC A\n",
        "0x%04x RRC B\n",
        "0x%04x RRC C\n",
        "0x%04x RRC D\n",
        "0x%04x RRC E\n",
        "0x%04x RRC H\n",
        "0x%04x RRC L\n",
        "0x%04x RRC (HL)\n",
        "0x%04x RRC A\n",
        "0x%04x RL B\n",
        "0x%04x RL C\n",
        "0x%04x RL D\n",
        "0x%04x RL E\n",
        "0x%04x RL H\n",
        "0x%04x RL L\n",
        "0x%04x RL (HL)\n",
        "0x%04x RL A\n",
        "0x%04x RR B\n",
        "0x%04x RR C\n",
        "0x%04x RR D\n",
        "0x%04x RR E\n",
        "0x%04x RR H\n",
        "0x%04x RR L\n",
        "0x%04x RR (HL)\n",
        "0x%04x RR A\n",
        "0x%04x SLA B\n",
        "0x%04x SLA C\n",
        "0x%04x SLA D\n",
        "0x%04x SLA E\n",
        "0x%04x SLA H\n",
        "0x%04x SLA L\n",
        "0x%04x SLA (HL)\n",
        "0x%04x SLA A\n",
        "0x%04x SRA B\n",
        "0x%04x SRA C\n",
        "0x%04x SRA D\n",
        "0x%04x SRA E\n",
        "0x%04x SRA H\n",
        "0x%04x SRA L\n",
        "0x%04x SRA (HL)\n",
        "0x%04x SRA A\n",
        "0x%04x SWAP B\n",
        "0x%04x SWAP C\n",
        "0x%04x SWAP D\n",
        "0x%04x SWAP E\n",
        "0x%04x SWAP H\n",
        "0x%04x SWAP L\n",
        "0x%04x SWAP (HL)\n",
        "0x%04x SWAP A\n",
        "0x%04x SRL B\n",
        "0x%04x SRL C\n",
        "0x%04x SRL D\n",
        "0x%04x SRL E\n",
        "0x%04x SRL H\n",
        "0x%04x SRL L\n",
        "0x%04x SRL (HL)\n",
        "0x%04x SRL A\n",
        "0x%04x BIT 0, B\n",
        "0x%04x BIT 0, C\n",
        "0x%04x BIT 0, D\n",
        "0x%04x BIT 0, E\n",
        "0x%04x BIT 0, H\n",
        "0x%04x BIT 0, L\n",
        "0x%04x BIT 0, (HL)\n",
        "0x%04x BIT 0, A\n",
        "0x%04x BIT 1, B\n",
        "0x%04x BIT 1, C\n",
        "0x%04x BIT 1, D\n",
        "0x%04x BIT 1, E\n",
        "0x%04x BIT 1, H\n",
        "0x%04x BIT 1, L\n",
        "0x%04x BIT 1, (HL)\n",
        "0x%04x BIT 1, A\n",
        "0x%04x BIT 2, B\n",
        "0x%04x BIT 2, C\n",
        "0x%04x BIT 2, D\n",
        "0x%04x BIT 2, E\n",
        "0x%04x BIT 2, H\n",
        "0x%04x BIT 2, L\n",
        "0x%04x BIT 2, (HL)\n",
        "0x%04x BIT 2, A\n",
        "0x%04x BIT 3, B\n",
        "0x%04x BIT 3, C\n",
        "0x%04x BIT 3, D\n",
        "0x%04x BIT 3, E\n",
        "0x%04x BIT 3, H\n",
        "0x%04x BIT 3, L\n",
        "0x%04x BIT 3, (HL)\n",
        "0x%04x BIT 3, A\n",
        "0x%04x BIT 4, B\n",
        "0x%04x BIT 4, C\n",
        "0x%04x BIT 4, D\n",
        "0x%04x BIT 4, E\n",
        "0x%04x BIT 4, H\n",
        "0x%04x BIT 4, L\n",
        "0x%04x BIT 4, (HL)\n",
        "0x%04x BIT 4, A\n",
        "0x%04x BIT 5, B\n",
        "0x%04x BIT 5, C\n",
        "0x%04x BIT 5, D\n",
        "0x%04x BIT 5, E\n",
        "0x%04x BIT 5, H\n",
        "0x%04x BIT 5, L\n",
        "0x%04x BIT 5, (HL)\n",
        "0x%04x BIT 5, A\n",
        "0x%04x BIT 6, B\n",
        "0x%04x BIT 6, C\n",
        "0x%04x BIT 6, D\n",
        "0x%04x BIT 6, E\n",
        "0x%04x BIT 6, H\n",
        "0x%04x BIT 6, L\n",
        "0x%04x BIT 6, (HL)\n",
        "0x%04x BIT 6, A\n",
        "0x%04x BIT 7, B\n",
        "0x%04x BIT 7, C\n",
        "0x%04x BIT 7, D\n",
        "0x%04x BIT 7, E\n",
        "0x%04x BIT 7, H\n",
        "0x%04x BIT 7, L\n",
        "0x%04x BIT 7, (HL)\n",
        "0x%04x BIT 7, A\n",
        "0x%04x RES 0, B\n",
        "0x%04x RES 0, C\n",
        "0x%04x RES 0, D\n",
        "0x%04x RES 0, E\n",
        "0x%04x RES 0, H\n",
        "0x%04x RES 0, L\n",
        "0x%04x RES 0, (HL)\n",
        "0x%04x RES 0, A\n",
        "0x%04x RES 1, B\n",
        "0x%04x RES 1, C\n",
        "0x%04x RES 1, D\n",
        "0x%04x RES 1, E\n",
        "0x%04x RES 1, H\n",
        "0x%04x RES 1, L\n",
        "0x%04x RES 1, (HL)\n",
        "0x%04x RES 1, A\n",
        "0x%04x RES 2, B\n",
        "0x%04x RES 2, C\n",
        "0x%04x RES 2, D\n",
        "0x%04x RES 2, E\n",
        "0x%04x RES 2, H\n",
        "0x%04x RES 2, L\n",
        "0x%04x RES 2, (HL)\n",
        "0x%04x RES 2, A\n",
        "0x%04x RES 3, B\n",
        "0x%04x RES 3, C\n",
        "0x%04x RES 3, D\n",
        "0x%04x RES 3, E\n",
        "0x%04x RES 3, H\n",
        "0x%04x RES 3, L\n",
        "0x%04x RES 3, (HL)\n",
        "0x%04x RES 3, A\n",
        "0x%04x RES 4, B\n",
        "0x%04x RES 4, C\n",
        "0x%04x RES 4, D\n",
        "0x%04x RES 4, E\n",
        "0x%04x RES 4, H\n",
        "0x%04x RES 4, L\n",
        "0x%04x RES 4, (HL)\n",
        "0x%04x RES 4, A\n",
        "0x%04x RES 5, B\n",
        "0x%04x RES 5, C\n",
        "0x%04x RES 5, D\n",
        "0x%04x RES 5, E\n",
        "0x%04x RES 5, H\n",
        "0x%04x RES 5, L\n",
        "0x%04x RES 5, (HL)\n",
        "0x%04x RES 5, A\n",
        "0x%04x RES 6, B\n",
        "0x%04x RES 6, C\n",
        "0x%04x RES 6, D\n",
        "0x%04x RES 6, E\n",
        "0x%04x RES 6, H\n",
        "0x%04x RES 6, L\n",
        "0x%04x RES 6, (HL)\n",
        "0x%04x RES 6, A\n",
        "0x%04x RES 7, B\n",
        "0x%04x RES 7, C\n",
        "0x%04x RES 7, D\n",
        "0x%04x RES 7, E\n",
        "0x%04x RES 7, H\n",
        "0x%04x RES 7, L\n",
        "0x%04x RES 7, (HL)\n",
        "0x%04x RES 7, A\n",
        "0x%04x SET 0, B\n",
        "0x%04x SET 0, C\n",
        "0x%04x SET 0, D\n",
        "0x%04x SET 0, E\n",
        "0x%04x SET 0, H\n",
        "0x%04x SET 0, L\n",
        "0x%04x SET 0, (HL)\n",
        "0x%04x SET 0, A\n",
        "0x%04x SET 1, B\n",
        "0x%04x SET 1, C\n",
        "0x%04x SET 1, D\n",
        "0x%04x SET 1, E\n",
        "0x%04x SET 1, H\n",
        "0x%04x SET 1, L\n",
        "0x%04x SET 1, (HL)\n",
        "0x%04x SET 1, A\n",
        "0x%04x SET 2, B\n",
        "0x%04x SET 2, C\n",
        "0x%04x SET 2, D\n",
        "0x%04x SET 2, E\n",
        "0x%04x SET 2, H\n",
        "0x%04x SET 2, L\n",
        "0x%04x SET 2, (HL)\n",
        "0x%04x SET 2, A\n",
        "0x%04x SET 3, B\n",
        "0x%04x SET 3, C\n",
        "0x%04x SET 3, D\n",
        "0x%04x SET 3, E\n",
        "0x%04x SET 3, H\n",
        "0x%04x SET 3, L\n",
        "0x%04x SET 3, (HL)\n",
        "0x%04x SET 3, A\n",
        "0x%04x SET 4, B\n",
        "0x%04x SET 4, C\n",
        "0x%04x SET 4, D\n",
        "0x%04x SET 4, E\n",
        "0x%04x SET 4, H\n",
        "0x%04x SET 4, L\n",
        "0x%04x SET 4, (HL)\n",
        "0x%04x SET 4, A\n",
        "0x%04x SET 5, B\n",
        "0x%04x SET 5, C\n",
        "0x%04x SET 5, D\n",
        "0x%04x SET 5, E\n",
        "0x%04x SET 5, H\n",
        "0x%04x SET 5, L\n",
        "0x%04x SET 5, (HL)\n",
        "0x%04x SET 5, A\n",
        "0x%04x SET 6, B\n",
        "0x%04x SET 6, C\n",
        "0x%04x SET 6, D\n",
        "0x%04x SET 6, E\n",
        "0x%04x SET 6, H\n",
        "0x%04x SET 6, L\n",
        "0x%04x SET 6, (HL)\n",
        "0x%04x SET 6, A\n",
        "0x%04x SET 7, B\n",
        "0x%04x SET 7, C\n",
        "0x%04x SET 7, D\n",
        "0x%04x SET 7, E\n",
        "0x%04x SET 7, H\n",
        "0x%04x SET 7, L\n",
        "0x%04x SET 7, (HL)\n",
        "0x%04x SET 7, A\n"
};

#endif
