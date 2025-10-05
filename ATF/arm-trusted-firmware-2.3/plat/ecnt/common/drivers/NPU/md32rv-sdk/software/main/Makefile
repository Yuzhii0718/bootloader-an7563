TARGET = main
C_SRCS += main.c
C_SRCS += core0_main.c core1_main.c core2_main.c core3_main.c core4_main.c core5_main.c core6_main.c core7_main.c
CFLAGS += -O2 -fno-builtin-printf

#CFLAGS += -DCORE_BEATS_MONITOR_SUPPORT
ifeq ($(TCSUPPORT_CPU_EN7523),1)
CFLAGS += -DTCSUPPORT_CPU_EN7523
#INTEL_E1000E_SUPPORT=1
ifeq ($(INTEL_E1000E_SUPPORT),1)
CFLAGS += -DINTEL_E1000E_SUPPORT
C_SRCS += e1000e.c
endif
endif

#TCSUPPORT_NPU_OFFLOAD=1
ifeq ($(TCSUPPORT_NPU_QDMA),1)
CFLAGS += -DTCSUPPORT_NPU_QDMA
C_SRCS += qdma.c
endif
ifeq ($(TCSUPPORT_NPU_OFFLOAD),1)
CFLAGS += -DTCSUPPORT_NPU_OFFLOAD
C_SRCS += 7615_offload.c 7615_enqueue.c
endif
#NPU_RAMS_TEST=1
ifeq ($(NPU_RAMS_TEST),1)
CFLAGS += -DNPU_RAMS_TEST
C_SRCS += test_code/dramtest.c
endif

ifeq ($(NPU_TEST_CODE),1)
CFLAGS += -DNPU_TEST_CODE
C_SRCS += test_code/npu_test.c test_code/hw_kernel_thread_test.c test_code/hw_kernel_mutex_test.c test_code/access_test.c test_code/gdma_util.c
C_SRCS += test_code/PLIC_Mbox_test.c test_code/misc_test.c
ifeq ($(TCSUPPORT_CPU_EN7523),1)
C_SRCS += test_code/hsdma_v2_util.c
else
C_SRCS += test_code/hsdma_util.c
endif
endif

INIT_BASE = ../../init
include $(INIT_BASE)/env/common.mk
