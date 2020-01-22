# project details
DEVICE  = atmega4809
FUSES   = 2:0x02
F_CPU   = 20000000	# in Hz
OUT     = alphatester

# source files to compile
OBJ     = main.o



HOST_ARCH         := $(shell uname -m)
UNAME             := $(shell uname)
ifeq ($(UNAME),Darwin)
PYUPDI_PORT       := /dev/cu.usbserial*
HOST_OS           := darwin
else
ifeq ($(UNAME),Linux)
PYUPDI_PORT       := /dev/ttyUSB*
HOST_OS           := linux
else
$(error Operating system "$(UNAME)" not currently supported by this Makefile)
endif
endif

# parent directory of the toolchain, device pack, and pyupdi directories
AVR_BASE_DIR      := /usr/local
AVR_TOOLCHAIN_DIR := $(AVR_BASE_DIR)/avr8-gnu-toolchain-$(HOST_OS)_$(HOST_ARCH)
AVR_DFP_DIR       := $(lastword $(sort $(wildcard $(AVR_BASE_DIR)/Atmel.ATmega_DFP*)))
PYUPDI_DIR        := $(AVR_BASE_DIR)/pyupdi

PYUPDI_DEVICE = $(subst atmega,mega,$(DEVICE))
PYUPDI   = $(PYUPDI_DIR)/pyupdi.py -d $(PYUPDI_DEVICE) -c $(PYUPDI_PORT) -b 115200 -v

CFLAGS_COMMON = -Os -B $(AVR_DFP_DIR)/gcc/dev/$(DEVICE) -I $(AVR_DFP_DIR)/include -Wall -DF_CPU=$(F_CPU) -mmcu=$(DEVICE) -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -fdata-sections -ffunction-sections -mrelax -MMD -MP -Wa,-adhlns=$(OUT).lst
LDFLAGS = -Wl,-Map,$(OUT).map -Wl,--relax
CFLAGS   = $(CFLAGS_COMMON) -std=gnu11
CXXFLAGS = $(CFLAGS_COMMON) -std=gnu++14 -fno-exceptions -fno-non-call-exceptions -fno-rtti -fno-use-cxa-atexit
CC       = $(AVR_TOOLCHAIN_DIR)/bin/avr-gcc
CXX      = $(AVR_TOOLCHAIN_DIR)/bin/avr-g++
OBJCOPY  = $(AVR_TOOLCHAIN_DIR)/bin/avr-objcopy
SIZE     = $(AVR_TOOLCHAIN_DIR)/bin/avr-size

DEPS     = $(OBJ:.o=.d)

.PHONY: all hex program fuse flash clean cpp

all: hex

hex: $(OUT).hex

program: fuse flash

# rule for programming fuse bits:
fuse:
	$(PYUPDI) --fuses $(FUSES)

# rule for uploading firmware:
flash: $(OUT).hex
	$(PYUPDI) -f $(OUT).hex

# rule for deleting dependent files (those which can be built by Make):
clean:
	rm -f $(OUT).hex $(OUT).lst $(OUT).obj $(OUT).map $(OUT).eep.hex $(OUT).elf *.o *.d

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.S
	$(CC) -x assembler-with-cpp -c $< -o $@

%.o: %.s
	$(CC) -S $< -o $@

# file targets:

$(OUT).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT).elf $(OBJ)

$(OUT).hex: $(OUT).elf
	rm -f $(OUT).hex $(OUT).eep.hex
	$(OBJCOPY) -j .text -j .data -O ihex $(OUT).elf $(OUT).hex
	$(SIZE) $(OUT).hex

# debugging targets:

cpp:
	$(CC) -E $(SRC)

-include $(DEPS)

