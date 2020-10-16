# Enable the MPU to detect stack overflows
ifneq ($(BOARD),native)
USEMODULE += mpu_stack_guard
endif
