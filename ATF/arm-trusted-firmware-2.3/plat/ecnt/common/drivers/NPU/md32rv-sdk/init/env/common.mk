# See LICENSE for license details.

ifndef _MK_COMMON
_MK_COMMON := # defined

.PHONY: all
all: $(TARGET)

LLVM_PATH = ../../../toolchain/llvm
LLVMRTLIB = $(LLVM_PATH)/lib/clang/7.0.0/lib/riscv32/$(MRV_VER)/
LLVMNEWLIB = $(LLVM_PATH)/riscv32-elf/$(MRV_VER)/lib/

#include $(INIT_BASE)/libwrap/libwrap.mk

ENV_DIR = $(INIT_BASE)/env
PLATFORM_DIR = $(ENV_DIR)/$(BOARD)

ifeq ($(TCSUPPORT_CPU_ARMV8),1)
CFLAGS += -DTCSUPPORT_CPU_ARMV8
endif
ifeq ($(NPU_CODE_IN_SRAM),1)
CFLAGS += -DNPU_CODE_IN_SRAM
endif
ASM_SRCS += $(ENV_DIR)/start.S
ASM_SRCS += $(ENV_DIR)/entry.S
C_SRCS += $(ENV_DIR)/PLIC.c
C_SRCS += $(ENV_DIR)/timer.c
C_SRCS += $(ENV_DIR)/mbox.c
C_SRCS += $(PLATFORM_DIR)/init.c
C_SRCS += $(PLATFORM_DIR)/rt63365_uart.c
C_SRCS += $(PLATFORM_DIR)/uart.c

LINKER_SCRIPT := $(PLATFORM_DIR)/$(LINK_TARGET).lds

INCLUDES += -I$(INIT_BASE)/include
INCLUDES += -I$(ENV_DIR)
INCLUDES += -I$(PLATFORM_DIR)
INCLUDES += -I$(BOARD_INCLUDE)

# hwknl lib files:
src_dir = ../..
C_SRCS += $(wildcard $(src_dir)/system/*.c) $(wildcard $(src_dir)/api/*.c)
HEADERS  +=  $(wildcard $(src_dir)/init/*.h)
HEADERS  +=  $(wildcard $(src_dir)/include/*.h)
INCLUDES += -I$(src_dir)/include


TOOL_DIR = $(INIT_BASE)/../toolchain/bin

LDFLAGS +=  -Xlinker -T $(LINKER_SCRIPT) -Xlinker -Map=Foo.map  -nostartfiles
LDFLAGS += -L$(ENV_DIR)

#NPU_INIC_CLIENT_SUPPORT=1
ifeq ($(NPU_INIC_CLIENT_SUPPORT),1)
#### NPU iNIC in BL1 just needs the following 3 files ####
ASM_SRCS = $(ENV_DIR)/start.S
C_SRCS = iNIC_client.c iNIC_client_api.c
CFLAGS += -DNPU_INIC_CLIENT_SUPPORT

C_SRCS += ../../../../sgmii/sgmii_api.c
endif

ASM_OBJS := $(ASM_SRCS:.S=.o)
C_OBJS := $(C_SRCS:.c=.o)

LINK_OBJS += $(ASM_OBJS) $(C_OBJS)
LINK_DEPS += $(LINKER_SCRIPT)

CLEAN_OBJS += $(TARGET) $(LINK_OBJS)

CFLAGS += -g
CFLAGS += --target=$(RISCV_TARGET) -mcpu=$(MRV_VER)
#CFLAGS += -march=$(RISCV_ARCH)
CFLAGS += -mabi=$(RISCV_ABI)
#CFLAGS += -mcmodel=medany

$(TARGET): $(LINK_OBJS) $(LINK_DEPS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LINK_OBJS) -o $@ $(LDFLAGS)

$(ASM_OBJS): %.o: %.S $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(C_OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -include sys/cdefs.h -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(CLEAN_OBJS)

endif # _MK_COMMON
