#############################################################
# Configuration
#############################################################

# Allows users to create Makefile.local or ../Makefile.project with
# configuration variables, so they don't have to be set on the command-line
# every time.
extra_configs := $(wildcard Makefile.local ../Makefile.project)
ifneq ($(extra_configs),)
$(info Obtaining additional make variables from $(extra_configs))
include $(extra_configs)
endif

ifneq ($(strip $(RDKB_BUILD)),)
CFLAGS :=
endif

RISCV_TARGET = riscv32
MRV_VER = MRV33E25

# Default target
BOARD ?= fpga
#PROGRAM ?= performance_counters
#PROGRAM ?= dhrystone
PROGRAM ?= main
#PROGRAM ?= hello
ifeq ($(TCSUPPORT_CPU_ARMV8),1)
ifeq ($(NPU_CODE_IN_SRAM),1)
LINK_TARGET ?= 7523_ld_sram
else
LINK_TARGET ?= 7523_ld_script
endif
else
LINK_TARGET ?= flash
endif
GDB_PORT ?= 3333

#############################################################
# BSP Loading
#############################################################

# Finds the directory in which this BSP is located, ensuring that there is
# exactly one.
board_dir := $(wildcard init/env/$(BOARD))
ifeq ($(words $(board_dir)),0)
$(error Unable to find BSP for $(BOARD), expected to find either "bsp/$(BOARD)" or "bsp-addons/$(BOARD)")
endif
ifneq ($(words $(board_dir)),1)
$(error Found multiple BSPs for $(BOARD): "$(board_dir)")
endif

# There must be a settings makefile fragment in the BSP's board directory.
ifeq ($(wildcard $(board_dir)/settings.mk),)
$(error Unable to find BSP for $(BOARD), expected to find $(board_dir)/settings.mk)
endif
include $(board_dir)/settings.mk

ifeq ($(RISCV_ARCH),)
$(error $(board_dir)/board.mk must set RISCV_ARCH, the RISC-V ISA string to target)
endif

ifeq ($(RISCV_ABI),)
$(error $(board_dir)/board.mk must set RISCV_ABI, the ABI to target)
endif

# Determines the XLEN from the toolchain tuple
ifeq ($(patsubst rv32%,rv32,$(RISCV_ARCH)),rv32)
RISCV_XLEN := 32
else
ifeq ($(patsubst rv64%,rv64,$(RISCV_ARCH)),rv64)
RISCV_XLEN := 64
else
$(error Unable to determine XLEN from $(RISCV_ARCH))
endif
endif

#############################################################
# Prints help message
#############################################################
.PHONY: help
help:
	@echo "  ECNT MD32RV Software Development Kit "
	@echo "  Makefile targets:"
	@echo ""
	@echo " software [PROGRAM=$(PROGRAM) BOARD=$(BOARD)]:"
	@echo "    Build a software program to load with the"
	@echo "    debugger."
	@echo ""
	@echo " clean [PROGRAM=$(PROGRAM) BOARD=$(BOARD)]:"
	@echo "    Clean compiled objects for a specified "
	@echo "    software program."
	@echo ""
	@echo " dasm [PROGRAM=$(PROGRAM)]:"
	@echo "     Generates the dissassembly output of 'objdump -d' to stdout."
	@echo ""

#############################################################
# This section is for tool installation
#############################################################
# Pointers to various important tools in the toolchain.
toolchain_builddir := /opt/trendchip/NPU/md32rv_toolchain
toolchain_prefix := $(toolchain_builddir)/llvm

RISCV_PATH ?= $(toolchain_prefix)

RISCV_CLANG     := $(abspath $(RISCV_PATH)/bin/clang)
RISCV_CLANGXX     := $(abspath $(RISCV_PATH)/bin/clangg++)
RISCV_OBJDUMP := $(abspath $(RISCV_PATH)/bin/llvm-objdump)
RISCV_LLDB     := $(abspath $(RISCV_PATH)../lldb/bin/lldb)
RISCV_AR      := $(abspath $(RISCV_PATH)/bin/llvm-ar)
RISCV_OBJCOPY := $(abspath $(RISCV_PATH)/bin/llvm-objcopy)

PATH := $(abspath $(RISCV_PATH)/bin):$(PATH)
BOARD_INCLUDE := $(abspath $(RISCV_PATH)/riscv32-elf/$(MRV_VER)/include)

#############################################################
# This Section is for Software Compilation
#############################################################
PROGRAM_DIR = software/$(PROGRAM)
PROGRAM_ELF = software/$(PROGRAM)/$(PROGRAM)
LIB_WRAP = software/$(PROGRAM)/libwrap.a
NPU_RV32 = npu_rv32
PROGRAM_BIN = software/$(PROGRAM)/$(NPU_RV32).bin
NPU_BIN = $(BOOTROM_UNOPEN_IMG_DIR)/$(NPU_RV32).bin
NPU_DATA = npu_data
DATA_BIN = software/$(PROGRAM)/$(NPU_DATA).bin
NPU_DATA_BIN = $(BOOTROM_UNOPEN_IMG_DIR)/$(NPU_DATA).bin

.PHONY: software_clean
clean: software_clean
software_clean:
	$(MAKE) -C $(PROGRAM_DIR) CC=$(RISCV_CLANG) RISCV_TARGET=$(RISCV_TARGET) MRV_VER=$(MRV_VER) RISCV_ARCH=$(RISCV_ARCH) RISCV_ABI=$(RISCV_ABI) AR=$(RISCV_AR) BSP_BASE=$(abspath init) BOARD=$(BOARD) BOARD_INCLUDE=$(BOARD_INCLUDE) LINK_TARGET=$(LINK_TARGET) clean
	rm -f $(NPU_BIN) $(NPU_DATA_BIN) $(PROGRAM_ELF).elf.txt $(LIB_WRAP) $(PROGRAM_BIN) $(DATA_BIN) software/main/Foo.map

.PHONY: software
software: software_clean
	$(MAKE) -C $(PROGRAM_DIR) CC=$(RISCV_CLANG) RISCV_TARGET=$(RISCV_TARGET) MRV_VER=$(MRV_VER) RISCV_ARCH=$(RISCV_ARCH) RISCV_ABI=$(RISCV_ABI) AR=$(RISCV_AR) BSP_BASE=$(abspath init) BOARD=$(BOARD) BOARD_INCLUDE=$(BOARD_INCLUDE) LINK_TARGET=$(LINK_TARGET)
	$(RISCV_OBJDUMP) -h -S -d -mcpu=$(MRV_VER) $(PROGRAM_ELF) > $(PROGRAM_ELF).elf.txt
	#$(RISCV_OBJCOPY) -R .data -O binary $(PROGRAM_ELF) $(PROGRAM_BIN)
	$(RISCV_OBJCOPY) -j .init -j .text -j .rodata -j .eh_frame -j .lalign -O binary $(PROGRAM_ELF) $(PROGRAM_BIN)
	$(RISCV_OBJCOPY) -j .dalign -j .data -j .bss -j .heap -O binary $(PROGRAM_ELF) $(DATA_BIN)
	mv $(PROGRAM_BIN) $(NPU_BIN)
	mv $(DATA_BIN) $(NPU_DATA_BIN)

dasm: software
	$(RISCV_OBJDUMP) -d --source -mcpu=$(MRV_VER) $(PROGRAM_ELF) | tee $(PROGRAM).lst

