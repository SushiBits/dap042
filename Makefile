#!/usr/bin/make -F
TARGET := dap042.elf
TARGET_HEX := $(TARGET:.elf=.ihx)
TARGET_BIN := $(TARGET:.elf=.bin)
TARGETS := $(TARGET) $(TARGET_HEX) $(TARGET_BIN)

ARCH := arm-none-eabi-
CC := $(ARCH)gcc
CXX := $(ARCH)g++
AS := $(ARCH)as
CCAS := $(CC) -x assembler-with-cpp
CCLD := $(CC)
CXXLD := $(CXX)
OBJCOPY := $(ARCH)objcopy

GENHDRS := $(patsubst %.h,%_handlers.h,$(wildcard contrib/platform/include/stm32f0??x?.h))

SRC_PATHS := src contrib/libusb_stm32/src contrib/platform/src
C_FILES := $(foreach SRC_PATH,$(SRC_PATHS),$(wildcard $(SRC_PATH)/*.c))
CXX_FILES := $(foreach EXTENSION,cc cpp,$(foreach SRC_PATH,$(SRC_PATHS),$(wildcard $(SRC_PATH)/*.$(EXTENSION))))
S_FILES := $(foreach EXTENSION,S s,$(foreach SRC_PATH,$(SRC_PATHS),$(wildcard $(SRC_PATH)/*.$(EXTENSION))))
SRCS := $(C_FILES) $(CXX_FILES) $(S_FILES)
OBJS := $(foreach SRC,$(SRCS),$(SRC).o)
DEPS := $(OBJS:.o=.d)

CPUFLAGS := -mcpu=cortex-m0 -mthumb
CPPFLAGS := -Iinclude -Icontrib/platform/include -Icontrib/libusb_stm32/inc -Icontrib/cmsis/CMSIS/Core/Include -DSTM32F042x6 -DHSE_VALUE=12000000
CFLAGS := $(CPUFLAGS) -std=gnu17 -MMD -MP -Og -g -ffunction-sections -fdata-sections -Wall
CXXFLAGS := $(CPUFLAGS) -std=gnu++17 -MMD -MP -Og -g -ffunction-sections -fdata-sections -Wall
ASFLAGS := $(CPUFLAGS) -MMD -MP -g -ffunction-sections -fdata-sections -Wall
LDFLAGS := $(CPUFLAGS) -Llib -Lcontrib/platform/lib -Wl,-Tdap042.ld -Wl,--gc-sections # Platform code include a linker script fragment.

all: headers $(TARGET)
hex: $(TARGET_HEX)
bin: $(TARGET_BIN)
headers: $(GENHDRS)

contrib/platform/include/%_handlers.h: contrib/platform/include/%.h
	bash contrib/platform/share/irqn.sh "\"stm32f0xx_it.h\"" < $< > $@

%.c.o: %.c $(GENHDRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.cc.o: %.cc $(GENHDRS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

%.cpp.o: %.cpp $(GENHDRS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

%.S.o: %.S $(GENHDRS)
	$(CCAS) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

%.s.o: %.s $(GENHDRS)
	$(AS) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(if $(CXX_FILES),$(CXXLD),$(CCLD)) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

%.ihx: %.elf
	$(OBJCOPY) -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

clean:
	-rm -rf $(TARGETS) $(DEPS) $(OBJS)

distclean: clean
	-rm -rf $(GENHDRS)

-include $(DEPS)
