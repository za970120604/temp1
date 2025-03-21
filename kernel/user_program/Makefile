SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

ASMS = $(wildcard *.S)
ASM_OBJS = $(ASMS:.S=.o)


CFLAGS = -Wall -Wextra -Wpedantic -O0 -ffreestanding -nostdinc -nostdlib -nostartfiles -g

all: user_program.img

%.o: %.S
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

user_program.img: $(ASM_OBJS) $(OBJS)
	aarch64-linux-gnu-ld $(ASM_OBJS) $(OBJS) -o user_program.elf
	aarch64-linux-gnu-objcopy -O binary user_program.elf user_program.img
	cp ./user_program.img ../rootfs/

clean:
	rm *.elf *.img *.o >/dev/null 2>/dev/null || true