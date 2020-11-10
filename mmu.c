#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "gbz80.h"
#include "mmu.h"
#include "gpu.h"
#include "keys.h"
#include "apu.h"
#include "gb_apu/GBAPU_Wrapper.h"


mmu_type *mmu = NULL;

char save_fname[50];


void init_mmu() {

        unsigned short addr = 0;
        for (addr = 0; addr < 0x8000; addr++) {
                wb(addr, 0);
        }
}

void print_n_bytes(unsigned short n) {
        unsigned short i = 0;
        while (i < n) {
                printf("%x\n", rb(i++));
        }
}

void save_sram() {
        printf("saving to %s\n", save_fname);
        FILE *f = fopen(save_fname, "w+b");
        fwrite(mmu->eram, sizeof(unsigned char), sizeof(mmu->eram), f);
        fclose(f);
}

void load_bios(char* fname) {
        FILE *f = fopen(fname, "rb");
        fread(mmu->bios, sizeof(unsigned char), 0x100, f);
        fclose(f);
}

void load_rom(char* fname) {
        char fname_copy[200];
        strcpy(fname_copy, fname);
        char *base_name = strtok(fname_copy, ".");
        if (base_name != NULL) {
                snprintf(save_fname, 50, "%s.sav", base_name);
        }
        unsigned char cart_type;

        FILE *f = fopen(fname, "rb");
        unsigned long rom_size;

        fseek(f, 0, SEEK_END);
        rom_size = (unsigned long)ftell(f);
        printf("ROM SIZE: %lu\n", rom_size);
        mmu = (mmu_type *)malloc(sizeof(mmu_type) + rom_size);
        
        fseek(f, 0x147, SEEK_SET);
        switch (cart_type = fgetc(f))
        {
                case 0x00: case 0x01: case 0x02: case 0x03:
                case 0x08: case 0x09:
                        mmu->mbc = 1;
                        break;
                case 0x05: case 0x06:
                        mmu->mbc = 2;
                        break;
                case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
                        mmu->mbc = 3;
                        break;
                default:
                        mmu->mbc = 1;
        }
        printf("using mbc%d\n", mmu->mbc);

        fseek(f, 0x148, SEEK_SET);
        unsigned char rom_size_byte = fgetc(f);
        printf("ROM SIZE byte: %d", rom_size_byte);
        if (rom_size_byte < 0x52) {
                mmu->rom_banks = 1 << (2 * rom_size_byte);
        } else {
                mmu->rom_banks = 72 + 4 * (rom_size_byte - 0x52) * (rom_size_byte - 0x52 + 1);
        }

        fseek(f, 0x149, SEEK_SET);
        unsigned char ram_size_byte = fgetc(f);
        switch (ram_size_byte) {
                case 0x00: case 0x01: case 0x02:
                        mmu->ram_banks = 1;
                        break;
                case 0x03:
                        mmu->ram_banks = 4;
                        break;
                case 0x04:
                        mmu->ram_banks = 16;
                        break;
                case 0x05:
                        mmu->ram_banks = 8;
                        break;

        }
        rewind(f);

        mmu->mode = 0;
        
        if (mmu == NULL) {
                fprintf(stderr, "Error using malloc for mmu.");
                fclose(f);
                exit(1);
        }


        fseek(f, 0, SEEK_SET);

        fread(mmu->rom, sizeof(unsigned char), rom_size, f);
        fclose(f);

        struct stat buffer;
        if (stat(save_fname, &buffer) == 0) {
                printf("reading save file %s ...\n", save_fname);
                f = fopen(save_fname, "rb");
                size_t result = fread(mmu->eram, sizeof(unsigned char), sizeof(mmu->eram), f);
                printf("read %lu bytes\n", result);
                fclose(f);
        } else {
                printf("save file %s does not exist\n", save_fname);
                memset(mmu->eram, 0, 0x20000);
        }

        mmu->rom_bank = 1;
        mmu->ram_bank = 0;
        mmu->eram_enable = false;
        mmu->inbios = true;

        memset(mmu->vram, 0, 0x2000);
        memset(mmu->wram, 0, 0x2000);
        memset(mmu->zram, 0, 0x80);
        memset(mmu->io, 0, 0x80);
        // Need to handle RTC save from file
}

unsigned char rb(unsigned short addr) {
        unsigned char effective_rom_bank;
        switch (addr & 0xF000) {
                /* BIOS (256b) or ROM0 */
                case 0x0000:
                        if (mmu->inbios)
                                if (addr < 0x0100)
                                        return mmu->bios[addr];
                case 0x1000:
                case 0x2000:
                case 0x3000:
                        if ((mmu->mbc == 1) && (mmu->mode == 1)) {
                                effective_rom_bank = ((mmu->rom_bank >> 5) << 5) % mmu->rom_banks;
                                printf("using effective rom bank %d\n", effective_rom_bank);
                                return mmu->rom[effective_rom_bank * 0x4000 + addr];
                        }
                        return mmu->rom[addr];

                /* ROM1 (banked) (16k) */
                case 0x4000:
                case 0x5000:
                case 0x6000:
                case 0x7000:
                        return mmu->rom[mmu->rom_bank * 0x4000 + (addr & 0x3FFF)];

                /* Graphics: VRAM (8k) */
                case 0x8000:
                case 0x9000:
                        if (gpu.mode == 3) return 0xFF;
                        return gpu.vram[addr & 0x1FFF];

                /* External RAM (8k) */
                case 0xA000:
                case 0xB000:
                        if (mmu->eram_enable) {
                                if ((mmu->mbc == 1) && (mmu->mode == 0)) {
                                        return mmu->eram[addr & 0x1FFF];
                                }
                                if (mmu->mbc == 3) {
                                        switch (mmu->ram_bank)
                                        {
                                        case 0x8:
                                                return mmu->rtc.seconds;
                                        case 0x9:
                                                return mmu->rtc.minutes;
                                        case 0xA:
                                                return mmu->rtc.hours;
                                        case 0xB:
                                                return mmu->rtc.day_counter & 0xFF;
                                        case 0xC:
                                                return (mmu->rtc.halt_flag << 6) |
                                                        (mmu->rtc.day_counter_carry << 7) |
                                                        GET_BIT(mmu->rtc.day_counter, 8);
                                        }
                                }
                                return mmu->eram[mmu->ram_bank * 0x2000 + (addr & 0x1FFF)];
                        }
                                
                        return 0xFF;

                /* Working RAM (8k) */
                case 0xC000:
                case 0xD000:
                        return mmu->wram[(addr & 0x1FFF)];

                /* Working RAM shadow (8k) */
                case 0xE000:
                        return mmu->wram[addr & 0x1FFF];
                /* Working RAM shadow, I/O, zero-page RAM */
                case 0xF000:
                        switch (addr & 0x0F00)
                        {
                                /* Working RAM shadow */
                                case 0x000: case 0x100: case 0x200: case 0x300:
                                case 0x400: case 0x500: case 0x600: case 0x700:
                                case 0x800: case 0x900: case 0xA00: case 0xB00:
                                case 0xC00: case 0xD00:
                                        return mmu->wram[addr & 0x1FFF];

                                case 0xE00:
                                        if (addr < 0xFEA0) {
                                                if (gpu.mode & 0x2) return 0xFF;
                                                return gpu.oam[addr & 0xFF];
                                        } else
                                                return 0;
                                case 0xF00:
                                        if (addr >= 0xFF80) {
                                                return mmu->zram[addr & 0x7F];
                                        } else if (addr >= Gb_Apu_start_addr &&
                                                       addr <= Gb_Apu_end_addr) {
                                                return Gb_Apu_read_register(apu,
                                                                z80.clock.long_time,
                                                                addr);
                                        } else {
                                        switch (addr) {
                                        case 0xFF00:
                                                return 0xC0 | key.rows[key.column];
                                        case 0xFF02:
                                                return 0x7E; // unimplemented serial read
                                        case 0xFF04:
                                                return z80_p->clock.div;
                                        case 0xFF05:
                                                return z80_p->clock.tima;
                                        case 0xFF06:
                                                return z80_p->clock.tma;
                                        case 0xFF07:
                                                return 0xF8 | z80_p->clock.tac;
                                        case 0xFF0F:
                                                return 0xE0 | z80_p->int_f;
                                        case 0xFF40:
                                                return gpu.gpu_ctrl;
                                        case 0xFF41:
                                                return 0x80 | gpu.gpu_stat;
                                        case 0xFF42:
                                                return gpu.scrollY;
                                        case 0xFF43:
                                                return gpu.scrollX;
                                        case 0xFF44:
                                                return gpu.line;
                                        case 0xFF45:
                                                return gpu.lineYC;
                                        case 0xFF46:
                                                return gpu.DMA;
                                        case 0xFF47:
                                                return gpu.bg_pal;
                                        case 0xFF48:
                                                return gpu.ob_pal0;
                                        case 0xFF49:
                                                return gpu.ob_pal1;
                                        case 0xFF4A:
                                                return gpu.wdow_y;
                                        case 0xFF4B:
                                                return gpu.wdow_x;
                                        case 0xFFFF:
                                                return z80_p->int_en;
                                        default:
                                                return 0xFF; // mmu->io[addr & 0x7F];
                                                /* TODO: handle I/O */
                                        }
                                        }
                        }
        }
        return 0;
}

unsigned short rw(unsigned short addr) {
        return (unsigned short)(rb(addr) | (rb(addr + 1) << 8));
}

void ww(unsigned short addr, unsigned short val) {
        wb(addr, val & 0xFF);
        wb(addr + 1, val >> 8);
}

// TODO: MBC when writing to ROM
void wb(unsigned short addr, unsigned char val) {
        unsigned short threshold, old_threshold;
        switch (addr & 0xF000) {
                /* BIOS (256b) or ROM0 */
                case 0x0000:
                case 0x1000:
                        if (mmu->mbc & 1) {
                                mmu->eram_enable = ((val & 0x0F) == 0x0A);
                                if (!mmu->eram_enable) {
                                        save_sram();
                                        printf("SRAM disabled\n");
                                } else {
                                        printf("SRAM enabled\n");
                                }
                        } else if ((mmu->mbc == 2) && ((addr & 0x100) == 0)) {
                                mmu->eram_enable = ((val & 0x0F) == 0x0A);
                                if (!mmu->eram_enable) {
                                        save_sram();
                                }
                        }
                        break;
                case 0x2000:
                case 0x3000:
                        if (mmu->mbc == 1) {
                                mmu->rom_bank = ((mmu->rom_bank & ~0x1F) | ((val ? val : val + 1) & 0x1F)) % mmu->rom_banks;
                                if ((mmu->rom_bank & 0x1F) == 0) {
                                        mmu->rom_bank = (mmu->rom_bank & ~0x1F) + 1;
                                }
                                printf("MBC1 ROM BANK %d selected\n", mmu->rom_bank);
                        } else if (mmu->mbc == 3) {
                                mmu->rom_bank = (val ? val : val + 1)  & 0x7F;
                        } else if (mmu->mbc == 2 && (addr & 0x100)) {
                                mmu->rom_bank = val & 0x0F;
                        }
                        break;

                /* ROM1 (unbanked) (16k) */
                case 0x4000:
                case 0x5000:
                        if (mmu->mbc == 1) {
                                if (mmu->mode) {
                                        mmu->ram_bank = (val & 0x3) % mmu->ram_banks;
                                        printf("MBC1 MODE 1 RAM BANK %d selected\n", mmu->ram_bank);
                                }
                                mmu->rom_bank = (unsigned char)((mmu->rom_bank & 0x1F) | ((val & 0x3) << 5)) % mmu->rom_banks;
                                if ((mmu->rom_bank & 0x1F) == 0) {
                                        mmu->rom_bank = (mmu->rom_bank & ~0x1F) + 1;
                                }
                                printf("MBC1 MODE 0 ROM BANK %d selected\n", mmu->rom_bank);
                        } else if (mmu->mbc == 3) {
                                mmu->ram_bank = val & 0x7;
                        }
                        break;
                case 0x6000:
                case 0x7000:
                        if (mmu->mbc == 1) {
                                mmu->mode = val & 1; // MBC 1
                                printf("MBC 1 MODE %d\n", mmu->mode);
                                if (mmu->mode) {
                                        mmu->rom_bank &= 0x1F;
                                        printf("MBC1 MODE 1 ROM BANK %d\n", mmu->rom_bank);
                                } else {
                                        // mmu->ram_bank = 0;
                                        printf("MBC1 MODE 0 IGNORING RAM BANK");
                                }
                        } else if (mmu->mbc == 3) {
                                if ((mmu->rtc.last_latch_write == 0) && (val == 1)) {
                                        time_t rawtime;
                                        struct tm* timeinfo;

                                        time(&rawtime);
                                        timeinfo = localtime(&rawtime);

                                        mmu->rtc.seconds = timeinfo->tm_sec;
                                        mmu->rtc.minutes = timeinfo->tm_min;
                                        mmu->rtc.hours = timeinfo->tm_hour;
                                        mmu->rtc.day_counter = timeinfo->tm_yday;
                                        printf("latched RTC\n");
                                }
                                mmu->rtc.last_latch_write = val;
                        }
                        break;

                /* Graphics: VRAM (8k) */
                case 0x8000:
                case 0x9000:
                        if (gpu.mode == 3) return;
                        gpu.vram[addr & 0x1FFF] = val;
                        if (addr <= 0x97FF) {
                                unsigned short tmp_addr = addr & 0x1FFE;
                                unsigned short tile = (tmp_addr >> 4) & 511;
                                unsigned char y = (tmp_addr >> 1) & 7;
                                unsigned char sx, x;
                                for (x = 0; x < 8; x++) {
                                        sx = (unsigned char)(1 << (7 - x));
                                        gpu.tileset[tile][y][x] = 
                                                ((gpu.vram[tmp_addr] & sx) ? 1 : 0) +
                                                ((gpu.vram[tmp_addr+1] & sx) ? 2 : 0);
                                }
                        }
                        break;

                /* External RAM (8k) */
                case 0xA000:
                case 0xB000:
                        if (!mmu->eram_enable) {
                                return;
                        }
                        if ((mmu->mbc == 1) && (mmu->mode == 0)) {
                                mmu->eram[addr & 0x1FFF] = val;
                                return;
                        }
                        if (mmu->mbc == 3) {
                                switch (mmu->ram_bank)
                                {
                                case 0x8:
                                        mmu->rtc.seconds = val;
                                        return;
                                case 0x9:
                                        mmu->rtc.minutes = val;
                                        return;
                                case 0xA:
                                        mmu->rtc.hours = val;
                                        return;
                                case 0xB:
                                        mmu->rtc.day_counter = (mmu->rtc.day_counter & 0x100) | val;
                                        return;
                                case 0xC:
                                        mmu->rtc.day_counter = (mmu->rtc.day_counter & 0xFF) | ((val & 1) << 8);
                                        mmu->rtc.halt_flag = GET_BIT(val, 6);
                                        mmu->rtc.day_counter_carry = GET_BIT(val, 7);
                                        return;
                                }
                        }
                        mmu->eram[mmu->ram_bank * 0x2000 + (addr & 0x1FFF)] = val;
                        break;

                /* Working RAM (8k) */
                case 0xC000:
                case 0xD000:
                        mmu->wram[addr & 0x1FFF] = val;
                        break;

                /* Working RAM shadow (8k) */
                case 0xE000:
                        mmu->wram[addr & 0x1FFF] = val;
                        break;
                /* Working RAM shadow, I/O, zero-page RAM */
                case 0xF000:
                        switch (addr & 0x0F00)
                        {
                                /* Working RAM shadow */
                                case 0x000: case 0x100: case 0x200: case 0x300:
                                case 0x400: case 0x500: case 0x600: case 0x700:
                                case 0x800: case 0x900: case 0xA00: case 0xB00:
                                case 0xC00: case 0xD00:
                                        mmu->wram[addr & 0x1FFF] = val;
                                        break;

                                case 0xE00:
                                        if (addr < 0xFEA0) {
                                                if (gpu.mode & 0x2) return;
                                                gpu.oam[addr & 0xFF] = val;
                                        } 
                                        break;
                                case 0xF00:
                                        if (addr >= 0xFF80) {
                                                mmu->zram[addr & 0x7F] = val;
                                        } else {
                                                mmu->io[addr & 0x7F] = val;
                                        }
                                        if (addr >= Gb_Apu_start_addr &&
                                                        addr <= Gb_Apu_end_addr) {
                                                Gb_Apu_write_register(apu,
                                                                z80.clock.long_time,
                                                                addr, val);
                                        }
                                        switch (addr) {
                                        case 0xFF00:
                                                key.column = ((val & 0x30) >> 4);
                                                break;
                                        case 0xFF02:
                                                if (val == 0x81) {
                                                        printf("%c", mmu->io[1]);
                                                        fflush(stdout);
                                                }
                                                break;
                                        case 0xFF04:
                                                threshold = 3 + (2 * (((z80_p->clock.tac & 3) - 1) & 3));
                                                printf("writing to DIV\n");
                                                printf("enter div register: 0x%04X\n", z80_p->clock.m);
                                                printf("t time: %d\n", z80_p->t);
                                                printf("threshold: 0x%04X\n", 1 << threshold);
                                                
                                                if ((z80_p->clock.tac & 4) && (((z80_p->clock.m /*+ z80_p->t */) >> threshold) & 1)) {
                                                        printf("div write tima inc\n");
                                                        z80_p->clock.tima++;
                                                        if (!z80_p->clock.tima) {
                                                                z80_p->clock.tima = z80_p->clock.tma;
                                                                z80_p->int_f |= 4;
                                                        }
                                                }
                                                z80_p->clock.m = 0;
                                                break;
                                        case 0xFF05:
                                                z80_p->clock.tima = val;
                                                break;
                                        case 0xFF06:
                                                z80_p->clock.tma = val;
                                                break;
                                        case 0xFF07:
                                                z80_p->clock.old_tac = z80_p->clock.tac;
                                                z80_p->clock.tac = val;
                                                old_threshold = 3 + (2 * (((z80_p->clock.old_tac & 3) - 1) & 3));
                                                threshold = 3 + (2 * (((z80_p->clock.tac & 3) - 1) & 3));
                                                if ((z80_p->clock.tac & z80_p->clock.old_tac & 4) &&
                                                        (((z80_p->clock.m + z80_p->t - 4) >> old_threshold) & 1) &&
                                                        !(((z80_p->clock.m + z80_p->t - 4) >> threshold) & 1)) {
                                                        printf("tima inc on tac write 1\n");
                                                        z80_p->clock.tima++;
                                                        if (!z80_p->clock.tima) {
                                                                z80_p->clock.tima = z80_p->clock.tma;
                                                                z80_p->int_f |= 4;
                                                        }
                                                }
                                                if ((~z80_p->clock.tac & z80_p->clock.old_tac & 4) &&
                                                        (((z80_p->clock.m + z80_p->t - 4) >> old_threshold) & 1)) {
                                                        printf("tima inc on tac write 2\n");
                                                        z80_p->clock.tima++;
                                                        if (!z80_p->clock.tima) {
                                                                z80_p->clock.tima = z80_p->clock.tma;
                                                                z80_p->int_f |= 4;
                                                        }
                                                }
                                                // check to see if this results in falling edge, inc TIMA
                                                break;
                                        case 0xFF0F:
                                                z80_p->int_f = val;
                                                break;
                                        case 0xFF40:
                                                gpu.gpu_ctrl = val;
                                                break;
                                        case 0xFF41:
                                                gpu.gpu_stat = val;
                                                break;
                                        case 0xFF42:
                                                gpu.scrollY = val;
                                                break;
                                        case 0xFF43:
                                                gpu.scrollX = val;
                                                break;
                                        case 0xFF44:
                                                gpu.line = val;
                                                break;
                                        case 0xFF45:
                                                gpu.lineYC = val;
                                                break;
                                        case 0xFF46:
                                                gpu.DMA = val;
                                                gpu.do_DMA = true;
                                                break;
                                        case 0xFF47:
                                                gpu.bg_pal = val;
                                                break;
                                        case 0xFF48:
                                                gpu.ob_pal0 = val;
                                                break;
                                        case 0xFF49:
                                                gpu.ob_pal1 = val;
                                                break;
                                        case 0xFF4A:
                                                gpu.wdow_y = val;
                                                break;
                                        case 0xFF4B:
                                                gpu.wdow_x = val;
                                                break;
                                        case 0xFF50:
                                                mmu->inbios = false;
                                                break;
                                        case 0xFFFF:
                                                z80_p->int_en = val;
                                                break;
                                        default:
                                                break;
                                        }
                        }
        }
}

