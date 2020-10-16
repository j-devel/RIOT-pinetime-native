USEMODULE += gui
USEMODULE += controller
USEMODULE += widget
USEMODULE += fonts
ifneq ($(BOARD),native)
  USEMODULE += bleman
  USEMODULE += hal
  USEMODULE += storage
endif
USEMODULE += util

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += schedstatistics

ifeq ($(BOARD),native)
  CFLAGS += -DUSE_BOARD_NATIVE
  USEMODULE += lvgl_sdl
endif
