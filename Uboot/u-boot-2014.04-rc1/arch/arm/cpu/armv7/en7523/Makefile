#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
obj-y	:= lowlevel_init.o
obj-y	+= init.o reset.o
obj-y	+= ecnt_timer.o
obj-y	+= ecnt_uart.o
obj-y	+= ecnt_gpio.o
obj-y	+= ecnt_system.o
obj-$(CONFIG_USE_IRQ)	+= gic.o
obj-$(CONFIG_USE_IRQ)	+= interrupts.o
ifneq ($(strip $(TCSUPPORT_CPU_EN7523)),)
obj-y += cpu_bus_test.o
endif
obj-$(CONFIG_SPL_BUILD) += spl.o
