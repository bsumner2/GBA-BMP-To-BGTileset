CC=clang
LD=clang
GENERAL_FLAGS=-std=c11 -Wall -Wextra -pedantic -D_REENTRANT -g
CFLAGS=$(GENERAL_FLAGS) $(INCS)
LDFLAGS=$(GENERAL_FLAGS) $(LIBS)
INCS=-I./include ${shell pkg-config --cflags libmd sdl2}
LIBS=${shell pkg-config --libs libmd sdl2}
EXE=test.elf
OBJS=${shell find ./src -type f -iname '*.c' | sed 's-\./src-\./bin-g' | sed 's-\.c-\.o-g'}

.PHONY: clean build run


def: bin clean build

bin:
	mkdir -p bin

clean: bin
	rm -f ./bin/*?.{e,o}*

build: clean $(EXE)

run: build

$(EXE): $(OBJS)
	$(LD) $^ -o ./bin/$@ $(LDFLAGS)

$(OBJS): ./bin/%.o : ./src/%.c
	$(CC) $< -c $(CFLAGS) -o $@


