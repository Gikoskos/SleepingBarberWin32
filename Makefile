CC              = gcc
CFLAGS          = -Wall -std=c11
DBGFLAGS        = -g -ggdb -D_DEBUG
RLSFLAGS        = -mwindows 
SRC             = main.c customer.c barber.c res.o
MORE_WARNINGS   = -Wextra -pedantic
LINKER          = -lcomctl32 -lgdi32 -luxtheme -lgdiplus FIFOqueue.dll.a

dbg: res debug
	del res.o

rls: res release
	del res.o

debug: $(SRC)
	$(CC) $(CFLAGS) $(DBGFLAGS) -o build/debug.exe $^ $(LINKER)

release: $(SRC) draw.c
	$(CC) $(CFLAGS) $(RLSFLAGS) -o build/SleepingBarberWin.exe $^ $(LINKER)

draw_dll: draw.c
	$(CC) $(CFLAGS) -c $^ -o draw.o
	$(CC) -shared -o build/draw.dll draw.o -lgdi32 -Wl,-no-undefined,--enable-runtime-pseudo-reloc
	@del draw.o

res:
	windres res.rc res.o

.PHONY:
clean:
	@del *.exe *.o
