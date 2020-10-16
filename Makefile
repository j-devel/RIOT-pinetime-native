all: native run

init:
	git submodule init
	git submodule update

MK_BOARD_NATIVE = make -C apps/pinetime -f native.mk
MK_BOARD_PINETIME = make -C apps/pinetime
CHECK_RIOT = [ -f ./RIOT/README.md ]
BYE = echo 'Maybe try \`make init\` first?' && exit 1
native:  # with MCU "native"
	if $(CHECK_RIOT); then $(MK_BOARD_NATIVE); else $(BYE); fi
pinetime:  # with MCU "nrf52"
	if $(CHECK_RIOT); then $(MK_BOARD_PINETIME); else $(BYE); fi

run:
	./apps/pinetime/bin/native/PineTime.elf
