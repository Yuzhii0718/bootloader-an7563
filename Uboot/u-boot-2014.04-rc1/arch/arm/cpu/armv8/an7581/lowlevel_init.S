#include <asm-offsets.h>
#include <asm/assembler.h>

/* copy from modules/private/arm_secure_upgrade/smccc-call-64.S */

#define ARM_SMCCC_QUIRK_QCOM_A6		1 /* Save/restore register a6 */
#define ARM_SMCCC_RES_X0_OFFS		0
#define ARM_SMCCC_RES_X2_OFFS		16
#define ARM_SMCCC_QUIRK_ID_OFFS		8

	.macro SMCCC instr
	.cfi_startproc
	\instr  #0
	ldr x4, [sp]
	stp x0, x1, [x4, #ARM_SMCCC_RES_X0_OFFS]
	stp x2, x3, [x4, #ARM_SMCCC_RES_X2_OFFS]
	ldr x4, [sp, #8]
	cbz x4, 1f /* no quirk structure */
	ldr x9, [x4, #0]
	cmp x9, #ARM_SMCCC_QUIRK_QCOM_A6
	b.ne    1f
	str x6, [x4, ARM_SMCCC_QUIRK_ID_OFFS]
1:  ret
	.cfi_endproc
	.endm
	
.globl do_smc
do_smc:
	SMCCC   smc
