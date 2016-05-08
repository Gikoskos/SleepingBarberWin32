CC              = gcc
CFLAGS          = -std=c11
DBGFLAGS        = -Wall -g -ggdb -D_DEBUG
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
	$(CC) $(CFLAGS) $(DBGFLAGS) -shared -o build/draw.dll $^ -lgdi32 -Wl,-no-undefined,--enable-runtime-pseudo-reloc

res:
	windres res.rc res.o

.PHONY:
clean:
	@del *.exe *.o
