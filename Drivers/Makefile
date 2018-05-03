CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
OBJCOPY=arm-none-eabi-objcopy
OBJDUMP=arm-none-eabi-objdump
SIZE=arm-none-eabi-size
RANLIB=arm-none-eabi-ranlib

CFLAGS += -DSTM32L433xx -DUSE_FULL_LL_DRIVER

CFLAGS += -mfloat-abi=soft
CFLAGS += -Wall -g -std=c99 -Os -fno-strict-aliasing
CFLAGS += -mlittle-endian -mcpu=cortex-m4 -mthumb

CFLAGS+=-ICMSIS/Include
CFLAGS+=-ICMSIS/Device/ST/STM32L4xx/Include
CFLAGS+=-ISTM32L4xx_HAL_Driver/Inc
CFLAGS+=-I.

LLSRC = $(wildcard STM32L4xx_HAL_Driver/Src/*_ll_*.c)
LLOBJ = $(LLSRC:.c=.o)

.PHONY: libll.a

all: libll.a

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^


libll.a: $(LLOBJ)
	$(AR) -r $@ $(LLOBJ)
	$(RANLIB) $@
clean:
	rm -f $(LLOBJ) libll.a

