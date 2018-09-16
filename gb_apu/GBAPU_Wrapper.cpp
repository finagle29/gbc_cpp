#include "Gb_Apu.h"
#include "GBAPU_Wrapper.h"

extern "C" {

        Gb_Apu* new_Gb_Apu() {
                return new Gb_Apu();
        }
        void delete_Gb_Apu(Gb_Apu* g) {
                delete g;
        }

        void Gb_Apu_volume(Gb_Apu* g, double vol) {
                g->volume(vol);
        }
        void Gb_Apu_treble_eq(Gb_Apu* g, const blip_eq_t* eq) {
                g->treble_eq(*eq);
        }
        void Gb_Apu_reset(Gb_Apu* g) {
                g->reset();
        }
        void Gb_Apu_output_3(Gb_Apu* g, Blip_Buffer* center,
                        Blip_Buffer* left, Blip_Buffer* right) {
                g->output(center, left, right);
        }

        void Gb_Apu_osc_output_3(Gb_Apu* g, int index,
                        Blip_Buffer* center, Blip_Buffer* left,
                        Blip_Buffer* right) {
                g->osc_output(index, center, left, right);
        }


        void Gb_Apu_write_register(Gb_Apu* g, gb_time_t cpu_time,
                        gb_addr_t addr, int data) {
                g->write_register(cpu_time, addr, data);
        }
        int Gb_Apu_read_register(Gb_Apu* g, gb_time_t cpu_time,
                        gb_addr_t addr) {
                return g->read_register(cpu_time, addr);
        }

        bool Gb_Apu_end_frame(Gb_Apu* g, gb_time_t end_time) {
                return g->end_frame(end_time);
        }

        void Gb_Apu_output_1( Gb_Apu* g, Blip_Buffer* b ) {
                g->output( b, NULL, NULL );
        }
                
        void Gb_Apu_osc_output_1( Gb_Apu* g, int i, Blip_Buffer* b ) {
                g->osc_output( i, b, NULL, NULL );
        }
}
