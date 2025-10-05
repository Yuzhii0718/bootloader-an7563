#include <asm/tc3162.h>
#include <asm/io.h>
#include <common.h>

#define GPIO_DBG 0

#if GPIO_DBG
#define GPIO_PRINTF	printf
#else
#define	GPIO_PRINTF(arg...)
#endif

#ifdef CONFIG_ECNT_MULTIUPGRADE
#define MAXMULTILEDNUM 16
#define SYS_BLK_TIME				0x10
#define MAX_GPIO_NUM	(64)


extern volatile unsigned long Jiffies;
extern volatile char finishMultiUpgrade;
uint8 multi_upgrade_gpio[MAXMULTILEDNUM];
uint8 internet_gpio;
uint8 dsl_gpio;
uint8 gpio_active_high[MAX_GPIO_NUM];
#endif

void LED_OEN(uint8 x)
{
	if(x>31){
		if(x>47){
			regWrite32(CR_GPIO_CTRL3,regRead32(CR_GPIO_CTRL3)|(1<<((x-48)*2)));
		}
		else{
			regWrite32(CR_GPIO_CTRL2,regRead32(CR_GPIO_CTRL2)|(1<<((x-32)*2)));
		}
		regWrite32(CR_GPIO_ODRAIN1,regRead32(CR_GPIO_ODRAIN1)|(1<<(x-32)));
	}
	else{
		if(x > 15){
			regWrite32(CR_GPIO_CTRL1,regRead32(CR_GPIO_CTRL1)|(1<<((x-16)*2)));
		}
		else{
			regWrite32(CR_GPIO_CTRL,regRead32(CR_GPIO_CTRL)|(1<<(x*2)));
		}
		regWrite32(CR_GPIO_ODRAIN,regRead32(CR_GPIO_ODRAIN)|(1<<(x)));
		
	}
}

void turn_high_gpio(uint8 x)
{
	if(x > 31 && x < 64){
		/* GPIO 32 ~ 63 */
		regWrite32(CR_GPIO_DATA1,regRead32(CR_GPIO_DATA1)|(1<<(x-32)));
	} else if(x < 32) {
		/* GPIO 0 ~ 31 */
		regWrite32(CR_GPIO_DATA,regRead32(CR_GPIO_DATA)|(1<<x));
	} else {
		printf("error GPIO index:%d.\n", x);
	}
}

void turn_low_gpio(uint8 x)
{
	if(x > 31 && x < 64){
		/* GPIO 32 ~ 63 */
		regWrite32(CR_GPIO_DATA1,regRead32(CR_GPIO_DATA1)& ~(1<<(x-32)));
	} else if(x < 32) {
		/* GPIO 0 ~ 31 */
		regWrite32(CR_GPIO_DATA,regRead32(CR_GPIO_DATA)& ~(1<<x));
	} else {
		printf("error GPIO index:%d.\n", x);
	}
}

void LED_OFF(uint8 x)
{
	char active_high = gpio_active_high[x];

	if(active_high) {
		turn_low_gpio(x);
	} else {
		turn_high_gpio(x);
	}
}


void LED_ON(uint8 x)
{
	char active_high = gpio_active_high[x];

	if(active_high) {
		turn_high_gpio(x);
	} else {
		turn_low_gpio(x);
	}
}


void multiupgrade_blink() 
{
	int i = 0;
	static int MultiUpgradeTag = 0;
	uint8 multi_led;

	/*
		after multi-boot, system led will blink slowly
	*/
	if (finishMultiUpgrade){
		if (Jiffies & SYS_BLK_TIME){
			if(MultiUpgradeTag == 0){
				for(i=0; i<MAXMULTILEDNUM; i++){
					multi_led = multi_upgrade_gpio[i];
					if(multi_led != 0) {
						LED_ON(multi_led);
					}
				}
				MultiUpgradeTag = 1;
			}
		}
		else{
			if(MultiUpgradeTag == 1){
				for(i=0; i<MAXMULTILEDNUM; i++){
					multi_led = multi_upgrade_gpio[i];
					if(multi_led != 0) {
						LED_OFF(multi_led);
					}
				}
				MultiUpgradeTag = 0;
			}
		}
	}
}

#ifdef CONFIG_ECNT_MULTIUPGRADE
/* parse gpio information from env*/
void multiupgrade_led_init(void)
{	
	int i = 0;
	char *multi_gpio;
	char temp[2] = {0};
	multi_gpio = getenv("multi_upgrade_gpio");
	for (i = 0; i < MAXMULTILEDNUM;i++)
	{
		temp[0] = multi_gpio[i<<1];
		temp[1] = multi_gpio[(i<<1)+1];
		multi_upgrade_gpio[i] = simple_strtoul(temp, NULL, 16);
		GPIO_PRINTF("multi_upgrade_gpio[%d]=%d\n", i, multi_upgrade_gpio[i]);
	}	
	internet_gpio =  simple_strtoul(getenv("internet_gpio"), NULL, 16);
	dsl_gpio = simple_strtoul(getenv("dsl_gpio"), NULL, 16);
	GPIO_PRINTF("internet_gpio=%d\n", internet_gpio);
	GPIO_PRINTF("dsl_gpio=%d\n", dsl_gpio);
}
#endif

void lan_led_init(void)
{
	int i = 0;
	char *active_high, temp[2];
	active_high = getenv("gpio_active_high");

	memset(temp, 0, sizeof(temp));
	if (active_high) {
		GPIO_PRINTF("strlen(active_high)=%d\n", strlen(active_high));
		GPIO_PRINTF("active_high=%s\n", active_high);
		for (i = 0; i < MAX_GPIO_NUM; i++) {
			temp[0] = active_high[i];
			GPIO_PRINTF("temp=%c\n", temp[0]);
			gpio_active_high[MAX_GPIO_NUM - i - 1] = simple_strtoul(temp, NULL, 2);
		}

#if GPIO_DBG
		for (i = 0; i < MAX_GPIO_NUM;i++) {
			GPIO_PRINTF("gpio_active_high[%d]=%d\n", i, gpio_active_high[i]);
		}
#endif
	}
	else {
		printf("No \"gpio_active_high\" at env file\n\n");
	}
	
#if defined(TCSUPPORT_AUTOBENCH) && defined(TCSUPPORT_CPU_EN7581)
	printf("SLT LAN LED disable\n");
#else
	//LAN_LED0_enable();
	LAN_LED_disable();
#endif
}
void power_led_init(void)
{
	char *s = getenv("power_gpio");
	uint8 power_gpio = simple_strtoul(s+2, NULL, 16);
	LED_OEN(power_gpio);
	LED_ON(power_gpio);
}
void led_init(void)
{	
	power_led_init();
	lan_led_init();
	
#ifdef CONFIG_ECNT_MULTIUPGRADE
	multiupgrade_led_init();
#endif
}

