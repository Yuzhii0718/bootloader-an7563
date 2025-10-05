/*
 *  crt0 - C-runtime startup Code for ARM U-Boot
 *
 *  Copyright (c) 2012  Albert ARIBAUD <albert.u.boot@aribaud.net>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <asm-offsets.h>
#include <linux/linkage.h>

/*
 * This file handles the target-independent stages of the U-Boot
 * start-up where a C runtime environment is needed. Its entry point
 * is _main and is branched into from the target's start.S file.
 *
 * _main execution sequence is:
 *
 * 1. Set up initial environment for calling board_init_f().
 *    This environment only provides a stack and a place to store
 *    the GD ('global data') structure, both located in some readily
 *    available RAM (SRAM, locked cache...). In this context, VARIABLE
 *    global data, initialized or not (BSS), are UNAVAILABLE; only
 *    CONSTANT initialized data are available.
 *
 * 2. Call board_init_f(). This function prepares the hardware for
 *    execution from system RAM (DRAM, DDR...) As system RAM may not
 *    be available yet, , board_init_f() must use the current GD to
 *    store any data which must be passed on to later stages. These
 *    data include the relocation destination, the future stack, and
 *    the future GD location.
 *
 * (the following applies only to non-SPL builds)
 *
 * 3. Set up intermediate environment where the stack and GD are the
 *    ones allocated by board_init_f() in system RAM, but BSS and
 *    initialized non-const data are still not available.
 *
 * 4. Call relocate_code(). This function relocates U-Boot from its
 *    current location into the relocation destination computed by
 *    board_init_f().
 *
 * 5. Set up final environment for calling board_init_r(). This
 *    environment has BSS (initialized to 0), initialized non-const
 *    data (initialized to their intended value), and stack in system
 *    RAM. GD has retained values set by board_init_f(). Some CPUs
 *    have some work left to do at this point regarding memory, so
 *    call c_runtime_cpu_setup.
 *
 * 6. Branch to board_init_r().
 */

/*
 * entry point of crt0 sequence
 */

ENTRY(_main)

/*
 * Set up initial C runtime environment and call board_init_f(0).
 */

#if defined(CONFIG_ECNT) && defined(CONFIG_SPL_BUILD) && !defined(CONFIG_TPL_BUILD)
/****************************************************************/
	#ifndef BOOT_ON_L2C_SRAM /* for qdma blkCnt test, code needs to run on l2c_sram, instead of fe_sram */
	/* Enable fe_Sram */
	ldr		r7, =0x1FB00958
	mov		r1, #1
	str		r1, [r7]
	#endif
/****************************************************************/
	adr		lr, sram_continue
	ldr		r1, =_start
	#ifdef BOOT_ON_L2C_SRAM
	ldr		r0,	=0x1EFC0000 /* l2c_sram */
	#else
	ldr		r0,	=0x1F000000
	#endif
	sub		r0, r0, r1
	add		lr, lr, r0
	#ifdef BOOT_ON_L2C_SRAM
	ldr		r0, =0x1EFC0000			/* r0 <- CONFIG_SYS_TEXT_BASE */
	#else
	ldr		r0, =0x1F000000			/* r0 <- CONFIG_SYS_TEXT_BASE */
	#endif
	ldr		r1, =__image_copy_start	/* r1 <- SRC &__image_copy_start */
	ldr		r2, =__bss_end			/* r2 <- SRC &__image_copy_end */
	ldr		r3, =copy_sram

copy_sram:
	ldmia	r1!, {r10-r11}			/* copy from source address [r1]    */
	stmia	r0!, {r10-r11}			/* copy to   target address [r0]    */
	cmp		r1, r2					/* until source end address [r2]    */
	bxlo	r3

	bx        lr
/****************************************************************/
sram_continue:
	ldr		r7, =0x1FA10114			/* HW TRAP */
	ldr		r4, [r7]
	and		r5, r4, #1				/* Store Information=> 0: 3Byte Mode , 1: 4Byte Mode */
	and		r6, r4, #2				/* Store Information=> 0: SPI NOR , 1: SPI NAND */
	mov		r0, #11					/*	PAGE _SIZE 2K */

	ldr		r1, =0x800				/* flash_start */
	ldr		r2,	=0x1FFFF			/* flash_end */
	#ifdef BOOT_ON_L2C_SRAM
	ldr		r3,	=0x1EFC0800			/* sram address */
	ldr		r4,	=0x1EFC0800			/* jump function */
	#else
	ldr		r3,	=0x1F000800			/* sram address */
	ldr		r4,	=0x1F000800			/* jump function */
	#endif

	b		move_data_begin
/****************************************************************/
#else
#if defined(CONFIG_ECNT) && !defined(CONFIG_SPL_BUILD) && !defined(CONFIG_TPL_BUILD)
/****************************************************************/
	/* Disable Sram */
	ldr		r7, =0x1FB00958
	mov		r1, #0
	str		r1, [r7]
/****************************************************************/
#endif
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_STACK)
	ldr	sp, =(CONFIG_SPL_STACK)
#else
	ldr	sp, =(CONFIG_SYS_INIT_SP_ADDR)
#endif
	bic	sp, sp, #7	/* 8-byte alignment for ABI compliance */
	sub	sp, sp, #GD_SIZE	/* allocate one GD above SP */
	bic	sp, sp, #7	/* 8-byte alignment for ABI compliance */
	mov	r9, sp		/* GD is above SP */
	mov	r0, #0

	bl	board_init_f
#endif

#if ! defined(CONFIG_SPL_BUILD)

/*
 * Set up intermediate environment (new sp and gd) and call
 * relocate_code(addr_moni). Trick here is that we'll return
 * 'here' but relocated.
 */

	ldr	sp, [r9, #GD_START_ADDR_SP]	/* sp = gd->start_addr_sp */
	bic	sp, sp, #7	/* 8-byte alignment for ABI compliance */
	ldr	r9, [r9, #GD_BD]		/* r9 = gd->bd */
	sub	r9, r9, #GD_SIZE		/* new GD is below bd */

	adr	lr, here
	ldr	r0, [r9, #GD_RELOC_OFF]		/* r0 = gd->reloc_off */
	add	lr, lr, r0
	ldr	r0, [r9, #GD_RELOCADDR]		/* r0 = gd->relocaddr */
	b	relocate_code
here:

/* Set up final (full) environment */

	bl	c_runtime_cpu_setup	/* we still call old routine here */

	ldr	r0, =__bss_start	/* this is auto-relocated! */
	ldr	r1, =__bss_end		/* this is auto-relocated! */

	mov	r2, #0x00000000		/* prepare zero to clear BSS */

clbss_l:cmp	r0, r1			/* while not at end of BSS */
	strlo	r2, [r0]		/* clear 32-bit BSS word */
	addlo	r0, r0, #4		/* move to next */
	blo	clbss_l

	bl coloured_LED_init
	bl red_led_on

	/* call board_init_r(gd_t *id, ulong dest_addr) */
	mov     r0, r9                  /* gd_t */
	ldr	r1, [r9, #GD_RELOCADDR]	/* dest_addr */
	/* call board_init_r */
	ldr	pc, =board_init_r	/* this is auto-relocated! */

	/* we should not return here. */

#endif

ENDPROC(_main)

#if defined(CONFIG_ECNT) && defined(CONFIG_SPL_BUILD) && !defined(CONFIG_TPL_BUILD)
ENTRY(move_data_begin)

#if 0
	/* set Tn to 1	*/
	ldr		r10, =0x1FA20068
	mov		r11, #9
	str		r11, [r10]
#endif
	/* Disable read idle enable */
	ldr		r10, =0x1FA10004
	mov		r11, #0
	str		r11, [r10]

fsm_idle:
	/*  wait until auto read status is IDLE - start */
	ldr		r10, =0x1FA10018
	ldr		r11, [r10]
	cmp		r11, #0
	bne		fsm_idle

	/* manual begin */
	ldr		r10, =0x1FA10014
	mov		r11, #9
	str		r11, [r10]

	ldr		r10, =0x1FA10020
	mov		r11, #1
	str		r11, [r10]

START_LOAD:
	/* 0: SPI NOR , 1: SPI NAND */
	cmp 	r6, #0
	beq		READ_DATA

LOAD_PAGE:
	/* If S1 > S2, goto LOAD_DONE */
	cmp		r1, r2
	bgt		LOAD_DONE

	/* 1. CSL : Send Chip Select Low command */
	mov		r8, #1
	mov		r9, #1
	bl		send_opfifo_write_cmd

	/* 2. Send 13h OP code */
	mov		r8, #8
	mov		r9, #1
	bl		send_opfifo_write_cmd

	mov		r8, #0x13
	bl		send_datafifo_write_cmd

PAGE_NUMBER:
	/* 3. Caculate the Page number */
	mov		r7, r1, lsr r0

	/* 4. Send _SPI_NAND_OP_PAGE_READ 13H */
	mov		r8, #8
	mov		r9, #3
	bl		send_opfifo_write_cmd

	lsr		r8, r7, #16
	and 	r8, r8, #0xFF
	bl		send_datafifo_write_cmd

	lsr		r8, r7, #8
	and 	r8, r8, #0xFF
	bl		send_datafifo_write_cmd

	and 	r8, r7, #0xFF
	bl		send_datafifo_write_cmd

	/* 5. CSH : Send Chip Select High command */
	mov		r8, #0
	mov		r9, #1
	bl		send_opfifo_write_cmd

GET_FEATURE:
	/* 1. CSL : Send Chip Select Low command */
	mov		r8, #1
	mov		r9, #1
	bl		send_opfifo_write_cmd

	/* 2. Send _SPI_NAND_OP_GET_FEATURE 0Fh */
	mov		r8, #8
	mov		r9, #1
	bl		send_opfifo_write_cmd

	mov		r8, #0xF
	bl		send_datafifo_write_cmd

	/* 3. Send _SPI_NAND_ADDR_STATUS C0h */
	mov		r8, #8
	mov		r9, #1
	bl		send_opfifo_write_cmd

	mov		r8, #0xC0
	bl		send_datafifo_write_cmd

	/* 4. Read Status */
	mov		r8, #12
	mov		r9, #1
	bl		send_opfifo_write_cmd
	bl		send_datafifo_read_cmd

	and		r7, r8, #1

	cmp		r7, #0
	bne		GET_FEATURE

	/* 5. CSH : Send Chip Select High command */
	mov		r8, #0
	mov		r9, #1
	bl		send_opfifo_write_cmd

READ_DATA:
	/* If S1 > S2, goto LOAD_DONE */
	cmp		r1, r2
	bgt		LOAD_DONE

	mov		r7, r1

	/* 0: SPI NOR , 1: SPI NAND */
	cmp 	r6, #0
	beq		ADDR_OPCODE

	mov		r10, #1
	mov		r10, r10, lsl r0
	sub		r10, r10, #1

	and		r7, r7, r10

ADDR_OPCODE:
	/* 1. CSL : Send Chip Select Low command */
	mov		r8, #1
	mov		r9, #1
	bl		send_opfifo_write_cmd

	/* 2. Send _SPI_NAND_OP_READ_FROM_CACHE_SINGLE 03h */
	mov		r8, #8
	mov		r9, #1
	bl		send_opfifo_write_cmd

	mov		r8, #3
	bl		send_datafifo_write_cmd

SEND_ADDR:
	/* 3. Send Read Address */
	cmp 	r5, #0
	beq		THREE_BYTE_ADDR
	cmp 	r6, #1
	beq		THREE_BYTE_ADDR

	mov		r8, #8
	mov		r9, #1
	bl		send_opfifo_write_cmd

	lsr		r8, r7, #24
	and 	r8, r8, #0xFF
	bl		send_datafifo_write_cmd

THREE_BYTE_ADDR:
	mov		r8, #8
	mov		r9, #3
	bl		send_opfifo_write_cmd

	/* 0: SPI NOR , 1: SPI NAND */
	cmp 	r6, #0
	bne		DUMMY_APPEND

	lsr		r8, r7, #16
	and 	r8, r8, #0xFF
	bl		send_datafifo_write_cmd

DUMMY_APPEND:
	lsr		r8, r7, #8
	and 	r8, r8, #0xFF
	bl		send_datafifo_write_cmd

	and 	r8, r7, #0xFF
	bl		send_datafifo_write_cmd

	/* 0: SPI NOR , 1: SPI NAND */
	cmp 	r6, #0
	beq		read_one_byte

	mov		r8, #0
	bl		send_datafifo_write_cmd

read_one_byte:
	/* If S1 > S2, READ_PAGE_DONE */
	cmp		r1, r2
	bgt		READ_PAGE_DONE

	mov		r8, #12
	mov		r9, #1
	bl		send_opfifo_write_cmd
	bl		send_datafifo_read_cmd

	strb	r8, [r3]

	add		r1, r1, #1
	add		r3, r3, #1
	add		r7, r7, #1

	mov		r10, #1
	mov		r10, r10, lsl r0
	sub		r10, r10, #1

	and		r8, r7, r10
	cmp		r8, #0
	bne		read_one_byte

READ_PAGE_DONE:
	/* 5. CSH : Send Chip Select High command */
	mov		r8, #0
	mov		r9, #1
	bl		send_opfifo_write_cmd
	b		START_LOAD

LOAD_DONE:
	bx		r4

ENDPROC(move_data_begin)

ENTRY(send_opfifo_write_cmd)

	/* (((op_cmd) & OP_CMD_MASK) << OP_SHIFT) | ((op_len) & OP_LEN_MASK) */
	/* write op_cmd to register OPFIFO_WDATA */
	ldr		r10, =0x1FA10028
	ldr		r11, =0x1F
	and 	r11, r8, r11
	lsl		r11, r11, #0x9
	ldr		r12, =0x1FF
	and 	r12, r9, r12
	orr		r11, r11, r12
	str		r11, [r10]

opfifo_full_check:
	/* wait until opfifo is not full */
	ldr		r10, =0x1FA1002C
	ldr		r11, [r10]
	cmp 	r11, #0
	bne		opfifo_full_check

	/* enable write from register OPFIFO_WDATA to opfifo */
	ldr		r10, =0x1FA10030
	mov		r11, #1
	str		r11, [r10]

opfifo_empty_check:
	/* wait until opfifo is empty */
	ldr		r10, =0x1FA10024
	ldr		r11, [r10]
	cmp 	r11, #1
	bne		opfifo_empty_check

	bx		r14

ENDPROC(send_opfifo_write_cmd)


ENTRY(send_datafifo_read_cmd)

dfifo_nonempty_check:
	/* wait until dfifo is not empty */
	ldr		r10, =0x1FA1003C
	ldr		r11, [r10]
	cmp 	r11, #0
	bne		dfifo_nonempty_check

	/* read 1 byte from dfifo to register DFIFO_RDATA */
	ldr		r10, =0x1FA10044
	ldr	 	r8, [r10]
	and 	r8, r8, #0xFF

	/* enable register DFIFO_RD to read next byte */
	ldr		r10, =0x1FA10040
	mov		r11, #1
	str		r11, [r10]

	bx		r14

ENDPROC(send_datafifo_read_cmd)


ENTRY(send_datafifo_write_cmd)

dfifo_full_check:
	/* wait until dfifo is not full */
	ldr		r10, =0x1FA10034
	ldr		r11, [r10]
	cmp 	r11, #0
	bne		dfifo_full_check

	/* write data  to register DFIFO_WDATA */
	ldr		r10, =0x1FA10038
	and 	r11, r8, #0xFF
	str		r11, [r10]

	bx		r14

ENDPROC(send_datafifo_write_cmd)
#endif
