# put your *.o targets here, make should handle the rest!

SRCS = system_stm32l4xx.c main.c i2c.c accel.c led.c interrupt.c display.c
OBJ = $(SRCS:.c=.o)

# all the files will be generated with this name (main.elf, main.bin, main.hex, etc)
PROJ_NAME=project

OUT_DIR=Output

# Location of the Libraries folder from the STM32F0xx Standard Peripheral Library
LL_LIB=Drivers

# Location of the linker scripts
LDSCRIPT_INC=Device/ldscripts

# that's it, no need to change anything below this line!

###################################################

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
OBJDUMP=arm-none-eabi-objdump
SIZE=arm-none-eabi-size

CFLAGS  = -Wall -g -std=gnu99 -Og
CFLAGS += -DSTM32L433xx -DUSE_FULL_LL_DRIVER 
CFLAGS += -mlittle-endian -mcpu=cortex-m4  -mthumb
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wl,--gc-sections -Wl,-Map=$(OUT_DIR)/$(PROJ_NAME).map
CFLAGS += -Werror -Wstrict-prototypes -Warray-bounds -fno-strict-aliasing -Wno-unused-const-variable
#-Wextra
###################################################

vpath %.c src
vpath %.a .

ROOT=$(shell pwd)

CFLAGS += -I $(LL_LIB) -I $(LL_LIB)/CMSIS/Device/ST/STM32L4xx/Include
CFLAGS += -I $(LL_LIB)/CMSIS/Include -I $(LL_LIB)/STM32L4xx_HAL_Driver/Inc -I src
CFLAGS += -I./inc

# add startup file to build
SRCS += ./src/startup_stm32l433xx.s 

OBJS = $(SRCS:.c=.o)

###################################################

.PHONY: lib proj

all: proj

proj: 	$(OUT_DIR)/$(PROJ_NAME).elf

%.o: %.c
	$(CC) $(CFLAGS) -c -o src/$@ $<

$(LL_LIB)/libll.a:
	cd $(LL_LIB) && make	

$(OUT_DIR)/$(PROJ_NAME).elf: $(SRCS) $(LL_LIB)/libll.a
	mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $^ -o $@ -L$(LL_LIB) -lll -L$(LDSCRIPT_INC) -lm -TSTM32L433CCTx_FLASH.ld
	$(OBJCOPY) -O ihex $(OUT_DIR)/$(PROJ_NAME).elf $(OUT_DIR)/$(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(OUT_DIR)/$(PROJ_NAME).elf $(OUT_DIR)/$(PROJ_NAME).bin
	$(OBJDUMP) -St $(OUT_DIR)/$(PROJ_NAME).elf >$(OUT_DIR)/$(PROJ_NAME).lst
	$(SIZE) $(OUT_DIR)/$(PROJ_NAME).elf

#	$(SIZE) -A $(OUT_DIR)/$(PROJ_NAME).elf
		
clean:
	find ./ -name '*~' | xargs rm -f	
	rm -f *.o
	rm -f src/*.o 
	rm -f $(OUT_DIR)/$(PROJ_NAME).elf
	rm -f $(OUT_DIR)/$(PROJ_NAME).hex
	rm -f $(OUT_DIR)/$(PROJ_NAME).bin
	rm -f $(OUT_DIR)/$(PROJ_NAME).map
	rm -f $(OUT_DIR)/$(PROJ_NAME).lst

