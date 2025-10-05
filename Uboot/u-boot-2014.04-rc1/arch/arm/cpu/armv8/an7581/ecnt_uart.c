#include <serial.h>
#include <common.h>
#include <config.h>
#include <asm/io.h>

#include <asm/arch/typedefs.h>
#include <asm/arch/en7523.h>
#include <asm/arch/ecnt_uart.h>

/*************************
 * UART Module Registers *
 *************************/
#define	CR_UART_BASE		0x1FBF0000
#define	CR_UART_RBR			(CR_UART_BASE+0x00)
#define	CR_UART_THR			(CR_UART_BASE+0x00)
#define	CR_UART_IER			(CR_UART_BASE+0x04)
#define	CR_UART_IIR			(CR_UART_BASE+0x08)
#define	CR_UART_FCR			(CR_UART_BASE+0x08)
#define	CR_UART_LCR			(CR_UART_BASE+0x0c)
#define	CR_UART_MCR			(CR_UART_BASE+0x10)
#define	CR_UART_LSR			(CR_UART_BASE+0x14)
#define	CR_UART_MSR			(CR_UART_BASE+0x18)
#define	CR_UART_SCR			(CR_UART_BASE+0x1c)
#define	CR_UART_BRDL		(CR_UART_BASE+0x00)
#define	CR_UART_BRDH		(CR_UART_BASE+0x04)
#define	CR_UART_WORDA		(CR_UART_BASE+0x20)
#define	CR_UART_MISCC		(CR_UART_BASE+0x24)
#define	CR_UART_HWORDA		(CR_UART_BASE+0x28)
#define	CR_UART_XYD			(CR_UART_BASE+0x2c)

#define	UART_BRD_ACCESS		0x80
#define	UART_XYD_Y			65000
#define	UART_UCLK_115200	0
#define	UART_UCLK_57600		1
#define	UART_UCLK_38400		2
#define	UART_UCLK_28800		3
#define	UART_UCLK_19200		4
#define	UART_UCLK_14400		5
#define	UART_UCLK_9600		6
#define	UART_UCLK_4800		7
#define	UART_UCLK_2400		8
#define	UART_UCLK_1200		9
#define	UART_UCLK_600		10
#define	UART_UCLK_300		11
#define	UART_UCLK_110		12
#define	UART_BRDL			0x03
#define	UART_BRDH			0x00
#define	UART_BRDL_20M		0x01
#define	UART_BRDH_20M		0x00
#define	UART_LCR			0x03
#define	UART_FCR			0x0f
#define	UART_WATERMARK		(0x0<<6)
#define	UART_MCR			0x0
#define	UART_MISCC			0x0
#define	UART_IER			0x01

#define	IER_RECEIVED_DATA_INTERRUPT_ENABLE	0x01
#define	IER_THRE_INTERRUPT_ENABLE			0x02
#define	IER_LINE_STATUS_INTERRUPT_ENABLE	0x04

#define	IIR_INDICATOR						readb(CR_UART_IIR)
#define	IIR_RECEIVED_LINE_STATUS			0x06
#define	IIR_RECEIVED_DATA_AVAILABLE			0x04
#define	IIR_RECEIVER_IDLE_TRIGGER			0x0C
#define	IIR_TRANSMITTED_REGISTER_EMPTY		0x02
#define	LSR_INDICATOR						readb(CR_UART_LSR)
#define	LSR_RECEIVED_DATA_READY				0x01
#define	LSR_OVERRUN							0x02
#define	LSR_PARITY_ERROR					0x04
#define	LSR_FRAME_ERROR						0x08
#define	LSR_BREAK							0x10
#define	LSR_THRE							0x20
#define	LSR_THE								0x40
#define	LSR_RFIFO_FLAG						0x80

#ifdef CONFIG_ECNT_MULTIUPGRADE
extern char startMultiUpgrade;
int escape_autoboot = 0;
#endif

/* crystal clock is 20Mhz */
static unsigned long uclk_20M[13]={ // 65000*(b*16*1)/2000000
	59904,		// Baud rate 115200
	29952,		// Baud rate 57600
	19968,		// Baud rate 38400
	14976,		// Baud rate 28800
	9984,		// Baud rate 19200
	7488,		// Baud rate 14400
	4992,		// Baud rate 9600
	2496,		// Baud rate 4800
	1248,		// Baud rate 2400
	624,		// Baud rate 1200
	312,		// Baud rate 600
	156,		// Baud rate 300
	57			// Baud rate 110
};


int ecnt_uart_init (void)
{
	//ECNT
	// Set FIFO controo enable, reset RFIFO, TFIFO, 16550 mode, watermark=0x00 (1 byte)
	writel(UART_FCR|UART_WATERMARK, CR_UART_FCR);

	// Set modem control to 0
	writel(UART_MCR, CR_UART_MCR);

	// Disable IRDA, Disable Power Saving Mode, RTS , CTS flow control
	writel(UART_MISCC, CR_UART_MISCC);

	// Set interrupt Enable to, enable Tx, Rx and Line status
	writel(UART_IER, CR_UART_IER);

	/* access the bardrate divider */
	writel(UART_BRD_ACCESS, CR_UART_LCR);

	writel(((((unsigned int)(uclk_20M[0]))<<16)|UART_XYD_Y), CR_UART_XYD);

	/* Set Baud Rate Divisor to 1*16 */
	writel(UART_BRDL_20M, CR_UART_BRDL);
	writel(UART_BRDH_20M, CR_UART_BRDH);

	/* Set DLAB = 0, clength = 8, stop =1, no parity check	*/
	writel(UART_LCR, CR_UART_LCR);

	return 0;
}

void uart_putc(const char c)
{
	while (!(readl(CR_UART_LSR) & LSR_THRE));
	writel(c, CR_UART_THR);
}

int uart_getc(void)  /* returns -1 if no data available */
{
	while (!(readl(CR_UART_LSR) & UART_LSR_DR));
	return (int)readl(CR_UART_RBR);
}

void uart_puts(const char *s)
{
	while (*s)
	{
		if (*s == '\n')
			uart_putc('\r');
		uart_putc(*s);
		s++;
	}
}

int uart_tstc(void)
{
#ifdef CONFIG_ECNT_MULTIUPGRADE
	if (startMultiUpgrade && (escape_autoboot == 0))
	{
		escape_autoboot = 1;
		return startMultiUpgrade;
	}
#endif
	return (readl(CR_UART_LSR) & 0x1);
}

void uart_setbrg(void)
{

}

static struct serial_device ecnt_uart_drv = {
	.name	= "ecnt_uart",
	.start	= ecnt_uart_init,
	.stop	= NULL,
	.setbrg	= uart_setbrg,
	.putc	= uart_putc,
	.puts	= uart_puts,
	.getc	= uart_getc,
	.tstc	= uart_tstc,
};

void ecnt_serial_initialize(void)
{
	serial_register(&ecnt_uart_drv);
}

struct serial_device *default_serial_console(void)
{
	return &ecnt_uart_drv;
}



