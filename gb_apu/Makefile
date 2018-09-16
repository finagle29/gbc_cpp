CXX = g++ -O2

all:	Gb_Apu.o Gb_Oscs.o Blip_Buffer.o Multi_Buffer.o MB_Wrapper.o GBAPU_Wrapper.o

clean:
	rm -rf *.o

Gb_Apu.o:	Gb_Apu.cpp Gb_Apu.h Gb_Oscs.h
	$(CXX) -c Gb_Apu.cpp

Gb_Oscs.o:	Gb_Oscs.cpp Gb_Oscs.h Blip_Buffer.o Gb_Apu.h
	$(CXX) -c Gb_Oscs.cpp

Blip_Buffer.o:	Blip_Buffer.cpp Blip_Buffer.h blargg_common.h
	$(CXX) -c Blip_Buffer.cpp

Multi_Buffer.o:	Multi_Buffer.cpp Multi_Buffer.h Blip_Buffer.h
	$(CXX) -c Multi_Buffer.cpp

MB_Wrapper.o:	Multi_Buffer.o MB_Wrapper.h
	$(CXX) -c MB_Wrapper.cpp

GBAPU_Wrapper.o:	Gb_Apu.o GBAPU_Wrapper.h
	$(CXX) -c GBAPU_Wrapper.cpp
