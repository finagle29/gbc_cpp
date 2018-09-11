CC = gcc
CCFLAGS = -Weverything

debug:	CCFLAGS += -g -O0 -DDEBUG
debug:	test

quiet:	CCFLAGS += -g -O0
quiet: test

release:	CCFLAGS += -O2
release: test

SDLFLAGS =  -framework SDL2 -L/Library/Frameworks/SDL2.framework

SDL2 = /Library/Frameworks/SDL2.framework/SDL2

gbz80.o: gbz80.c gbz80.h mmu.h mmu.c gpu.h
	$(CC) $(CCFLAGS) -c gbz80.c

mmu.o:	mmu.c mmu.h gpu.h
	$(CC) $(CCFLAGS) -c mmu.c

gpu.o:	gpu.c gpu.h mmu.h mmu.c
	$(CC) $(CCFLAGS) -c gpu.c

main.o: main.c gpu.h gpu.c mmu.c mmu.h gbz80.c gbz80.h
	$(CC) $(CCFLAGS) -c main.c

test: main.o gpu.o gbz80.o mmu.o
	$(CC) $(SDLFLAGS) main.o gbz80.o mmu.o gpu.o $(SDL2) -o test
	dsymutil test

clean:
	rm -rf *.o test
