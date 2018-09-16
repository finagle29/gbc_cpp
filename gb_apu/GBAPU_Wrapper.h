#ifndef GBAPU_WRAPPER_H
#define GBAPU_WRAPPER_H

typedef long     gb_time_t; // clock cycle count
typedef unsigned gb_addr_t; // 16-bit address

#include "BB_Wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Gb_Apu Gb_Apu;

Gb_Apu* new_Gb_Apu(void);
void delete_Gb_Apu(Gb_Apu* g);
void Gb_Apu_volume(Gb_Apu* g, double vol);
void Gb_Apu_treble_eq(Gb_Apu* g, const blip_eq_t* eq);
void Gb_Apu_reset(Gb_Apu* g);
void Gb_Apu_output_1(Gb_Apu* g, Blip_Buffer* mono);
void Gb_Apu_output_3(Gb_Apu* g, Blip_Buffer* center, Blip_Buffer* left,
                Blip_Buffer* right);

enum {Gb_Apu_osc_count = 4};
void Gb_Apu_osc_output_1(Gb_Apu* g, int index, Blip_Buffer* mono);
void Gb_Apu_osc_output_3(Gb_Apu* g, int index, Blip_Buffer* center,
                Blip_Buffer* left, Blip_Buffer* right);

enum {Gb_Apu_start_addr = 0xff10};
enum {Gb_Apu_end_addr = 0xff3f};
enum {Gb_Apu_register_count = Gb_Apu_end_addr - Gb_Apu_start_addr + 1};

void Gb_Apu_write_register(Gb_Apu* g, gb_time_t cpu_time, gb_addr_t addr,
                int data);
int Gb_Apu_read_register(Gb_Apu* g, gb_time_t cpu_time, gb_addr_t addr);

bool Gb_Apu_end_frame(Gb_Apu* g, gb_time_t end_time);


#ifdef __cplusplus
}
#endif

#endif
