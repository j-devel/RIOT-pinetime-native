all: build

build:
	PATH=./bin/pkg/native/sdl-local/bin:$(PATH) BOARD=native make
	ls -l ./bin/native/PineTime.elf
	ldd ./bin/native/PineTime.elf

clean:
	BOARD=native make clean
