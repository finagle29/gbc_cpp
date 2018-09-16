CC = gcc
CXX = g++
CCFLAGS = -Weverything
CXFLAGS = -Weverything

OBJECTS = main.o gbz80.o mmu.o gpu.o apu.o
OBJECTS += gb_apu/GBAPU_Wrapper.o gb_apu/Gb_Apu.o
OBJECTS += gb_apu/MB_Wrapper.o gb_apu/Multi_Buffer.o
OBJECTS += gb_apu/Gb_Oscs.o gb_apu/Blip_Buffer.o

all:
	+$(MAKE) -C gb_apu

debug:	CCFLAGS += -g -O0 -DDEBUG
debug:	CXFLAGS += -g -O0
debug:	test

quiet:	CCFLAGS += -g -O0
quiet:	CXFLAGS += -g -O0
quiet: test

release:	CCFLAGS += -O2
release:	CXFLAGS += -O2
release:	test

SDLFLAGS =  -framework SDL2 -L/Library/Frameworks/SDL2.framework

SDL2 = /Library/Frameworks/SDL2.framework/SDL2

apu:
	$(MAKE) -C gb_apu

gbz80.o: gbz80.c gbz80.h mmu.h mmu.c gpu.h
	$(CC) $(CCFLAGS) -c gbz80.c

mmu.o:	mmu.c mmu.h gpu.h apu
	$(CC) $(CCFLAGS) -c mmu.c

gpu.o:	gpu.c gpu.h mmu.h mmu.c
	$(CC) $(CCFLAGS) -c gpu.c

apu.o:	apu.c apu apu.h
	$(CC) $(CCFLAGS) -c apu.c

main.o: main.c gpu.h gpu.c mmu.c mmu.h gbz80.c gbz80.h apu
	$(CC) $(CCFLAGS) -c main.c

test: main.o gpu.o gbz80.o mmu.o apu.o apu
	$(CXX) $(SDLFLAGS) $(OBJECTS) $(SDL2) -o test
	dsymutil test

clean:
	rm -rf *.o test
