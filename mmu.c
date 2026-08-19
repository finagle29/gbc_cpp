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

char save_fname[256];

void init_mmu()
{

        unsigned short addr = 0;
        for (addr = 0; addr < 0x8000; addr++)
        {
                wb(addr, 0);
        }
}

void print_n_bytes(unsigned short n)
{
        unsigned short i = 0;
        while (i < n)
        {
                printf("%x\n", rb(i++));
        }
}

void save_sram()
{
        printf("saving to %s\n", save_fname);
        FILE *f = fopen(save_fname, "w+b");
        fwrite(mmu->eram, sizeof(unsigned char), sizeof(mmu->eram), f);
        fwrite(&mmu->rtc, sizeof(rtc_type), 1, f);
        fclose(f);
}

void load_bios(char *fname)
{
        struct stat buffer;
        if (stat(fname, &buffer) != 0)
        {
                printf("BIOS %s not found\n.", fname);
                exit(1);
        }
        FILE *f = fopen(fname, "rb");
        if (f == NULL)
        {
                printf("Error opening %s", fname);
                exit(1);
        }
        fread(mmu->bios, sizeof(unsigned char), 0x100, f);
        fclose(f);
}

void load_rom(char *fname)
{
        char fname_copy[256];
        strcpy(fname_copy, fname);
        char *base_name = strtok(fname_copy, ".");
        unsigned char rom_size_byte;
        unsigned long rom_size;
        struct stat buffer;

        // char *extension = strrchr(fname_copy, ".");
        if (base_name != NULL)
        {
                snprintf(save_fname, 256, "%s.sav", base_name);
        }

        if (stat(fname, &buffer) != 0)
        {
                printf("ROM %s not found.\n", fname);
                exit(1);
        }
        FILE *f = fopen(fname, "rb");

        fseek(f, 0, SEEK_END);
        rom_size = (unsigned long)ftell(f);

        mmu = (mmu_type *)malloc(sizeof(mmu_type) + rom_size);

        fseek(f, 0x147, SEEK_SET);
        switch (fgetc(f))
        {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x08:
        case 0x09:
                mmu->mbc = 1;
                break;
        case 0x05:
        case 0x06:
                mmu->mbc = 2;
                break;
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
                mmu->mbc = 3;
                break;
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0X1E:
                mmu->mbc = 5;
                break;
        default:
                mmu->mbc = 1;
        }
        printf("using mbc%d\n", mmu->mbc);
        switch (rom_size_byte = fgetc(f))
        {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        case 0x8:
                // no ROM banking
                mmu->rom_bank_mask = (2 << rom_size_byte) - 1;
                break;
        default:
                mmu->rom_bank_mask = 0;
        }
        switch (fgetc(f))
        {
        case 0x0:
        case 0x2:
                mmu->ram_bank_mask = 0;
                break;
        case 0x3:
                mmu->ram_bank_mask = 3;
                break;
        case 0x4:
                mmu->ram_bank_mask = 0xF;
                break;
        case 0x5:
                mmu->ram_bank_mask = 7;
                break;
        }
        printf("ROM size: %lx\n", rom_size);
        printf("rom banking mask: %02x\n", mmu->rom_bank_mask);
        printf("ram banking mask: %02x\n", mmu->ram_bank_mask);

        // check for MBC1M
        mmu->mbc1m = false;
        if ((rom_size >= 0x40104) && (mmu->mbc == 1))
        {
                fseek(f, 0x40104, SEEK_SET);
                if ((fgetc(f) == 0xCE) && (fgetc(f) == 0xED) && (fgetc(f) == 0x66) && (fgetc(f) == 0x66))
                {
                        // MBC1M mode
                        mmu->mbc1m = true;
                        printf("MBC1M detected\n");
                }
        }

        rewind(f);
        mmu->mode = 0;

        if (mmu == NULL)
        {
                fprintf(stderr, "Error using malloc for mmu.");
                fclose(f);
                exit(1);
        }

        fseek(f, 0, SEEK_SET);

        fread(mmu->rom, sizeof(unsigned char), rom_size, f);
        fclose(f);

        mmu->rtc = (rtc_type){};
        mmu->rtc_latched = (rtc_type){};

        if (stat(save_fname, &buffer) == 0)
        {
                printf("reading save file %s ...\n", save_fname);
                f = fopen(save_fname, "rb");
                size_t result = fread(mmu->eram, sizeof(unsigned char), sizeof(mmu->eram), f);
                printf("read %lu bytes\n", result);
                result = fread(&mmu->rtc, sizeof(rtc_type), 1, f);
                fclose(f);
        }
        else
        {
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

        // mmu->serial = 0;
        // Need to handle RTC save from file
}

unsigned char rb(unsigned short addr)
{
        switch (addr & 0xF000)
        {
        /* BIOS (256b) or ROM0 */
        case 0x0000:
                if (mmu->inbios)
                        if (addr < 0x0100)
                                return mmu->bios[addr];
        case 0x1000:
        case 0x2000:
        case 0x3000:
                if (mmu->mbc == 1)
                {
                        if (mmu->mode == 1)
                        {
                                if (mmu->mbc1m)
                                {
                                        return mmu->rom[(mmu->rom_bank & 0x30) * 0x4000 + addr];
                                }
                                return mmu->rom[(mmu->rom_bank & 0x60) * 0x4000 + addr];
                        }
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
                if (gpu.mode == 3)
                        return 0xFF;
                return gpu.vram[addr & 0x1FFF];

        /* External RAM (8k) */
        case 0xA000:
        case 0xB000:
                if (mmu->eram_enable)
                {
                        if (mmu->mbc == 3)
                        {
                                switch (mmu->ram_bank)
                                {
                                case 0x8:
                                        return mmu->rtc_latched.seconds;
                                case 0x9:
                                        return mmu->rtc_latched.minutes;
                                case 0xA:
                                        return mmu->rtc_latched.hours;
                                case 0xB:
                                        return mmu->rtc_latched.day_counter & 0xFF;
                                case 0xC:
                                        return (mmu->rtc_latched.halt_flag << 6) |
                                               (mmu->rtc_latched.day_counter_carry << 7) |
                                               GET_BIT(mmu->rtc_latched.day_counter, 8);
                                }
                        }
                        if (mmu->mbc == 1)
                        {
                                if (mmu->mode == 1)
                                {
                                        return mmu->eram[(mmu->ram_bank & mmu->ram_bank_mask) * 0x2000 + (addr & 0x1FFF)];
                                }
                                else
                                {
                                        return mmu->eram[addr & 0x1FFF];
                                }
                        }
                        if (mmu->mbc == 2)
                        {
                                return mmu->eram[addr & 0x1FF];
                        }
                        return mmu->eram[(mmu->ram_bank & mmu->ram_bank_mask) * 0x2000 + (addr & 0x1FFF)];
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
                case 0x000:
                case 0x100:
                case 0x200:
                case 0x300:
                case 0x400:
                case 0x500:
                case 0x600:
                case 0x700:
                case 0x800:
                case 0x900:
                case 0xA00:
                case 0xB00:
                case 0xC00:
                case 0xD00:
                        return mmu->wram[addr & 0x1FFF];

                case 0xE00:
                        if (addr < 0xFEA0)
                        {
                                if (gpu.mode & 0x2)
                                        return 0xFF;
                                return gpu.oam[addr & 0xFF];
                        }
                        else
                                return 0;
                case 0xF00:
                        if (addr >= 0xFF80)
                        {
                                return mmu->zram[addr & 0x7F];
                        }
                        else if (addr >= Gb_Apu_start_addr &&
                                 addr <= Gb_Apu_end_addr)
                        {
                                return Gb_Apu_read_register(apu,
                                                            z80.clock.long_time,
                                                            addr);
                        }
                        else
                        {
                                switch (addr)
                                {
                                case 0xFF00:
                                        return 0xC0 | key.rows[key.column];
                                        // return 0xC0 | key.rows[((key.column & 0x30) >> 4)];
                                case 0xFF02:
                                        return 0x7E;
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
                                        return 0xFF;
                                        // return mmu->io[addr & 0x7F];
                                        /* TODO: handle I/O */
                                }
                        }
                }
        }
        return 0;
}

unsigned short rw(unsigned short addr)
{
        return (unsigned short)(rb(addr) | (rb(addr + 1) << 8));
}

void ww(unsigned short addr, unsigned short val)
{
        wb(addr, val & 0xFF);
        wb(addr + 1, val >> 8);
}

// TODO: MBC when writing to ROM
void wb(unsigned short addr, unsigned char val)
{
        unsigned short threshold, old_threshold;
        switch (addr & 0xF000)
        {
        /* BIOS (256b) or ROM0 */
        /* RAMG */
        case 0x0000:
        case 0x1000:
                if ((mmu->mbc == 1) || (mmu->mbc == 3))
                {
                        if (mmu->eram_enable)
                        {
                                mmu->eram_enable = ((val & 0xF) == 0xA);
                                if (!mmu->eram_enable)
                                {
                                        save_sram();
                                }
                        }
                        else
                        {
                                mmu->eram_enable = ((val & 0xF) == 0xA);
                                if (mmu->eram_enable)
                                {
                                        printf("enabling RAM\n");
                                }
                        }
                        // mmu->eram_enable = ((val & 0xF) == 0xA);
                        // if (mmu->eram_enable)
                        //         printf("eram enabled: (%04x) = %02x\n", addr, val);
                        // else
                        // {
                        //         printf("eram disabled: (%04x) = %02x\n", addr, val);
                        // }
                        // if (!mmu->eram_enable)
                        // {
                        //         save_sram();
                        // }
                        break;
                }
                else if (mmu->mbc == 5)
                {
                        if (mmu->eram_enable)
                        {
                                mmu->eram_enable = (val == 0x0A);
                                if (!mmu->eram_enable)
                                {
                                        save_sram();
                                }
                        }
                        else
                        {
                                mmu->eram_enable = (val == 0x0A);
                        }
                        break;
                }

        /* BANK1 / ROMB0 */
        case 0x2000:
                if (mmu->mbc == 5)
                {
                        mmu->rom_bank = (mmu->rom_bank & 0x100) | val;
                        mmu->rom_bank &= mmu->rom_bank_mask;
                        // printf("rom_bank %03x\n", mmu->rom_bank);
                        break;
                }
        case 0x3000:
                // printf("(%04x) <- %02x\n", addr, val);
                if (mmu->mbc == 1)
                {
                        if (mmu->mbc1m)
                        {
                                mmu->rom_bank = ((val & 0x1F) ? (val & 0xF) : val + 1) | (mmu->rom_bank & 0x30);
                        }
                        else
                        {

                                mmu->rom_bank = (((val & 0x1F) ? val : val + 1) & 0x1F) | (mmu->rom_bank & 0x60);
                        }
                        // mmu->rom_bank = (((val & 0x1F) ? val : val + 1) & mmu->rom_bank_mask) | (mmu->rom_bank & 0x60);
                }
                else if (mmu->mbc == 3)
                {
                        mmu->rom_bank = ((val & 0x7F) ? val : val + 1) & 0x7F;
                }
                else if (mmu->mbc == 2)
                {
                        if ((addr & 0x100) == 0)
                        {
                                {
                                        mmu->eram_enable = ((val & 0xF) == 0xA);
                                        if (!mmu->eram_enable)
                                        {
                                                save_sram();
                                        }
                                }
                        }
                        else
                        {
                                mmu->rom_bank = ((val & 0xF) ? val : val + 1) & 0xF;
                        }
                }
                else if (mmu->mbc == 5)
                {
                        mmu->rom_bank = (mmu->rom_bank & 0xFF) | ((val & 1) << 9);
                }
                mmu->rom_bank &= mmu->rom_bank_mask;
                // printf("ROM bank %03x\n", mmu->rom_bank);
                break;

        /* ROM1 (unbanked) (16k) */
        /* BANK2 / ROMB1 */
        case 0x4000:
        case 0x5000:
                // printf("(%04x) <- %02x\n", addr, val);
                if (mmu->mbc == 1)
                {
                        if (mmu->mode)
                        {
                                mmu->ram_bank = val & 3;
                        }
                        if (mmu->mbc1m)
                        {
                                mmu->rom_bank = (unsigned char)((mmu->rom_bank & 0xF) | ((val & 3) << 4));
                                mmu->rom_bank &= mmu->rom_bank_mask;
                        }
                        else
                        {
                                mmu->rom_bank = (unsigned char)((mmu->rom_bank & 0x1F) | ((val & 3) << 5));
                                mmu->rom_bank &= mmu->rom_bank_mask;
                        }
                }
                else if (mmu->mbc == 3)
                {
                        mmu->ram_bank = val & 0x7;
                        mmu->ram_bank = val;
                }
                else if (mmu->mbc == 5)
                {
                        mmu->ram_bank = val & 0xF;
                }
                // mmu->ram_bank &= mmu->ram_bank_mask;
                // printf("ROM bank: %02x\n", mmu->rom_bank);
                // printf("RAM bank: %02x\n", mmu->ram_bank);
                break;
        case 0x6000:
        case 0x7000:
                // printf("(%04x) <- %02x\n", addr, val);
                if (mmu->mbc == 1)
                {
                        mmu->mode = val & 1; // MBC 1
                        // if (mmu->mode == 0)
                        // {
                        //         mmu->ram_bank = 0;
                        //         printf("RAM bank: %02x\n", mmu->ram_bank);
                        // }
                }
                else if (mmu->mbc == 3)
                {
                        if ((mmu->rtc.last_latch_write == 0) && (val == 1))
                        {

                                mmu->rtc_latched.seconds = mmu->rtc.seconds;
                                mmu->rtc_latched.minutes = mmu->rtc.minutes;
                                mmu->rtc_latched.hours = mmu->rtc.hours;
                                mmu->rtc_latched.day_counter = mmu->rtc.day_counter;
                                mmu->rtc_latched.day_counter_carry = mmu->rtc.day_counter_carry;
                                mmu->rtc_latched.halt_flag = mmu->rtc.halt_flag;

                                // printf("latched RTC\n");
                                // printf("%03d %02d:%02d:%02d\n", mmu->rtc_latched.day_counter, mmu->rtc_latched.hours,
                                //        mmu->rtc_latched.minutes, mmu->rtc_latched.seconds);
                        }
                        mmu->rtc.last_latch_write = val;
                }
                break;

        /* Graphics: VRAM (8k) */
        case 0x8000:
        case 0x9000:
                if (gpu.mode == 3)
                        return;
                gpu.vram[addr & 0x1FFF] = val;
                if (addr <= 0x97FF)
                {
                        unsigned short tmp_addr = addr & 0x1FFE;
                        unsigned short tile = (tmp_addr >> 4) & 511;
                        unsigned char y = (tmp_addr >> 1) & 7;
                        unsigned char sx, x;
                        for (x = 0; x < 8; x++)
                        {
                                sx = (unsigned char)(1 << (7 - x));
                                gpu.tileset[tile][y][x] =
                                    ((gpu.vram[tmp_addr] & sx) ? 1 : 0) +
                                    ((gpu.vram[tmp_addr + 1] & sx) ? 2 : 0);
                        }
                }
                break;

        /* External RAM (8k) */
        case 0xA000:
        case 0xB000:
                if (mmu->mbc == 3)
                {
                        switch (mmu->ram_bank)
                        {
                        case 0x8:
                                mmu->rtc.seconds = val & 0x3F;
                                mmu->rtc.ticks = 0;
                                return;
                        case 0x9:
                                mmu->rtc.minutes = val & 0x3F;
                                return;
                        case 0xA:
                                mmu->rtc.hours = val & 0x1F;
                                return;
                        case 0xB:
                                mmu->rtc.day_counter = (mmu->rtc.day_counter & 0x100) | (val & 0xFF);
                                return;
                        case 0xC:
                                mmu->rtc.day_counter = (mmu->rtc.day_counter & 0xFF) | ((val & 1) << 8);
                                mmu->rtc.halt_flag = GET_BIT(val, 6);
                                mmu->rtc.day_counter_carry = GET_BIT(val, 7);
                                return;
                        }
                }
                if (mmu->eram_enable)
                {
                        if (mmu->mbc == 2)
                        {
                                mmu->eram[addr & 0x1FF] = (val | 0xF0);
                                break;
                        }
                        else if ((mmu->mbc == 1) && (mmu->mode == 0))
                        {
                                mmu->eram[addr & 0x1FFF] = val;
                                break;
                        }
                        mmu->eram[mmu->ram_bank * 0x2000 + (addr & 0x1FFF)] = val;
                }
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
                case 0x000:
                case 0x100:
                case 0x200:
                case 0x300:
                case 0x400:
                case 0x500:
                case 0x600:
                case 0x700:
                case 0x800:
                case 0x900:
                case 0xA00:
                case 0xB00:
                case 0xC00:
                case 0xD00:
                        mmu->wram[addr & 0x1FFF] = val;
                        break;

                case 0xE00:
                        if (addr < 0xFEA0)
                        {
                                if (gpu.mode & 0x2)
                                        return;
                                gpu.oam[addr & 0xFF] = val;
                        }
                        break;
                case 0xF00:
                        if (addr >= 0xFF80)
                        {
                                mmu->zram[addr & 0x7F] = val;
                        }
                        else
                        {
                                mmu->io[addr & 0x7F] = val;
                        }
                        if (addr >= Gb_Apu_start_addr &&
                            addr <= Gb_Apu_end_addr)
                        {
                                Gb_Apu_write_register(apu,
                                                      z80.clock.long_time,
                                                      addr, val);
                        }
                        switch (addr)
                        {
                        case 0xFF00:
                                key.column = ((val & 0x30) >> 4);
                                // key.column = val;
                                break;
                        // case 0xFF01:
                        //         mmu->serial = val;
                        //         break;
                        case 0xFF02:
                                if (val == 0x81)
                                {
                                        printf("%c", mmu->io[1]);
                                        // printf("%c", mmu->serial);
                                        fflush(stdout);
                                }
                                break;
                        case 0xFF04:
                                threshold = 3 + (2 * (((z80_p->clock.tac & 3) - 1) & 3));
                                printf("writing to DIV\n");
                                printf("enter div register: 0x%04X\n", z80_p->clock.m);
                                printf("t time: %d\n", z80_p->t);
                                printf("threshold: 0x%04X\n", 1 << threshold);

                                if ((z80_p->clock.tac & 4) && (((z80_p->clock.m /*+ z80_p->t */) >> threshold) & 1))
                                {
                                        printf("div write tima inc\n");
                                        z80_p->clock.tima++;
                                        if (!z80_p->clock.tima)
                                        {
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
                                    !(((z80_p->clock.m + z80_p->t - 4) >> threshold) & 1))
                                {
                                        printf("tima inc on tac write 1\n");
                                        z80_p->clock.tima++;
                                        if (!z80_p->clock.tima)
                                        {
                                                z80_p->clock.tima = z80_p->clock.tma;
                                                z80_p->int_f |= 4;
                                        }
                                }
                                if ((~z80_p->clock.tac & z80_p->clock.old_tac & 4) &&
                                    (((z80_p->clock.m + z80_p->t - 4) >> old_threshold) & 1))
                                {
                                        printf("tima inc on tac write 2\n");
                                        z80_p->clock.tima++;
                                        if (!z80_p->clock.tima)
                                        {
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
                                // printf("DMA: %02x\n", val);
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
