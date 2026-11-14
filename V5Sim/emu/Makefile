# Builds the fake VEXos (v5emu.elf) that QEMU boots before the PROS program.
CC = arm-none-eabi-gcc
CFLAGS = -mcpu=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=hard -marm -O2 -g -ffreestanding \
         -fno-builtin-printf -Wall -Wno-misleading-indentation -Wno-unused-function
LDFLAGS = -T link.ld -nostartfiles -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs

v5emu.elf: start.S vexos.c link.ld jt_names.h trampolines.h
	$(CC) $(CFLAGS) $(LDFLAGS) start.S vexos.c -lm -o $@

clean:
	rm -f v5emu.elf
