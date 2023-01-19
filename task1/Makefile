COMPILER=gcc
OUTPUT=a.out
FLAGS=-std=gnu11 -O0
APPEND_FLAGS=

all: native

native: build_native
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) main.o mul.o -o $(OUTPUT) && rm -rf *.o

build_native: main.c src/common.h src/native.c
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) -c main.c
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) -c src/native.c -o mul.o



native_m: build_native_m
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) main.o mul.o -o $(OUTPUT) && rm -rf *.o

build_native_m: main.c src/common.h src/native_m.c
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) -c main.c
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) -c src/native_m.c -o mul.o



block: build_block
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) main.o mul.o -o $(OUTPUT) && rm -rf *.o
build_block: main.c src/common.h src/block.c
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) -c main.c
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) -c src/block.c -o mul.o



block_m: build_block_m
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) main.o mul.o -o $(OUTPUT) && rm -rf *.o
build_block_m: main.c src/common.h src/block_m.c
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) -c main.c
	$(COMPILER) $(FLAGS) $(APPEND_FLAGS) -c src/block_m.c -o mul.o

clean:
	rm -rf *.o $(OUTPUT)