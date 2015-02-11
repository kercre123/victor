/*
 * linux/arch/arm/mach-omap2/board-omap3logic.c
 *
 * Copyright (C) 2010 Li-Pro.Net
 * Stephan Linz <linz@li-pro.net>
 *
 * Copyright (C) 2010 Logic Product Development, Inc.
 * Peter Barada <peter.barada@logicpd.com>
 *
 * Modified from Beagle, EVM, and RX51
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/opp.h>
#include <linux/gpio_keys.h>

#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>

#include <linux/i2c/twl.h>
#include <linux/i2c/tsc2004.h>
#include <linux/wl12xx.h>
#include <linux/mmc/host.h>
#include <linux/usb/isp1763.h>
#include <linux/skbuff.h>
#include <linux/ti_wilink_st.h>

#include <linux/spi/spi.h>
#include <linux/spi/brf6300.h>
#include <linux/spi/eeprom.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include "mux.h"
#include "hsmmc.h"
#include "timer-gp.h"
#include "control.h"
#include "common-board-devices.h"

#include <plat/mux.h>
#include <plat/board.h>
#include <plat/usb.h>
#include <plat/common.h>
#include <video/omapdss.h>
#include <linux/smsc911x.h>
#include <plat/gpmc-smsc911x.h>
#include <plat/gpmc.h>
#include <plat/sdrc.h>
#include <plat/omap_device.h>
#include <plat/mcspi.h>

#include "smartreflex.h"
#include "pm.h"

#include <plat/board-omap3logic.h>
#include <plat/board-omap3logic-display.h>
// #include <plat/omap3logic-new-productid.h>
#include <plat/omap3logic-productid.h>
#include <plat/omap3logic-cf.h>
#include "board-omap3logic.h"

#ifdef CONFIG_PRINTK_DEBUG
#include <plat/printk-debug.h>
#endif

#if defined(CONFIG_MEDIA_CONTROLLER) || defined(CONFIG_MEDIA_CONTROLLER_MODULE)
// #include <media/omap3isp.h>	// for 3.1 kernel
#include <../drivers/media/video/omap3isp/isp.h>
#endif
#include "devices.h"
#include <../drivers/media/video/ov7690.h>
#include <../drivers/media/video/ov2686.h>
#include <media/mt9p031.h>


#define OMAP3LOGIC_SMSC911X_CS			1

#define OMAP3530_LV_SOM_MMC_GPIO_CD		110
#define OMAP3530_LV_SOM_MMC_GPIO_WP		126
#define OMAP3530_LV_SOM_SMSC911X_GPIO_IRQ	152

#define OMAP3_TORPEDO_MMC_GPIO_CD		127
#define OMAP3_TORPEDO_SMSC911X_GPIO_IRQ		129

#define DM37_TORP_WIFI_BT_EN_GPIO               162

static struct regulator_consumer_supply omap3logic_vmmc1_supply = {
	.supply			= "vmmc",
};

/* wl128x BT, FM, GPS connectivity chip */
static int kim_suspend(struct platform_device *plat, pm_message_t msg)
{
	printk("%s:\n", __FUNCTION__);

/* This is a work around for DM3730 TI errata:
   "Usage Note 2.9 GPIO Is Driving Random Values When Device Returns From OFF Mode"
*/
	gpio_direction_input(DM37_TORP_WIFI_BT_EN_GPIO);
	return 0;
}

static int kim_resume(struct platform_device *plat)
{
	printk("%s:\n", __FUNCTION__);

/* This is a work around for DM3730 TI errata:
   "Usage Note 2.9 GPIO Is Driving Random Values When Device Returns From OFF Mode"
*/
	gpio_direction_output(DM37_TORP_WIFI_BT_EN_GPIO, 1);
	return 0;
}

static struct ti_st_plat_data omap3logic_wilink_pdata = {
        .nshutdown_gpio = DM37_TORP_WIFI_BT_EN_GPIO,
        .dev_name = "/dev/ttyO1",
        .flow_cntrl = 1,
        .baud_rate = 3000000,
	.suspend = kim_suspend,
	.resume = kim_resume,
};

static struct platform_device omap3logic_ti_st= {
        .name           = "kim",
        .id             = -1,
	.dev = {
        	.platform_data = &omap3logic_wilink_pdata,
	},
};

static struct platform_device omap3logic_btwilink_device = {
       .name = "btwilink",
       .id = -1,
};

/* VPLL2 for wl1283 GPS antenna supply */
static struct regulator_consumer_supply omap3logic_vmmc2_supplies[] = {
	REGULATOR_SUPPLY("wl1283-gps", "tigps"),
};

static struct regulator_init_data omap3logic_vmmc2 = {
	.constraints = {
		.name			= "VMMC2",
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(omap3logic_vmmc2_supplies),
	.consumer_supplies	= omap3logic_vmmc2_supplies,
};


/* VMMC1 for MMC1 pins CMD, CLK, DAT0..DAT3 (20 mA, plus card == max 220 mA) */
static struct regulator_init_data omap3logic_vmmc1 = {
	.constraints = {
		.name			= "VMMC1",
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &omap3logic_vmmc1_supply,
};

static struct regulator_consumer_supply omap3logic_vaux1_supply = {
	.supply			= "vaux1",
};

/* VAUX1 for touch/product ID chip */
static struct regulator_init_data omap3logic_vaux1 = {
	.constraints = {
		.min_uV		= 3000000,
		.max_uV		= 3000000,
		.apply_uV	= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
#if 1
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
#endif
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &omap3logic_vaux1_supply,
};

static struct regulator_consumer_supply omap3logic_vaux3_supplies[] = {
	REGULATOR_SUPPLY("vmmc_aux", "omap_hsmmc.2"),
};

static struct regulator_init_data omap3logic_vaux3 = {
	.constraints = {
		.min_uV			= 2800000,
		.max_uV			= 2800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies		= ARRAY_SIZE(omap3logic_vaux3_supplies),
	.consumer_supplies		= omap3logic_vaux3_supplies,
};

#if defined(CONFIG_VIDEO_OMAP3)
static struct regulator_consumer_supply omap3logic_vaux4_supply = {
	.supply			= "vaux4",
};

/* VAUX4 for cam_d0/cam_d1 supply */
static struct regulator_init_data omap3logic_vaux4 = {
	.constraints = {
		.name		= "vaux4",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &omap3logic_vaux4_supply,
};
#endif	// CONFIG_VIDEO_OMAP3

static struct regulator_consumer_supply omap3logic_vmmc3_supply = {
	.supply			= "vmmc",
	.dev_name		= "omap_hsmmc.2",
};

static struct regulator_init_data omap3logic_vmmc3 = {
	.constraints = {
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies = &omap3logic_vmmc3_supply,
};

static struct regulator_consumer_supply omap3_vdd1_supply[] = {
	REGULATOR_SUPPLY("vcc", "mpu.0"),
};

static struct regulator_consumer_supply omap3_vdd2_supply[] = {
	REGULATOR_SUPPLY("vcc", "l3_main.0"),
};

static int twl_set_voltage(void *data, int target_uV)
{
        struct voltagedomain *voltdm = (struct voltagedomain *)data;
	int retval = 0;

	/* Disable voltage processor Module */
	/* Disable SmartReflex Module */
	omap_sr_disable(voltdm);

	/* Force SMSPS voltage update using FORCEUPDATE bit of the voltage processor */
	retval = voltdm_scale(voltdm, target_uV);

	/* Enable voltage processor Module */
	/* Enable SmartReflex Module */
	omap_sr_enable(voltdm);

	return retval;
}

static int twl_get_voltage(void *data)
{
        struct voltagedomain *voltdm = (struct voltagedomain *)data;
	return voltdm_get_voltage(voltdm);
}

static struct twl_regulator_driver_data omap3_vdd1_drvdata = {
        .get_voltage = twl_get_voltage,
        .set_voltage = twl_set_voltage,
};

static struct twl_regulator_driver_data omap3_vdd2_drvdata = {
        .get_voltage = twl_get_voltage,
        .set_voltage = twl_set_voltage,
};

static struct regulator_init_data omap3_vdd1 = {
	.constraints = {
		.name                   = "vdd_mpu_iva",
		.min_uV                 = 600000,
		.max_uV                 = 1450000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL,
		.valid_ops_mask         = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies          = ARRAY_SIZE(omap3_vdd1_supply),
	.consumer_supplies              = omap3_vdd1_supply,
};

static struct regulator_init_data omap3_vdd2 = {
	.constraints = {
		.name                   = "vdd_core",
		.min_uV                 = 600000,
		.max_uV                 = 1450000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL,
		.valid_ops_mask         = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies          = ARRAY_SIZE(omap3_vdd2_supply),
	.consumer_supplies              = omap3_vdd2_supply,
};

#define OMAP3LOGIC_WLAN_SOM_LV_PMENA_GPIO 3
#define OMAP3LOGIC_WLAN_SOM_LV_IRQ_GPIO 2
#define OMAP3LOGIC_WLAN_TORPEDO_PMENA_GPIO 157
#define OMAP3LOGIC_WLAN_TORPEDO_IRQ_GPIO 152

static struct fixed_voltage_config omap3logic_vwlan = {
	.supply_name		= "vwl1271",
	.microvolts		= 1800000, /* 1.8V */
	.gpio			= -EINVAL,
	.startup_delay		= 70000, /* 70msec */
	.enable_high		= 1,
	.enabled_at_boot	= 0,
	.init_data		= &omap3logic_vmmc3,
};

static struct platform_device omap3logic_vwlan_device = {
	.name		= "reg-fixed-voltage",
	.id		= 1,
	.dev = {
		.platform_data	= &omap3logic_vwlan,
	},
};

static struct wl12xx_platform_data omap3logic_wlan_data __initdata = {
	.irq = -EINVAL,
	.board_ref_clock = -EINVAL,
	.board_tcxo_clock = -EINVAL,
};

static int omap3logic_twl_gpio_base;	/* base GPIO of TWL4030 GPIO.0 */

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
static struct gpio_keys_button gpio_buttons[] = {
	{
		.code = KEY_HOME,
		.gpio = -EINVAL,
		.desc = "home",
		.active_low = 1,
		.debounce_interval = 30,
		.wakeup = 0,
	},
	{
		.code = KEY_MENU,
		.gpio = -EINVAL,
		.desc = "menu",
		.active_low = 1,
		.debounce_interval = 30,
		.wakeup = 0,
	},
	{
		.code = KEY_BACK,
		.gpio = -EINVAL,
		.desc = "back",
		.active_low = 1,
		.debounce_interval = 30,
		.wakeup = 0,
	},
	{
		.code = KEY_SEARCH,
		.gpio = -EINVAL,
		.desc = "search",
		.active_low = 1,
		.debounce_interval = 30,
		.wakeup = 0,
	},
};

static struct gpio_keys_platform_data gpio_key_info = {
	.buttons = gpio_buttons,
	.nbuttons = ARRAY_SIZE(gpio_buttons),
};

static struct platform_device keys_gpio = {
	.name = "gpio-keys",
	.id   = -1,
	.dev  = {
		.platform_data = &gpio_key_info,
	},
};

static void omap3logic_gpio_key_init(unsigned int gpio)
{
	int i, j;

	int omap3logic_key_menu = -EINVAL;
	int omap3logic_key_home = -EINVAL;
	int omap3logic_key_back = -EINVAL;
	int omap3logic_key_search = -EINVAL;

	if (machine_is_dm3730_torpedo()) {
		omap3logic_key_home = 181;
		omap3logic_key_menu = 7;
		omap3logic_key_back = 2;
		omap3logic_key_search = 178;

		omap_mux_init_signal("sys_boot0.gpio_2", OMAP_PIN_INPUT_PULLUP);
		omap_mux_init_signal("sys_boot5.gpio_7", OMAP_PIN_INPUT_PULLUP);
		omap_mux_init_signal("mcspi2_cs0.gpio_181", OMAP_PIN_INPUT_PULLUP);
		omap_mux_init_signal("mcspi2_clk.gpio_178", OMAP_PIN_INPUT_PULLUP);

	} else if (machine_is_dm3730_som_lv()) {
		omap3logic_key_home = gpio + 7;   /* S6/GPIO5 */
		omap3logic_key_menu = gpio + 15;  /* S5/GPIO4 */
		omap3logic_key_back = 111;        /* S4/GPIO3 */

		omap_mux_init_signal("cam_xclkb.gpio_111", OMAP_PIN_INPUT_PULLUP);
	}

	for (i = 0; i < ARRAY_SIZE(gpio_buttons); i++) {
		if( KEY_MENU == gpio_buttons[i].code )
			gpio_buttons[i].gpio = omap3logic_key_menu;
		if( KEY_HOME == gpio_buttons[i].code )
			gpio_buttons[i].gpio = omap3logic_key_home;
		if( KEY_BACK == gpio_buttons[i].code )
			gpio_buttons[i].gpio = omap3logic_key_back;
		if( KEY_SEARCH == gpio_buttons[i].code )
			gpio_buttons[i].gpio = omap3logic_key_search;
	}

	/* Adjust deboucing configuration of GPIO buttons, depending on baseboard type. */
	for (i = 0; i < ARRAY_SIZE(gpio_buttons); i++) {
		if (machine_is_dm3730_torpedo()) {
			/* On Torpedo, HOME (S6) and SEARCH (S7) buttons are connected to capacitors.
                           Thus there is no need of debouncing for those two buttons. */
			if( KEY_HOME == gpio_buttons[i].code )
				gpio_buttons[i].debounce_interval = 0;
			if( KEY_SEARCH == gpio_buttons[i].code )
				gpio_buttons[i].debounce_interval = 0;
		} else if (machine_is_dm3730_som_lv()) {
			/* On SOM-LV, BACK button (S4) is connected to a capacitor.
                           Thus there is no need of debouncing for that button. */
			if( KEY_BACK == gpio_buttons[i].code )
				gpio_buttons[i].debounce_interval = 0;
		}
	}

	/** Find any buttons that are invalid, and move up the buttons
	 *  in the structure. */
	for(i=0,j=0; i < ARRAY_SIZE(gpio_buttons);++i,++j)
	{
		if(gpio_buttons[i].gpio < 0)
		{
			--j;
		} else if(i != j)
		{
			gpio_buttons[j] = gpio_buttons[i];
		}
	}

	printk(KERN_INFO "Found %i valid devkit keys\n", j);
	gpio_key_info.nbuttons = j;

	if (platform_device_register(&keys_gpio) < 0)
		printk(KERN_ERR "Unable to register GPIO key device\n");
}
#endif

#if defined(CONFIG_NEW_LEDS) || defined(CONFIG_NEW_LEDS_MODULE)
static struct gpio_led omap3logic_leds[] = {
	{
		.name			= "led1",	/* D1 on baseboard */
		.default_trigger	= "heartbeat",
		.gpio			= -EINVAL,	/* gets replaced */
		.active_low		= false,
	},
	{
		.name			= "led2",	/* D2 on baseboard */
		.default_trigger	= "none",
		.gpio			= -EINVAL,	/* gets replaced */
		.active_low		= false,
	},
	{
		.name			= "led3",	/* D1 on Torpedo module */
		.default_trigger	= "none",
		.gpio			= -EINVAL,	/* gets replaced */
		.active_low		= true,
	},
};

static struct gpio_led_platform_data omap3logic_led_data = {
	.leds		= omap3logic_leds,
	.num_leds	= 0,	/* Initialized in omap3logic_led_init() */
 };

static struct platform_device omap3logic_led_device = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data	= &omap3logic_led_data,
	},
};

#define GPIO_LED1_SOM_LV	133
#define GPIO_LED2_SOM_LV	11

#define GPIO_LED1_TORPEDO	180
#define GPIO_LED2_TORPEDO	179

static void omap3logic_led_init(void)
{
	int gpio_led1 = -EINVAL;
	int gpio_led2 = -EINVAL;

	if (machine_is_omap3_torpedo() || machine_is_dm3730_torpedo()) {
#if defined(CONFIG_GPIO_TWL4030) || defined(CONFIG_GPIO_TWL4030_MODULE)
		if (!omap3logic_twl_gpio_base) {
			printk(KERN_ERR "Huh?!? twl4030_gpio_base not set!\n");
			return;
		}
#endif
		/* baseboard LEDs are MCSPIO2_SOMI, MCSPOI2_SIMO */
		gpio_led1 = GPIO_LED1_TORPEDO;
		gpio_led2 = GPIO_LED2_TORPEDO;

		/* twl4030 ledA is the LED on the module */
#if defined(CONFIG_GPIO_TWL4030) || defined(CONFIG_GPIO_TWL4030_MODULE)
		omap3logic_leds[2].gpio = omap3logic_twl_gpio_base + TWL4030_GPIO_MAX + 0;
		omap3logic_led_data.num_leds = 3;
#else
		omap3logic_led_data.num_leds = 2;
#endif
	} else if (machine_is_omap3530_lv_som() || machine_is_dm3730_som_lv()) {
		gpio_led1 = GPIO_LED1_SOM_LV;
		omap3logic_leds[0].active_low = true;
		gpio_led2 = GPIO_LED2_SOM_LV;
		omap3logic_leds[1].active_low = true;

		/* SOM has only two LEDs */
		omap3logic_led_data.num_leds = 2;
	}

#if defined(CONFIG_GPIO_TWL4030) || defined(CONFIG_GPIO_TWL4030_MODULE)
	if (gpio_led1 < omap3logic_twl_gpio_base)
#endif
		omap_mux_init_gpio(gpio_led1, OMAP_PIN_OUTPUT);
	omap3logic_leds[0].gpio = gpio_led1;

#if defined(CONFIG_GPIO_TWL4030) || defined(CONFIG_GPIO_TWL4030_MODULE)
	if (gpio_led2 < omap3logic_twl_gpio_base)
#endif
		omap_mux_init_gpio(gpio_led2, OMAP_PIN_OUTPUT);
	omap3logic_leds[1].gpio = gpio_led2;

	if (platform_device_register(&omap3logic_led_device) < 0)
		printk(KERN_ERR "Unable to register LED device\n");
}
#else
static void omap3logic_led_init(void)
{
}
#endif

static void omap3logic_spi_init(void);
static void brf6300_dev_init(void);


static int omap3logic_twl_gpio_setup(struct device *dev,
		unsigned gpio, unsigned ngpio)
{
	omap3logic_twl_gpio_base = gpio;

#if defined(CONFIG_GPIO_TWL4030) || defined(CONFIG_GPIO_TWL4030_MODULE)
	omap3logic_led_init();
#endif
#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
	omap3logic_gpio_key_init(gpio);
#endif

	brf6300_dev_init();
	omap3logic_spi_init();

	return 0;
}

static struct twl4030_gpio_platform_data omap3logic_gpio_data = {
	.gpio_base	= OMAP_MAX_GPIO_LINES,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
	.use_leds	= true,
	.pullups	= BIT(1)  | BIT(7)
#if 1
	| BIT(8)
#endif
	| BIT(15),
	.pulldowns	= BIT(2)  | BIT(6)
#if 0
	| BIT(8)
#endif
			| BIT(13) | BIT(16) | BIT(17),
	.setup		= omap3logic_twl_gpio_setup,
};

#if defined(CONFIG_OMAP2_DSS) || defined(CONFIG_OMAP2_DSS_MODULE)
static struct regulator_consumer_supply omap3logic_vdda_dac_supplies[] = {
	REGULATOR_SUPPLY("vdda_dac", "omapdss_venc"),
};

/* VDAC for DSS driving S-Video */
static struct regulator_init_data omap3logic_vdac = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.consumer_supplies	= omap3logic_vdda_dac_supplies,
	.num_consumer_supplies	= ARRAY_SIZE(omap3logic_vdda_dac_supplies),
};

/* VPLL2 for digital video outputs */
static struct regulator_consumer_supply omap3logic_vpll2_supplies[] = {
	REGULATOR_SUPPLY("vdds_dsi", "omapdss"),
	REGULATOR_SUPPLY("vdds_dsi", "omapdss_dsi1"),
	REGULATOR_SUPPLY("vpll2", NULL),
};

static struct regulator_init_data omap3logic_vpll2 = {
	.constraints = {
		.name			= "VDVI",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(omap3logic_vpll2_supplies),
	.consumer_supplies	= omap3logic_vpll2_supplies,
};
#endif

static struct twl4030_usb_data omap3logic_usb_data = {
	.usb_mode	= T2_USB_MODE_ULPI,
};

static struct twl4030_ins  sleep_on_seq[] = {
	/* Broadcast message to put res to sleep (TYPE2 = 1, 2) */
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R1, RES_STATE_SLEEP), 2},
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R2, RES_STATE_SLEEP), 2},
};

static struct twl4030_script sleep_on_script = {
	.script	= sleep_on_seq,
	.size	= ARRAY_SIZE(sleep_on_seq),
	.flags	= TWL4030_SLEEP_SCRIPT,
};

static struct twl4030_ins wakeup_p12_seq[] = {
	/*
	 * Broadcast message to put resources to active
	 *
	 * Since we are not using TYPE, resources which have TYPE2 configured
	 * as 1 will be targeted (VPLL1, VDD1, VDD2, REGEN, NRES_PWRON, SYSEN).
	 */
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R1, RES_STATE_ACTIVE), 2},
};

static struct twl4030_script wakeup_p12_script = {
	.script	= wakeup_p12_seq,
	.size	= ARRAY_SIZE(wakeup_p12_seq),
	.flags	= TWL4030_WAKEUP12_SCRIPT,
};

static struct twl4030_ins wakeup_p3_seq[] = {
	/*
	 * Broadcast message to put resources to active
	 *
	 * Since we are not using TYPE, resources which have TYPE2 configured
	 * as 2 will be targeted
	 * (VINTANA1, VINTANA2, VINTDIG, VIO, CLKEN, HFCLKOUT).
	 */
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R2, RES_STATE_ACTIVE), 2},
};

static struct twl4030_script wakeup_p3_script = {
	.script = wakeup_p3_seq,
	.size   = ARRAY_SIZE(wakeup_p3_seq),
	.flags  = TWL4030_WAKEUP3_SCRIPT,
};

static struct twl4030_ins wrst_seq[] = {
	/*
	 * As a workaround for OMAP Erratum  (ID: i537 - OMAP HS devices are
	 * not recovering from warm reset while in OFF mode)
	 * NRESPWRON is toggled to force a power on reset condition to OMAP
	 */
	/* Trun OFF NRES_PWRON */
	{MSG_SINGULAR(DEV_GRP_NULL, RES_NRES_PWRON, RES_STATE_OFF), 2},
	/* Reset twl4030 */
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_OFF), 2},
	/* Reset MAIN_REF */
	{MSG_SINGULAR(DEV_GRP_NULL, RES_MAIN_REF, RES_STATE_WRST), 2},
	/* Reset All type2_group2 */
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R2, RES_STATE_WRST), 2},
	/* Reset VUSB_3v1 */
	{MSG_SINGULAR(DEV_GRP_NULL, RES_VUSB_3V1, RES_STATE_WRST), 2},
	/* Reset All type2_group1 */
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R1, RES_STATE_WRST), 2},
	/* Reset the Reset & Contorl_signals */
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_RC, RES_TYPE_ALL, RES_TYPE2_R0, RES_STATE_WRST), 2},
	/* Re-enable twl4030 */
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_ACTIVE), 2},
	/* Turn ON NRES_PWRON */
	{MSG_SINGULAR(DEV_GRP_NULL, RES_NRES_PWRON, RES_STATE_ACTIVE), 2},
};
static struct twl4030_script wrst_script = {
	.script = wrst_seq,
	.size   = ARRAY_SIZE(wrst_seq),
	.flags  = TWL4030_WRST_SCRIPT,
};

static struct twl4030_script *twl4030_scripts[] = {
	&wakeup_p12_script,
	&wakeup_p3_script,
	&sleep_on_script,
	&wrst_script,
};

static struct twl4030_resconfig twl4030_rconfig[] = {
	{
		.resource = RES_NRES_PWRON,
		.devgroup = DEV_GRP_P1 | DEV_GRP_P3,
		.type = 0,
		.type2 = 1,
		.remap_sleep = RES_STATE_SLEEP
	},
	{
		.resource = RES_VINTANA2,
		.devgroup = DEV_GRP_P1 | DEV_GRP_P3,
		.type = 0,
		.type2 = 2,
		.remap_sleep = RES_STATE_SLEEP
	},
	{
		.resource = RES_HFCLKOUT,
		.devgroup = DEV_GRP_P3,
		.type = 0,
		.type2 = 2,
		.remap_sleep = RES_STATE_SLEEP
	},
	{
		.resource = RES_VINTANA1,
		.devgroup = DEV_GRP_P1 | DEV_GRP_P3,
		.type = 1,
		.type2 = 2,
		.remap_sleep = RES_STATE_SLEEP
	},
	{
		.resource = RES_VINTDIG,
		.devgroup = DEV_GRP_P1 | DEV_GRP_P3,
		.type = 1,
		.type2 = 2,
		.remap_sleep = RES_STATE_SLEEP
	},
	{
		.resource = RES_REGEN,
		.devgroup = DEV_GRP_P1 | DEV_GRP_P3,
		.type = 2,
		.type2 = 1,
		.remap_sleep = RES_STATE_SLEEP
	},
	{
		.resource = RES_VIO,
		.devgroup = DEV_GRP_P1 | DEV_GRP_P3,
		.type = 2,
		.type2 = 2,
		.remap_sleep = RES_STATE_SLEEP
	},
	{
		.resource = RES_VPLL1,
		.devgroup = DEV_GRP_P1,
		.type = 3,
		.type2 = 1,
		.remap_sleep = RES_STATE_OFF
	},
	{
		.resource = RES_VDD2,
		.devgroup = DEV_GRP_P1,
		.type = 3,
		.type2 = 1,
		.remap_sleep = RES_STATE_OFF
	},
	{
		.resource = RES_CLKEN,
		.devgroup = DEV_GRP_P1 | DEV_GRP_P3,
		.type = 3,
		.type2 = 2,
		.remap_sleep = RES_STATE_SLEEP
	},
	{
		.resource = RES_VDD1,
		.devgroup = DEV_GRP_P1,
		.type = 4,
		.type2 = 1,
		.remap_sleep = RES_STATE_OFF
	},
	{
		.resource = RES_SYSEN,
		.devgroup = DEV_GRP_P1 | DEV_GRP_P3,
		.type = 6,
		.type2 = 1,
		.remap_sleep = RES_STATE_SLEEP
	},
	{ 0, 0},
};

static struct twl4030_power_data omap3logic_t2scripts_data = {
	.scripts        = twl4030_scripts,
	.num = ARRAY_SIZE(twl4030_scripts),
	.resource_config = twl4030_rconfig,
};

static struct twl4030_codec_audio_data omap3logic_audio_data = {
	.ramp_delay_value = 3,	/* 161 ms */
};

static struct twl4030_codec_data omap3logic_codec_data = {
	.audio_mclk = 26000000,
	.audio = &omap3logic_audio_data,
};

static struct twl4030_clock_init_data omap3logic_twl_clock_data = {
	.ck32k_lowpwr_enable = 1,
};

static struct twl4030_platform_data omap3logic_twldata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,
	.clock		= &omap3logic_twl_clock_data,

	/* platform_data for children goes here */
	.usb		= &omap3logic_usb_data,
	.gpio		= &omap3logic_gpio_data,
	.power		= &omap3logic_t2scripts_data,
	.codec		= &omap3logic_codec_data,
	.vmmc1		= &omap3logic_vmmc1,
	.vmmc2		= &omap3logic_vmmc2,
	.vaux1		= &omap3logic_vaux1,
	.vaux3		= &omap3logic_vaux3,
#if defined(CONFIG_VIDEO_OMAP3)
	.vaux4		= &omap3logic_vaux4,
#endif
#if defined(CONFIG_OMAP2_DSS) || defined(CONFIG_OMAP2_DSS_MODULE)
	.vdac           = &omap3logic_vdac,
	.vpll2          = &omap3logic_vpll2,
#endif
	.vdd1		= &omap3_vdd1,
	.vdd2		= &omap3_vdd2,
};

#ifdef CONFIG_TOUCHSCREEN_TSC2004

#define	GPIO_TSC2004_IRQ	153

static int tsc2004_pre_init(struct tsc2004_platform_data *pdata)
{
	int err;

	pdata->regulator_name = "vaux1";
	pdata->regulator = regulator_get(NULL, "vaux1");
	if (IS_ERR(pdata->regulator)) {
		pr_err("%s: unable to get %s regulator\n", __FUNCTION__, pdata->regulator_name);
		return -1;
	}

	err = regulator_enable(pdata->regulator);
	if (err) {
		pr_err("%s: unable to enable %s regulator\n", __FUNCTION__, pdata->regulator_name);
		regulator_put(pdata->regulator);
		pdata->regulator = NULL;
		return err;
	}
	return 0;
}

static int tsc2004_init_irq(void)
{
	int ret = 0;

	omap_mux_init_gpio(GPIO_TSC2004_IRQ, OMAP_PIN_INPUT | OMAP_PIN_OFF_WAKEUPENABLE);
	ret = gpio_request(GPIO_TSC2004_IRQ, "tsc2004-irq");
	if (ret < 0) {
		printk(KERN_WARNING "failed to request GPIO#%d: %d\n",
				GPIO_TSC2004_IRQ, ret);
		return ret;
	}

	if (gpio_direction_input(GPIO_TSC2004_IRQ)) {
		printk(KERN_WARNING "GPIO#%d cannot be configured as "
				"input\n", GPIO_TSC2004_IRQ);
		return -ENXIO;
	}

	return ret;
}

static void tsc2004_exit_irq(void)
{
	gpio_free(GPIO_TSC2004_IRQ);
}

static void tsc2004_post_exit(struct tsc2004_platform_data *pdata)
{
	if (pdata->regulator && regulator_is_enabled(pdata->regulator)) {
		regulator_disable(pdata->regulator);
	}
}

static int tsc2004_get_irq_level(void)
{
	return gpio_get_value(GPIO_TSC2004_IRQ) ? 0 : 1;
}

struct tsc2004_platform_data omap3logic_tsc2004data = {
	.model = 2004,
	.x_plate_ohms = 180,
	.get_pendown_state = tsc2004_get_irq_level,
	.pre_init_platform_hw = tsc2004_pre_init,
	.init_platform_hw = tsc2004_init_irq,
	.exit_platform_hw = tsc2004_exit_irq,
	.post_exit_platform_hw = tsc2004_post_exit,
	.regulator_name = "vaux1",
};

#endif

#if defined(CONFIG_VIDEO_OMAP3)
static struct regulator	*vaux4_reg = NULL;
static int omap3logic_enable_vaux4(struct v4l2_subdev *subdev, u32 on)
{
	int regerr = 0;
	if (NULL == vaux4_reg) {
		vaux4_reg = regulator_get(NULL, "vaux4");
		if (IS_ERR(vaux4_reg)) {
			pr_err("%s: unable to get vaux4 regulator\n", __FUNCTION__);
			return PTR_ERR(vaux4_reg);
		}
	}

	if (on) {
		regerr = regulator_enable(vaux4_reg);
		if (regerr) {
			printk(KERN_INFO "%s: error enabling vaux4 regulator\n", __FUNCTION__);
		}
	} else {
		regerr = regulator_disable(vaux4_reg);
		if (regerr) {
			printk(KERN_INFO "%s: error disabling vaux4 regulator\n", __FUNCTION__);
		}

	}
	return regerr;
}

static int omap3logic_set_xclk(struct v4l2_subdev *subdev, int hz)
{
	int isperr = 0;
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);
	if (NULL == isp) {
		printk(KERN_ERR "%s: could not determine isp_device: v4l2_dev=%p\n",
			__FUNCTION__, subdev->v4l2_dev);
		return -EINVAL;
	}
	if (NULL == isp->platform_cb.set_xclk) {
		printk(KERN_ERR "%s: set_xclk function is not set\n",
			__FUNCTION__);
		return -EINVAL;
	}

	isperr = isp->platform_cb.set_xclk(isp, hz, ISP_XCLK_A);
	udelay(5);
	return isperr;
}
#endif /* defined(CONFIG_VIDEO_OMAP3) */

static int omap3logic_ov2686_s_xclk(struct v4l2_subdev *s, u32 on) {
  omap3logic_enable_vaux4(s, on);
  return omap3logic_set_xclk(s, on ? 24000000 : 0);
}

static struct ov2686_platform_data omap3logic_ov2686_platform_data = {
  .s_xclk = omap3logic_ov2686_s_xclk,
};

static struct i2c_board_info omap3logic_ov2686_board_info = {
  I2C_BOARD_INFO("ov2686", 0x3c),
  .platform_data = &omap3logic_ov2686_platform_data,
};

static struct isp_subdev_i2c_board_info omap3logic_ov2686_subdevs[] = {
  {
    .board_info = &omap3logic_ov2686_board_info,
    .i2c_adapter_id = 2, // This is the camera I2C on the Zoom dev board and the Cozmo 3 head board
  },
  { NULL, 0 },
};


#if defined(CONFIG_VIDEO_OV7690)
static int omap3logic_ov7690_s_xclk(struct v4l2_subdev *s, u32 on) {
	omap3logic_enable_vaux4(s, on);
	return omap3logic_set_xclk(s, on ? 24000000 : 0);
}

static struct ov7690_platform_data omap3logic_ov7690_platform_data = {
	.s_xclk                 = omap3logic_ov7690_s_xclk,
	.min_width              = 640,
	.min_height             = 480,
};

static struct i2c_board_info omap3logic_ov7690_board_info = {
	I2C_BOARD_INFO("ov7690", 0x42>>1),
	.platform_data = &omap3logic_ov7690_platform_data,
};

static struct isp_subdev_i2c_board_info omap3logic_ov7690_subdevs[] = {
	{
		.board_info = &omap3logic_ov7690_board_info,
		/* ov7690 is on i2c-3 on Catalyst, i2c-2 on DevKit */
		.i2c_adapter_id = 2,
	},
	{ NULL, 0 },
};
#endif /* defined(CONFIG_VIDEO_OV7690) */

#if defined(CONFIG_VIDEO_MT9P031)
static int omap3logic_mt9p031_set_xclk(struct v4l2_subdev *s, int hz) {
	omap3logic_enable_vaux4(s, 0 != hz ? 1 : 0);
	return omap3logic_set_xclk(s, hz);
}

static struct mt9p031_platform_data omap3logic_mt9p031_platform_data = {
	.set_xclk               = omap3logic_mt9p031_set_xclk,
	.ext_freq               = 24000000,
	/* MT9P031 max is 48Mhz for 1.8V IO, 96Mhz for 2.8V IO */
	/* Camera ISP max is 75Mhz for Normal/RAW and ITU/BT.656 at OPP100
	   Camera ISP max is 45Mhz for Normal/RAW and ITU/BT.656 at OPP50
	   Camera ISP max is 130Mhz for Packed/YUV (using bridge) at OPP100
	   Camera ISP max is 65Mhz for Packed/YUV (using bridge) at OPP50
	   See DM3730 Datasheet 6.5.1.2 Parallel Camera Interface */
	.target_freq            = 72000000,
	.version                = MT9P031_COLOR_VERSION,
};

static struct i2c_board_info omap3logic_mt9p031_board_info = {
	I2C_BOARD_INFO("mt9p031", 0x48),
	.platform_data = &omap3logic_mt9p031_platform_data,
};

static struct isp_subdev_i2c_board_info omap3logic_mt9p031_subdevs[] = {
	{
		.board_info = &omap3logic_mt9p031_board_info,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0 },
};
#endif /* defined(CONFIG_VIDEO_MT9P031) */

static struct i2c_board_info __initdata omap3logic_i2c3_boardinfo[] = {
#ifdef CONFIG_TOUCHSCREEN_TSC2004
	{
		I2C_BOARD_INFO("tsc2004", 0x48),
		.type		= "tsc2004",
		.platform_data = &omap3logic_tsc2004data,
		.irq = OMAP_GPIO_IRQ(GPIO_TSC2004_IRQ),
	},
#endif
};

static int __init omap3logic_i2c_init(void)
{
	omap3_vdd1.driver_data = &omap3_vdd1_drvdata;
	omap3_vdd1_drvdata.data = voltdm_lookup("mpu_iva");

	omap3_vdd2.driver_data = &omap3_vdd2_drvdata;
	omap3_vdd2_drvdata.data = voltdm_lookup("core");

	omap3_pmic_init("twl4030", &omap3logic_twldata);

	omap_register_i2c_bus(2, 400, NULL, 0);
	omap_register_i2c_bus(3, 400, omap3logic_i2c3_boardinfo,
			ARRAY_SIZE(omap3logic_i2c3_boardinfo));

	return 0;
}

#if defined(CONFIG_VIDEO_OMAP3)
static struct isp_v4l2_subdevs_group omap3logic_camera_subdevs[] = {
  {
    .subdevs = omap3logic_ov2686_subdevs,
    .interface = ISP_INTERFACE_PARALLEL,
    .bus = {
      .parallel = {
	.data_lane_shift = ISPCTRL_SHIFT_0 >> ISPCTRL_SHIFT_SHIFT, //3.3+ISP_LANE_SHIFT_0,
	.clk_pol = 0,
	.hs_pol  = 0,
	.vs_pol  = 0,
	.bridge  = ISPCTRL_PAR_BRIDGE_LENDIAN >> ISPCTRL_PAR_BRIDGE_SHIFT //3.3+ISP_BRIDGE_LITTLE_ENDIAN,
      },
    },
  },
#if defined(CONFIG_VIDEO_OV7690)
	{
		.subdevs = omap3logic_ov7690_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			.parallel = {
				.data_lane_shift	= ISPCTRL_SHIFT_0 >> ISPCTRL_SHIFT_SHIFT, //3.3+ISP_LANE_SHIFT_0,
				.clk_pol		= 0,
				.hs_pol			= 0,	// HSYNC not inverted
				.vs_pol			= 0,	// VSYNC not inverted
				.bridge			= ISPCTRL_PAR_BRIDGE_LENDIAN>> ISPCTRL_PAR_BRIDGE_SHIFT //3.3+ISP_BRIDGE_LITTLE_ENDIAN,
//3.3+				.bt656			= 0,
			},
		},
	},
#endif /*  defined(CONFIG_VIDEO_OV7690) */
#if defined(CONFIG_VIDEO_MT9P031)
	{
		.subdevs = omap3logic_mt9p031_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = {
			.parallel = {
				.data_lane_shift	= ISPCTRL_SHIFT_0 >> ISPCTRL_SHIFT_SHIFT, //3.3+ISP_LANE_SHIFT_0,
				.clk_pol		= 1,	// PIXCLK inverted
				.hs_pol			= 0,	// HSYNC not inverted
				.vs_pol			= 0,	// VSYNC not inverted
				.bridge			= ISPCTRL_PAR_BRIDGE_DISABLE>> ISPCTRL_PAR_BRIDGE_SHIFT //3.3+ISP_BRIDGE_DISABLE,
//3.3+				.bt656			= 0,
			},
		},
	},
#endif /*  defined(CONFIG_VIDEO_MT9P031) */
	{ NULL, 0 },
};

static struct isp_platform_data  omap3logic_isp_platform_data = {
	.subdevs = omap3logic_camera_subdevs,
};
#endif

static struct omap2_hsmmc_info __initdata board_mmc_info[] = {
	{
		.name		= "external",
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
	},
	{
		.name		= "wl1271",
		.mmc		= 3,
		.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_POWER_OFF_CARD,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
		.nonremovable	= true,
	},
	{}      /* Terminator */
};

static int omap3logic_wl12xx_probed;
static int omap3logic_wl12xx_found;
int omap3logic_wl12xx_exists(void)
{
	int i, gpio, val;

	if (omap3logic_wl12xx_probed)
		return omap3logic_wl12xx_found;

	omap3logic_wl12xx_probed = 1;

	/* Figure out if a WiLink is on the board.  Use
	   the pullup on WIFI_EN to determine such */
	if (machine_is_dm3730_torpedo())
		gpio = OMAP3LOGIC_WLAN_TORPEDO_PMENA_GPIO;
	else
		gpio = OMAP3LOGIC_WLAN_SOM_LV_PMENA_GPIO;

	omap_mux_init_gpio(gpio, OMAP_PIN_INPUT);
	if (gpio_request_one(gpio, GPIOF_OUT_INIT_HIGH, "wifi probe") < 0)
		printk("%s:%d\n", __FUNCTION__, __LINE__);

	/* Let it soak for a while */
	for (i=0; i<0x100; ++i)
		asm("nop");

	/* Now flip the GPIO to an input and let it drain.  If there's a
	   pulldown on the pin then when we read it back it will come out as
	   zero */
	if (gpio_direction_input(gpio) < 0)
		printk("%s:%d\n", __FUNCTION__, __LINE__);

	/* Let it soak for a while */
	for (i=0; i<0x100; ++i)
		asm("nop");

	val = gpio_get_value(gpio);
	gpio_free(gpio);

	omap3logic_wl12xx_found = !val;

	return omap3logic_wl12xx_found;
}

#ifdef CONFIG_WL12XX_PLATFORM_DATA
static int ignore_nvs_data;
static int __init omap3logic_ignore_nvs_data(char *str)
{
	/* Ignore the NVS data in the product ID chip; instead use
	 * whats on the filesystem. */
	ignore_nvs_data = 1;
	return 1;
}
__setup("ignore-nvs-data", omap3logic_ignore_nvs_data);
#endif

static int __init board_wl12xx_init(void)
{
	unsigned char mac_addr[6];

	// Setup the mux for mmc3
	if (machine_is_dm3730_som_lv() || machine_is_omap3530_lv_som()) {
		omap_mux_init_signal("mcspi1_cs1.sdmmc3_cmd", OMAP_PIN_INPUT_PULLUP); /* McSPI1_CS1/ADPLLV2D_DITHERING_EN2/MMC3_CMD/GPIO_175 */
		omap_mux_init_signal("mcspi1_cs2.sdmmc3_clk", OMAP_PIN_INPUT_PULLUP); /* McSPI1_CS2/MMC3_CLK/GPIO_176 */
	} else if (machine_is_dm3730_torpedo()) {
		omap_mux_init_signal("etk_ctl.sdmmc3_cmd", OMAP_PIN_INPUT_PULLUP); /* ETK_CTL/MMC3_CMD/HSUSB1_CLK/HSUSB1_TLL_CLK/GPIO_13 */
		omap_mux_init_signal("etk_clk.sdmmc3_clk", OMAP_PIN_INPUT_PULLUP); /* ETK_CTL/McBSP5_CLKX/MMC3_CLK/HSUSB1_STP/MM1_RXDP/HSUSB1_TLL_STP/GPIO_12 */
	} else
		return -ENODEV;

	omap_mux_init_signal("sdmmc2_dat4.sdmmc3_dat0", OMAP_PIN_INPUT_PULLUP); /* MMC2_DAT4/MMC2_DIR_DAT0/MMC3_DAT0/GPIO_136 */
	omap_mux_init_signal("sdmmc2_dat5.sdmmc3_dat1", OMAP_PIN_INPUT_PULLUP); /* MMC2_DAT5/MMC2_DIR_DAT1/CAM_GLOBAL_RESET/MMC3_DAT1/HSUSB3_TLL_STP/MM3_RXDP/GPIO_137 */
	omap_mux_init_signal("sdmmc2_dat6.sdmmc3_dat2", OMAP_PIN_INPUT_PULLUP); /* MMC2_DAT6/MMC2_DIR_CMD/CAM_SHUTTER/MMC3_DAT2/HSUSB3_TLL_DIR/GPIO_138 */
	omap_mux_init_signal("sdmmc2_dat7.sdmmc3_dat3", OMAP_PIN_INPUT_PULLUP); /* MMC2_DAT7/MMC2_CLKIN/MMC3_DAT3/HSUSB3_TLL_NXT/MM3_RXDM/GPIO_139 */

	if (machine_is_omap3530_lv_som() || machine_is_dm3730_som_lv()) {
		omap_mux_init_gpio(OMAP3LOGIC_WLAN_SOM_LV_PMENA_GPIO, OMAP_PIN_OUTPUT);
#if 0
		gpio_export(OMAP3LOGIC_WLAN_SOM_LV_PMENA_GPIO, 0);
#endif
		omap_mux_init_gpio(OMAP3LOGIC_WLAN_SOM_LV_IRQ_GPIO, OMAP_PIN_INPUT_PULLUP);
		if (gpio_request_one(OMAP3LOGIC_WLAN_SOM_LV_IRQ_GPIO, GPIOF_IN, "wlan_irq") < 0) {
			printk(KERN_WARNING "Failed to gpio_request %d for wlan_irq\n", OMAP3LOGIC_WLAN_SOM_LV_IRQ_GPIO);
			return -ENODEV;
		}

		omap3logic_wlan_data.irq = OMAP_GPIO_IRQ(OMAP3LOGIC_WLAN_SOM_LV_IRQ_GPIO);
		omap3logic_vwlan.gpio = OMAP3LOGIC_WLAN_SOM_LV_PMENA_GPIO;
		/* wl1271 ref clock is 26 MHz */
		omap3logic_wlan_data.board_ref_clock = WL12XX_REFCLOCK_26;
	} else if (machine_is_dm3730_torpedo()) {
		omap_mux_init_gpio(OMAP3LOGIC_WLAN_TORPEDO_PMENA_GPIO, OMAP_PIN_OUTPUT);
#if 0
		gpio_export( OMAP3LOGIC_WLAN_TORPEDO_PMENA_GPIO, 0 );
#endif
		omap_mux_init_gpio(OMAP3LOGIC_WLAN_TORPEDO_IRQ_GPIO, OMAP_PIN_INPUT_PULLUP);
		if (gpio_request_one(OMAP3LOGIC_WLAN_TORPEDO_IRQ_GPIO, GPIOF_IN, "wlan_irq") < 0) {
			printk(KERN_WARNING "Failed to gpio_request %d for wlan_irq\n", OMAP3LOGIC_WLAN_TORPEDO_IRQ_GPIO);
			return -ENODEV;
		}
		omap3logic_wlan_data.irq = OMAP_GPIO_IRQ(OMAP3LOGIC_WLAN_TORPEDO_IRQ_GPIO);
		omap3logic_vwlan.gpio = OMAP3LOGIC_WLAN_TORPEDO_PMENA_GPIO;

		/* Mux BT_EN as an output w/pullup and on OFF keep it high.
		 * This prevents a ~1.75mS droop of BT_EN during suspend */
		omap_mux_init_gpio(DM37_TORP_WIFI_BT_EN_GPIO, OMAP_PIN_OUTPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_HIGH);

		//gpio_request_one(DM37_TORP_WIFI_BT_EN_GPIO, GPIOF_OUT_INIT_LOW, "bt_en");
		//gpio_export(DM37_TORP_WIFI_BT_EN_GPIO, 0);

		/* wl128x ref clock is 26 MHz; torpedo TXCO clock is 26Mhz */
		omap3logic_wlan_data.board_ref_clock = WL12XX_REFCLOCK_26;
		omap3logic_wlan_data.board_tcxo_clock = WL12XX_TCXOCLOCK_26;
	} else
		return -ENODEV;

	/* Extract the MAC addr from the productID data */
	if (!omap3logic_extract_wifi_ethaddr(mac_addr))
		memcpy(omap3logic_wlan_data.mac_addr, mac_addr, sizeof(mac_addr));
#ifdef CONFIG_WL12XX_PLATFORM_DATA
	if (ignore_nvs_data)
		omap3logic_wlan_data.platform_quirks |= WL12XX_QUIRK_IGNORE_PRODUCT_ID_NVS;

	/* WL12xx WLAN Init */
	if (wl12xx_set_platform_data(&omap3logic_wlan_data))
		pr_err("error setting wl12xx data\n");
	platform_device_register(&omap3logic_vwlan_device);
	platform_device_register(&omap3logic_ti_st);
	platform_device_register(&omap3logic_btwilink_device);
#endif

	return 0;
}

/* Fix the PBIAS voltage for Torpedo MMC1 pins that
 * are used for other needs (IRQs, etc). */
static void omap3torpedo_fix_pbias_voltage(void)
{
	u16 control_pbias_offset = OMAP343X_CONTROL_PBIAS_LITE;
	static int pbias_fixed = 0;
	unsigned long timeout;
	u32 reg;

	if (!pbias_fixed) {
		/* Set the bias for the pin */
		reg = omap_ctrl_readl(control_pbias_offset);

		if(!(reg & OMAP343X_PBIASLITEPWRDNZ1) ||
		   (!!(reg & OMAP343X_PBIASLITESUPPLY_HIGH1) !=
		    !!(reg & OMAP343X_PBIASLITEVMODE1)))
		{
			reg &= ~OMAP343X_PBIASLITEPWRDNZ1;
			omap_ctrl_writel(reg, control_pbias_offset);

			/* 100ms delay required for PBIAS configuration */
			msleep(100);

			/* Set PBIASLITEVMODE1 appropriately */
			if (reg & OMAP343X_PBIASLITESUPPLY_HIGH1)
				reg |= OMAP343X_PBIASLITEVMODE1;
			else
				reg &= ~OMAP343X_PBIASLITEVMODE1;

			reg |= OMAP343X_PBIASLITEPWRDNZ1;

			omap_ctrl_writel(reg, control_pbias_offset);

			/* Wait for pbias to match up */
			timeout = jiffies + msecs_to_jiffies(5);
			do {
				reg = omap_ctrl_readl(control_pbias_offset);
				if (!(reg & OMAP343X_PBIASLITEVMODEERROR1))
					break;
			} while (!time_after(jiffies, timeout));
			if (reg & OMAP343X_PBIASLITEVMODEERROR1)
				printk("%s: Error - VMODE1 doesn't matchup to supply!\n", __FUNCTION__);
		} else {
			printk(KERN_INFO "Skipping fix pbias voltage - already set properly\n");
		}

		/* For DM3730, turn on GPIO_IO_PWRDNZ to connect input pads*/
		if (cpu_is_omap3630()) {
			reg = omap_ctrl_readl(OMAP36XX_CONTROL_WKUP_CTRL);
			reg |= OMAP36XX_GPIO_IO_PWRDNZ;
			omap_ctrl_writel(reg, OMAP36XX_CONTROL_WKUP_CTRL);
			printk("%s:%d PKUP_CTRL %#x\n", __FUNCTION__, __LINE__, omap_ctrl_readl(OMAP36XX_CONTROL_WKUP_CTRL));
		}

		pbias_fixed = 1;
	}
}

static void __init board_mmc_init(void)
{
	int ret;

	omap3torpedo_fix_pbias_voltage();

	if (machine_is_omap3530_lv_som() || machine_is_dm3730_som_lv()) {
		/* OMAP35x/DM37x LV SOM board */
		board_mmc_info[0].gpio_cd = OMAP3530_LV_SOM_MMC_GPIO_CD;
		board_mmc_info[0].gpio_wp = OMAP3530_LV_SOM_MMC_GPIO_WP;
		/* gpio_cd for MMC wired to CAM_STROBE; cam_strobe and
		 * another pin share GPIO_126. Mux CAM_STROBE as GPIO. */
		omap_mux_init_signal("cam_strobe.gpio_126", OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP);
	} else if (machine_is_omap3_torpedo() || machine_is_dm3730_torpedo()) {
		/* OMAP35x/DM37x Torpedo board */
		board_mmc_info[0].gpio_cd = OMAP3_TORPEDO_MMC_GPIO_CD;
	} else {
		/* unsupported board */
		printk(KERN_ERR "%s(): unknown machine type\n", __func__);
		return;
	}

	/* Check the SRAM for valid product_id data(put there by u-boot). */
	ret = omap3logic_fetch_sram_product_id_data();
	if (ret)
		printk(KERN_ERR "No valid product ID data found in SRAM\n");
	else
		omap3logic_create_product_id_sysfs();

	ret = board_wl12xx_init();
	if (ret) {
		/* No wifi configuration for this board */
		board_mmc_info[2].mmc = 0;
	}

	omap2_hsmmc_init(board_mmc_info);

	/* link regulators to MMC adapters */
	omap3logic_vmmc1_supply.dev = board_mmc_info[0].dev;
}

static struct omap_smsc911x_platform_data __initdata board_smsc911x_data = {
	.cs             = OMAP3LOGIC_SMSC911X_CS,
	.gpio_irq       = -EINVAL,
	.gpio_reset     = -EINVAL,
	.flags		= SMSC911X_USE_32BIT,
};


static inline void __init board_smsc911x_init(void)
{
	if (machine_is_omap3530_lv_som() || machine_is_dm3730_som_lv()) {
		/* OMAP3530 LV SOM board */
		board_smsc911x_data.gpio_irq =
					OMAP3530_LV_SOM_SMSC911X_GPIO_IRQ;
		omap_mux_init_gpio(OMAP3530_LV_SOM_SMSC911X_GPIO_IRQ, OMAP_PIN_INPUT);
	} else if (machine_is_omap3_torpedo() || machine_is_dm3730_torpedo()) {
		/* OMAP3 Torpedo board */
		board_smsc911x_data.gpio_irq = OMAP3_TORPEDO_SMSC911X_GPIO_IRQ;
		omap_mux_init_gpio(OMAP3_TORPEDO_SMSC911X_GPIO_IRQ, OMAP_PIN_INPUT);
	} else {
		/* unsupported board */
		printk(KERN_ERR "%s(): unknown machine type\n", __func__);
		return;
	}

	gpmc_smsc911x_init(&board_smsc911x_data);
}

#if defined(CONFIG_USB_ISP1763) || defined(CONFIG_USB_ISP1763_MODULE)
/* ISP1763 USB interrupt */
#define OMAP3TORPEDO_ISP1763_IRQ_GPIO          128

static struct isp1763_platform_data omap3logic_isp1763_pdata = {
       .bus_width_8            = 0,
       .port1_otg              = 0,
       .dack_polarity_high     = 0,
       .dreq_polarity_high     = 0,
       .intr_polarity_high     = 0,
       .intr_edge_trigger      = 0,
};

static struct resource omap3logic_isp1763_resources[] = {
       [0] = {
               .flags = IORESOURCE_MEM,
       },
       [1] = {
               .start = -EINVAL,
               .flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL,
       },
};

static struct platform_device omap3logic_isp1763 = {
       .name           = "isp1763",
       .id             = -1,
       .dev            = {
               .platform_data  = &omap3logic_isp1763_pdata,
       },
       .num_resources = ARRAY_SIZE(omap3logic_isp1763_resources),
       .resource = omap3logic_isp1763_resources,
};


static int omap3logic_init_isp1763(void)
{
       unsigned long cs_mem_base;
       unsigned int irq_gpio;

       /* ISP1763 IRQ is an MMC1 data pin - need to update PBIAS
        * to get voltage to the device so the IRQ works correctly rather
        * than float below logic 1 and cause IRQ storm... */
       if (machine_is_dm3730_torpedo() || machine_is_omap3_torpedo())
               omap3torpedo_fix_pbias_voltage();
       else
               return -ENODEV;

       if (gpmc_cs_request(6, SZ_16M, &cs_mem_base) < 0) {
               printk(KERN_ERR "Failed to request GPMC mem for ISP1763\n");
               return -ENOMEM;
       }

       omap3logic_isp1763_resources[0].start = cs_mem_base;
       omap3logic_isp1763_resources[0].end = cs_mem_base + 0xffff;

       irq_gpio = OMAP3TORPEDO_ISP1763_IRQ_GPIO;
       omap_mux_init_gpio(irq_gpio, OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_WAKEUPENABLE);
       /* Setup ISP1763 IRQ pin as input */
       if (gpio_request(irq_gpio, "isp1763_irq") < 0) {
               printk(KERN_ERR "Failed to request GPIO%d for isp1763 IRQ\n",
               irq_gpio);
               return -EINVAL;
       }
       gpio_direction_input(irq_gpio);
       omap3logic_isp1763_resources[1].start = OMAP_GPIO_IRQ(irq_gpio);
       if (platform_device_register(&omap3logic_isp1763) < 0) {
               printk(KERN_ERR "Unable to register isp1763 device\n");
               gpio_free(irq_gpio);
               return -EINVAL;
       } else {
               pr_info("registered isp1763 platform_device\n");
       }
       return 0;
}
#else
static int omap3logic_init_isp1763(void)
{
	return -ENODEV;
}
#endif

#if defined(CONFIG_USB_MUSB_OMAP2PLUS)
static void omap3logic_musb_init(void)
{
	/* Set up the mux for musb */
	omap_mux_init_signal("hsusb0_clk", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_stp", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_dir", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_nxt", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_data0", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_data1", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_data2", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_data3", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_data4", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_data5", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_data6", OMAP_PIN_INPUT);
	omap_mux_init_signal("hsusb0_data7", OMAP_PIN_INPUT);

	usb_musb_init(NULL);
}
#else
static void omap3logic_musb_init(void)
{
}
#endif

#ifdef CONFIG_OMAP3LOGIC_AT25160AN
/* Access the AT25160AN chip on the Torpedo baseboard using eeprom driver */
static struct spi_eeprom at25160an_config = {
	.name		= "at25160an",
	.byte_len	= 2048,
	.page_size	= 32,
	.flags		= EE_ADDR2,
};

static struct spi_board_info omap3logic_spi_at25160an = {
	.modalias	= "at25",
	.max_speed_hz	= 30000,
	.bus_num	= 1,
	.chip_select	= 0,
	.platform_data	= &at25160an_config,
	.bits_per_word	= 8,
};

#else
/* Access the AT25160AN chip on the Torpedo baseboard using spidev driver */
static struct omap2_mcspi_device_config at25160an_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel	= 0,	/* 0: slave, 1: master */
};

static struct spi_board_info omap3logic_spi_at25160an = {
	/*
	 * SPI EEPROM on Torpedo baseboard
	 */
	.modalias		= "spidev",
	.bus_num		= 1,
	.chip_select		= 0,
	.max_speed_hz		= 19000000,
	.controller_data	= &at25160an_mcspi_config,
	.irq			= 0,
	.platform_data		= NULL,
	.bits_per_word		= 8,
};
#endif

#if defined(CONFIG_OMAP3LOGIC_SPI1_CS0) \
	|| defined(CONFIG_OMAP3LOGIC_SPI1_CS1) \
	|| defined(CONFIG_OMAP3LOGIC_SPI1_CS2) \
	|| defined(CONFIG_OMAP3LOGIC_SPI1_CS3) \
	|| defined(CONFIG_OMAP3LOGIC_SPI3_CS0) \
	|| defined(CONFIG_OMAP3LOGIC_SPI3_CS1)
static struct omap2_mcspi_device_config expansion_board_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel	= 1, 	/* 0: slave, 1: master */ // Note: This doesn't actually seem to be connected anywhere in the code base.
};
#endif

#ifdef CONFIG_OMAP3LOGIC_SPI1_CS0
static struct spi_board_info omap3logic_spi1_expansion_board_cs0 = {
	/*
	 * Generic SPI on expansion board, SPI1/CS0
	 */
	.modalias		= "spidev",
	.bus_num		= 1,
	.chip_select		= 0,
	.max_speed_hz		= 48000000,
	.controller_data	= &expansion_board_mcspi_config,
	.irq			= 0,
	.platform_data		= NULL,
	.bits_per_word		= 8,
};
#endif

#ifdef CONFIG_OMAP3LOGIC_SPI1_CS1
static struct spi_board_info omap3logic_spi1_expansion_board_cs1 = {
	/*
	 * Generic SPI on expansion board, SPI1/CS1
	 */
	.modalias		= "spidev",
	.bus_num		= 1,
	.chip_select		= 1,
	.max_speed_hz		= 48000000,
	.controller_data	= &expansion_board_mcspi_config,
	.irq			= 0,
	.platform_data		= NULL,
	.bits_per_word		= 8,
};
#endif

#ifdef CONFIG_OMAP3LOGIC_SPI1_CS2
static struct spi_board_info omap3logic_spi1_expansion_board_cs2 = {
	/*
	 * Generic SPI on expansion board, SPI1/CS2
	 */
	.modalias		= "spidev",
	.bus_num		= 1,
	.chip_select		= 2,
	.max_speed_hz		= 48000000,
	.controller_data	= &expansion_board_mcspi_config,
	.irq			= 0,
	.platform_data		= NULL,
	.bits_per_word		= 8,
};
#endif

#ifdef CONFIG_OMAP3LOGIC_SPI1_CS3
static struct spi_board_info omap3logic_spi1_expansion_board_cs3 = {
	/*
	 * Generic SPI on expansion board, SPI1/CS3
	 */
	.modalias		= "spidev",
	.bus_num		= 1,
	.chip_select		= 3,
	.max_speed_hz		= 48000000,
	.controller_data	= &expansion_board_mcspi_config,
	.irq			= 0,
	.platform_data		= NULL,
	.bits_per_word		= 8,
};
#endif

#ifdef CONFIG_OMAP3LOGIC_SPI3_CS0
static struct spi_board_info omap3logic_spi3_expansion_board_cs0 = {
	/*
	 * Generic SPI on expansion board, SPI3/CS0
	 */
	.modalias		= "spidev",
	.bus_num		= 3,
	.chip_select		= 0,
	.max_speed_hz		= 48000000,
	.controller_data	= &expansion_board_mcspi_config,
	.irq			= 0,
	.platform_data		= NULL,
	.bits_per_word		= 8,
};
#endif

#ifdef CONFIG_OMAP3LOGIC_SPI3_CS1
static struct spi_board_info omap3logic_spi3_expansion_board_cs1= {
	/*
	 * Generic SPI on expansion board, SPI3/CS1
	 */
	.modalias		= "spidev",
	.bus_num		= 3,
	.chip_select		= 1,
	.max_speed_hz		= 48000000,
	.controller_data	= &expansion_board_mcspi_config,
	.irq			= 0,
	.platform_data		= NULL,
	.bits_per_word		= 8,
};
#endif

#if defined(CONFIG_BT_HCIBRF6300_SPI) || defined(CONFIG_BT_HCIBRF6300_SPI_MODULE)
static struct omap2_mcspi_device_config brf6300_mcspi_config = {
    .turbo_mode = 0,
    .single_channel = 1,  /* 0: slave, 1: master */
};


/* Intialized in omap3logic_spi_init() */
struct brf6300_platform_data brf6300_config;

#define BT_IRQ_GPIO 157

static struct spi_board_info omap3logic_spi_brf6300 =
{
    /*
     * BRF6300 operates at a max freqency of 2MHz, so
     * operate slightly below at 1.5MHz
     */
    .modalias       = "brf6300",
    .bus_num        = 1,
    .chip_select        = 0,
    .max_speed_hz       = 12000000,
    .controller_data    = &brf6300_mcspi_config,
    .irq            = OMAP_GPIO_IRQ(BT_IRQ_GPIO),
    .platform_data      = &brf6300_config,
    .mode           = SPI_MODE_1,
    .bits_per_word      = 16,
    .quirks         = SPI_QUIRK_BRF6300,
};
#endif


/* SPI device entries we can have */
static struct spi_board_info omap3logic_spi_devices[7];

static void omap3logic_spi_init(void)
{
    int nspi=0;
    int use_mcspi1 = 0;
    int use_mcspi3 = 0;

    if (machine_is_dm3730_som_lv()) {
        /* LV SOM only has the brf6300 on SPI */
#if defined(CONFIG_BT_HCIBRF6300_SPI) || defined(CONFIG_BT_HCIBRF6300_SPI_MODULE)
        omap3logic_spi_devices[nspi++] = omap3logic_spi_brf6300;
        omap_mux_init_signal("mcspi1_cs0", OMAP_PIN_INPUT);
        use_mcspi1=1;
#endif
    } else if (machine_is_dm3730_torpedo()) {
#ifdef USE_AT25_AS_EEPROM
		omap3logic_spi_devices[nspi++] = omap3logic_spi_at25160an;
		omap_mux_init_signal("mcspi1_cs0", OMAP_PIN_INPUT);
		use_mcspi1=1;
#endif
    }

#ifdef CONFIG_OMAP3LOGIC_SPI1_CS0
	/* SPIDEV on McSPI1/CS0 can only work if we aren't using it
	   for either the bfr6300 or at25160an, each of which would
	   set use_mcspi1 to non-zero. */
	if (!use_mcspi1) {
		omap3logic_spi_devices[nspi++] = omap3logic_spi1_expansion_board_cs0;
		omap_mux_init_signal("mcspi1_cs0", OMAP_PIN_INPUT);
		use_mcspi1=1;
	}
#endif
#ifdef CONFIG_OMAP3LOGIC_SPI1_CS1
	if (machine_is_dm3730_som_lv()) {
		printk("Can't setup SPIDEV SPI1/CS1 as McSPI1_CS1 not available on LV SOM\n");
	} else {
		omap3logic_spi_devices[nspi++] = omap3logic_spi1_expansion_board_cs1;
		omap_mux_init_signal("mcspi1_cs1", OMAP_PIN_INPUT);
		use_mcspi1=1;
	}
#endif
#ifdef CONFIG_OMAP3LOGIC_SPI1_CS2
	if (machine_is_dm3730_som_lv()) {
		printk("Can't setup SPIDEV SPI1/CS2 as McSPI1_CS2 not available on LV SOM\n");
	} else {
		omap3logic_spi_devices[nspi++] = omap3logic_spi1_expansion_board_cs2;
		omap_mux_init_signal("mcspi1_cs2.mcspi1_cs2", OMAP_PIN_INPUT);
		use_mcspi1=1;
	}
#endif
#ifdef CONFIG_OMAP3LOGIC_SPI1_CS3
	if (machine_is_dm3730_som_lv()) {
		printk("Can't setup SPIDEV SPI1/CS3 as McSPI1_CS3 not available on LV SOM\n");
	} else {
		omap3logic_spi_devices[nspi++] = omap3logic_spi1_expansion_board_cs3;
		omap_mux_init_signal("mcspi1_cs3.mcspi1_cs3", OMAP_PIN_INPUT);
		use_mcspi1=1;
	}
#endif
#ifdef CONFIG_OMAP3LOGIC_SPI3_CS0
	omap3logic_spi_devices[nspi++] = omap3logic_spi3_expansion_board_cs0;
	if (machine_is_dm3730_som_lv())
		omap_mux_init_signal("sdmmc2_dat3.mcspi3_cs0", OMAP_PIN_INPUT);
	else
		omap_mux_init_signal("etk_d2.mcspi3_cs0", OMAP_PIN_INPUT);
	use_mcspi3=1;
#endif
#ifdef CONFIG_OMAP3LOGIC_SPI3_CS1
	omap3logic_spi_devices[nspi++] = omap3logic_spi3_expansion_board_cs1;
	if (machine_is_dm3730_som_lv())
		omap_mux_init_signal("sdmmc2_dat2.mcspi3_cs1", OMAP_PIN_INPUT);
	else
		omap_mux_init_signal("etk_d7.mcspi3_cs1", OMAP_PIN_INPUT);
	use_mcspi3=1;
#endif

    if (use_mcspi1) {
        omap_mux_init_signal("mcspi1_clk", OMAP_PIN_INPUT);
        omap_mux_init_signal("mcspi1_simo", OMAP_PIN_INPUT);
        omap_mux_init_signal("mcspi1_somi", OMAP_PIN_INPUT);
    }

    if (use_mcspi3) {
        if (machine_is_dm3730_som_lv()) {

            omap_mux_init_signal("sdmmc2_clk.mcspi3_clk", OMAP_PIN_INPUT);
            omap_mux_init_signal("sdmmc2_cmd.mcspi3_simo", OMAP_PIN_INPUT);
            omap_mux_init_signal("sdmmc2_dat0.mcspi3_somi", OMAP_PIN_INPUT);
        } else {
            omap_mux_init_signal("etk_d3.mcspi3_clk", OMAP_PIN_INPUT);
            omap_mux_init_signal("etk_d0.mcspi3_simo", OMAP_PIN_INPUT);
            omap_mux_init_signal("etk_d1.mcspi3_somi", OMAP_PIN_INPUT);
        }
    }

    if (nspi)
        spi_register_board_info(omap3logic_spi_devices, nspi);
}

#if defined(CONFIG_BT_HCIBRF6300_SPI) || defined(CONFIG_BT_HCIBRF6300_SPI_MODULE)

int brf6300_request_shutdown_gpio(struct brf6300_platform_data *pdata)
{
	struct clk *sys_clkout1_clk;
	int ret;

	// printk("%s: pdata->shutdown_gpio %d\n", __func__, pdata->shutdown_gpio);

	BUG_ON(!pdata->shutdown_gpio);

	/* Enable sys_clkout1 (uP_CLKOUT1_26Mhz used by brf6300 */
	sys_clkout1_clk = clk_get(NULL, "sys_clkout1");
	if (IS_ERR(sys_clkout1_clk)) {
		printk("%s: Can't get sys_clkout1\n", __FUNCTION__);
		return -1;
	}
	clk_enable(sys_clkout1_clk);

	ret = gpio_request(pdata->shutdown_gpio, "BT_nSHUTDOWN");
	// printk("%s: request(%d) = %d\n", __func__, pdata->shutdown_gpio, ret);
	return ret;
}

void brf6300_free_shutdown_gpio(struct brf6300_platform_data *pdata)
{
	BUG_ON(!pdata->shutdown_gpio);
	// printk("%s: gpio %d\n", __func__, pdata->shutdown_gpio);
	gpio_free(pdata->shutdown_gpio);
}

int brf6300_set_shutdown_gpio_direction(struct brf6300_platform_data *pdata, int direction, int value)
{
	int ret;
	BUG_ON(!pdata->shutdown_gpio);
	if (!direction)
		ret = gpio_direction_output(pdata->shutdown_gpio, value);
	else
		ret = gpio_direction_input(pdata->shutdown_gpio);
	// printk("%s: direction %d value %d ret %d\n", __func__, direction, value, ret);
	return ret;
}

void brf6300_set_shutdown_gpio(struct brf6300_platform_data *pdata, int value)
{
	// printk("%s: value %d\n", __func__, value);
	BUG_ON(!pdata->shutdown_gpio);
	gpio_set_value_cansleep(pdata->shutdown_gpio, value);
}

int brf6300_get_shutdown_gpio(struct brf6300_platform_data *pdata)
{
	int ret;
	BUG_ON(!pdata->shutdown_gpio);
	ret = gpio_get_value_cansleep(pdata->shutdown_gpio);
	// printk("%s: ret %d\n", __func__, ret);
	return ret;
}

#ifdef BRF6300_DEBUG_GPIO
void brf6300_set_debug_gpio(struct brf6300_platform_data *pdata, int which, int value)
{
	if (which)
		gpio_set_value(pdata->debug_gpio1, value);
	else
		gpio_set_value(pdata->debug_gpio2, value);

}
#endif

static void brf6300_dev_init(void)
{
	printk("%s\n", __func__);
	/* Only the LV SOM has a BRF6300 */
	if (!machine_is_dm3730_som_lv())
		return;

	if (!omap3logic_twl_gpio_base) {
		printk(KERN_ERR "Huh?!? omap3logic_twl_gpio_base not set!\n");
		BUG();
		return;
	}

	brf6300_config.irq_gpio = BT_IRQ_GPIO;
	brf6300_config.shutdown_gpio = omap3logic_twl_gpio_base + TWL4030_BT_nSHUTDOWN;
	brf6300_config.request_shutdown_gpio = brf6300_request_shutdown_gpio;
	brf6300_config.free_shutdown_gpio = brf6300_free_shutdown_gpio;
	brf6300_config.set_shutdown_gpio_direction = brf6300_set_shutdown_gpio_direction;
	brf6300_config.set_shutdown_gpio = brf6300_set_shutdown_gpio;
	brf6300_config.get_shutdown_gpio = brf6300_get_shutdown_gpio;

#ifdef BRF6300_DEBUG_GPIO
	brf6300_config.set_debug_gpio = brf6300_set_debug_gpio;
	/* IRQ entry signaled by pulsing GPIO_LED1_SOM_LV */
	brf6300_config.debug_gpio1 = GPIO_LED1_SOM_LV;
	/* IRQ enable state signaled by GPIO_LED2_SOM_LV */
	brf6300_config.debug_gpio2 = GPIO_LED2_SOM_LV;
#endif
	omap_mux_init_gpio(brf6300_config.irq_gpio, OMAP_PIN_INPUT_PULLUP); /* GPIO_157 */
	if (gpio_request(brf6300_config.irq_gpio, "BRF6300 IRQ") < 0)
		printk(KERN_ERR "can't get BRF6300 irq GPIO\n");

	gpio_direction_input(brf6300_config.irq_gpio);
}
#else
static void brf6300_dev_init(void)
{
}
#endif

static void __init omap3logic_init_early(void)
{
	omap2_init_common_infrastructure();
	omap2_init_common_devices(NULL, NULL);
}

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	OMAP3_MUX(SYS_NIRQ, OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_WAKEUPENABLE | OMAP_MUX_MODE0),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#endif

static const struct usbhs_omap_board_data usbhs_bdata __initconst = {

	.port_mode[0] = OMAP_EHCI_PORT_MODE_PHY,
	.port_mode[1] = OMAP_EHCI_PORT_MODE_PHY,
	.port_mode[2] = OMAP_USBHS_PORT_MODE_UNUSED,

	.phy_reset  = true,
	.reset_gpio_port[0]  = -EINVAL,
	.reset_gpio_port[1]  = 4,
	.reset_gpio_port[2]  = -EINVAL
};

static void omap3logic_init_ehci(void)
{
	omap_mux_init_gpio(usbhs_bdata.reset_gpio_port[1], OMAP_PIN_OUTPUT);
	usbhs_init(&usbhs_bdata);
}

static void omap3logic_usb_init(void)
{
	if (machine_is_omap3530_lv_som() || machine_is_dm3730_som_lv())
		omap3logic_init_ehci();
	else
		omap3logic_init_isp1763();
}

#if defined(CONFIG_VIDEO_OMAP3) || defined(CONFIG_VIDEO_OMAP3_MODULE)
/*
 * Configure the Camera connection pins
*/
static void dm3730logic_camera_init(void)
{
        omap_mux_init_signal("cam_hs.cam_hs",OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_vs.cam_vs",OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_xclka.cam_xclka", OMAP_PIN_INPUT);
//        omap_mux_init_signal("cam_xclkb.cam_xclkb", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_pclk.cam_pclk", OMAP_PIN_INPUT);

        omap_mux_init_signal("cam_d0.cam_d0", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_d1.cam_d1", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_d2.cam_d2", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_d3.cam_d3", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_d4.cam_d4", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_d5.cam_d5", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_d6.cam_d6", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_d7.cam_d7", OMAP_PIN_INPUT);

#if 0
	printk("%s: muxing 8-bit interface (cam_d0-cam_d7)\n", __FUNCTION__);
#else
	printk("%s: muxing 12-bit interface (cam_d0-cam_d11)\n", __FUNCTION__);
        omap_mux_init_signal("cam_d8.cam_d8", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_d9.cam_d9", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_d10.cam_d10", OMAP_PIN_INPUT);
        omap_mux_init_signal("cam_d11.cam_d11", OMAP_PIN_INPUT);
#endif

}
#endif

#if defined(CONFIG_OMAP3LOGIC_COMPACT_FLASH) || defined(CONFIG_OMAP3LOGIC_COMPACT_FLASH_MODULE)

#define DM3730_SOM_LV_CF_RESET_GPIO 6
#define DM3730_SOM_LV_CF_EN_GPIO 128
#define DM3730_SOM_LV_CF_CD_GPIO 154

static struct resource omap3logic_som_lv_cf_resources[] = {
	[0] = {
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = OMAP_GPIO_IRQ(DM3730_SOM_LV_CF_CD_GPIO),
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL,
	},
};

static struct omap3logic_cf_data cf_data = {
	.gpio_reset = DM3730_SOM_LV_CF_RESET_GPIO,
	.gpio_en = DM3730_SOM_LV_CF_EN_GPIO,
	.gpio_cd = DM3730_SOM_LV_CF_CD_GPIO,
};

static struct platform_device omap3logic_som_lv_cf = {
	.name		= "omap3logic-cf",
	.id		= 0,
	.dev		= {
		.platform_data	= &cf_data,
	},
	.num_resources = ARRAY_SIZE(omap3logic_som_lv_cf_resources),
	.resource = omap3logic_som_lv_cf_resources,
};

void omap3logic_cf_init(void)
{
	unsigned long cs_mem_base;
	int result;

	/* Only the LV SOM SDK has a CF interface */
	if (!machine_is_dm3730_som_lv())
		return;

	/* Fix PBIAS to get USIM enough voltage to power up */
	omap3torpedo_fix_pbias_voltage();

	if (gpmc_cs_request(3, SZ_16M, &cs_mem_base) < 0) {
		printk(KERN_ERR "Failed to request GPMC mem for CF\n");
		return;
	}

	omap3logic_som_lv_cf_resources[0].start = cs_mem_base;
	omap3logic_som_lv_cf_resources[0].end = cs_mem_base + 0x1fff;

	omap_mux_init_signal("gpmc_ncs3", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("gpmc_io_dir", OMAP_PIN_OUTPUT);

	omap_mux_init_gpio(DM3730_SOM_LV_CF_CD_GPIO, OMAP_PIN_INPUT_PULLUP);
	if (gpio_request(DM3730_SOM_LV_CF_CD_GPIO, "CF card detect") < 0) {
		printk(KERN_ERR "Failed to request GPIO%d for CompactFlash CD IRQ\n",
		DM3730_SOM_LV_CF_CD_GPIO);
		return;
	}
	gpio_set_debounce(DM3730_SOM_LV_CF_CD_GPIO, 0xa);
	gpio_direction_input(DM3730_SOM_LV_CF_CD_GPIO);
	gpio_export(DM3730_SOM_LV_CF_CD_GPIO, 0);

	// Setup ComapctFlash Enable pin
	omap_mux_init_gpio(DM3730_SOM_LV_CF_EN_GPIO, OMAP_PIN_OUTPUT);
	if (gpio_request(DM3730_SOM_LV_CF_EN_GPIO, "CF enable") < 0) {
		printk(KERN_ERR "Failed to request GPIO%d for CompactFlash EN\n",
		DM3730_SOM_LV_CF_EN_GPIO);
		return;
	}
	gpio_direction_output(DM3730_SOM_LV_CF_EN_GPIO, 0);
	gpio_export(DM3730_SOM_LV_CF_EN_GPIO, 0);

	// Setup ComapctFlash Reset pin
	omap_mux_init_gpio(DM3730_SOM_LV_CF_RESET_GPIO, OMAP_PIN_OUTPUT);
	if (gpio_request(DM3730_SOM_LV_CF_RESET_GPIO, "CF reset") < 0) {
		printk(KERN_ERR "Failed to request GPIO%d for CompactFlash Reset\n",
		DM3730_SOM_LV_CF_RESET_GPIO);
		return;
	}
	gpio_direction_output(DM3730_SOM_LV_CF_RESET_GPIO, 0);
	gpio_export(DM3730_SOM_LV_CF_RESET_GPIO, 0);

	result = platform_device_register(&omap3logic_som_lv_cf);
	if (result)
		printk("%s: platform device register of CompactFlash device failed: %d\n", __FUNCTION__, result);
}
#else
static void omap3logic_cf_init(void)
{
}
#endif

/* Code that is invoked only after
 * the product ID data has been found; used for finer-grain
 * board configuration
 */
void omap3logic_init_productid_specifics(void)
{
	omap3logic_init_twl_audio();
}

#ifdef CONFIG_PRINTK_DEBUG
struct printk_debug *printk_debug;
static int __init printk_debug_setup(char *str)
{
	/* printk debug buffer is in start of DRAM; u-boot will look
	 * there for printk_buffer information */
	printk_debug = (void *)PAGE_OFFSET;
	return 1;
}
__setup("printk-debug", printk_debug_setup);
#endif

static void __init omap3logic_opp_init(void)
{
	int ret = 0;
	int speed_mhz;

	if (omap3_opp_init()) {
		pr_err("%s: opp default init failed\n", __func__);
		return;
	}

	/* Custom OPP enabled for DM37x versions */
	if (cpu_is_omap3630()) {
		struct device *mpu_dev, *iva_dev;

		mpu_dev = omap_device_get_by_hwmod_name("mpu");
		if (omap3_has_iva())
			iva_dev = omap_device_get_by_hwmod_name("iva");
		else
			iva_dev = NULL;

		if (!mpu_dev) {
			pr_err("%s: Aiee.. no mpu device?\n", __func__);
			return;
		}

		/* Try to get the product ID max speed.  If not found
		 * default to 800Mhz max */
		if (omap3logic_extract_speed_mhz(&speed_mhz))
			speed_mhz=800;

		if (speed_mhz >= 1000) {
			/* Enable MPU 1GHz opp */
			ret |= opp_enable(mpu_dev, 1000000000);
			/* MPU 1GHz requires:
			 * - Smart Reflex (SR)
			 * - Adaptive Body Bias (ABB)
			 * - Operation restricted below 90C
			 *   junction temp
			 * !! TODO: add 90C junction temp
			 *          throttle service   */
			/* Enable IVA 800Mhz opp */
			if (iva_dev)
				ret |= opp_enable(iva_dev, 800000000);
			if (ret) {
				pr_err("%s: failed to enable opp %d\n",
					__func__, ret);

				/* Cleanup - disable the higher freqs
				 * - we don't care about the results */
				opp_disable(mpu_dev, 1000000000);
				if (iva_dev)
					opp_disable(iva_dev, 800000000);
			}
		}

		if (speed_mhz >= 800) {
			/* Enable MPU 800MHz opp */
			ret |= opp_enable(mpu_dev, 800000000);
			/* MPU 800MHz requires:
			 * - Operation restricted below 90C
			 *   junction temp
			 * !! TODO: add 90C junction temp
			 *          throttle service   */

			/* Enable IVA 660Mhz opp */
			if (iva_dev)
				ret |= opp_enable(iva_dev, 660000000);
			if (ret) {
				pr_err("%s: failed to enable opp %d\n",
					__func__, ret);

				/* Cleanup - disable the higher freqs
				 * - we don't care about the results */
				opp_disable(mpu_dev, 800000000);
				if (iva_dev)
					opp_disable(iva_dev, 660000000);
			}
		}
	}
}

static void __init omap3logic_pm_init(void)
{
	/* Use sys_offmode signal                                       */
	/* We use the off mode signal instead of I2C to trigger scripts */
	/* loaded in the PMIC to achieve very low standby power.        */
	omap_pm_sys_offmode_select(1);

	/* sys_clkreq - active high */
	omap_pm_sys_clkreq_pol(1);

	/* sys_offmode - active low */
	omap_pm_sys_offmode_pol(0);

	/* Automatically send OFF command */
	omap_pm_auto_off(1);

	/* Automatically send RET command */
	omap_pm_auto_ret(0);
}

#define PWR_P1_SW_EVENTS	0x10
#define PWR_DEVOFF			(1<<0)
#define REMAP_OFFSET		2

static void omap3logic_poweroff(void)
{
	u8 val;
	int err;
	printk(KERN_INFO "twl4030_poweroff()\n");

	err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
						 PWR_P1_SW_EVENTS);
	if (err) {
			printk(KERN_WARNING "I2C error %d while reading TWL4030"
				  "PM_MASTER P1_SW_EVENTS\n", err);
			return ;
	}

	val |= PWR_DEVOFF;

	err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
						  PWR_P1_SW_EVENTS);

	if (err) {
			printk(KERN_WARNING "I2C error %d while writing TWL4030"
				  "PM_MASTER P1_SW_EVENTS\n", err);
			return ;
	}
}

#if defined(CONFIG_HDQ_MASTER_OMAP) || defined(CONFIG_HDQ_MASTER_OMAP_MODULE)
static void omap3logic_hdq_init(void)
{
	/* Initialize HDQ_SIO for 1-wire communications */
	omap_mux_init_signal("hdq_sio.hdq_sio", OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);
}
#else
static void omap3logic_hdq_init(void)
{
}
#endif
static void __init omap3logic_init(void)
{
	struct omap_board_data bdata;

	/* hang on start if "hang" is on command line */
	while (omap3logic_hang)
		;

	pm_power_off = omap3logic_poweroff;

	/* Pick the right MUX table based on the machine */
	if (machine_is_dm3730_som_lv() || machine_is_dm3730_torpedo())
		omap3_mux_init(board_mux, OMAP_PACKAGE_CBP);
	else if (machine_is_omap3530_lv_som() || machine_is_omap3_torpedo())
		omap3_mux_init(board_mux, OMAP_PACKAGE_CBB);

	omap3torpedo_fix_pbias_voltage();
	omap3logic_i2c_init();
	omap3logic_lcd_init();

#ifdef CONFIG_OMAP3LOGIC_UART_A
	printk(KERN_INFO "Setup pinmux and enable UART A\n");
	omap_mux_init_signal("uart1_tx.uart1_tx", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("uart1_rts.uart1_rts", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("uart1_cts.uart1_cts", OMAP_PIN_INPUT);
	omap_mux_init_signal("uart1_rx.uart1_rx", OMAP_PIN_INPUT);

	// Taken from serial.c:omap_serial_init()
	bdata.id = 0;
	bdata.flags = 0;
	bdata.pads = NULL;
	bdata.pads_cnt = 0;
	omap_serial_init_port(&bdata);
#endif

#ifdef CONFIG_OMAP3LOGIC_UART_B
	printk(KERN_INFO "Setup pinmux and enable UART B\n");
	omap_mux_init_signal("uart2_tx.uart2_tx", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("uart2_rts.uart2_rts", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("uart2_cts.uart2_cts", OMAP_PIN_INPUT);
	omap_mux_init_signal("uart2_rx.uart2_rx", OMAP_PIN_INPUT);

	// Taken from serial.c:omap_serial_init()
	bdata.id = 1;
	bdata.flags = 0;
	bdata.pads = NULL;
	bdata.pads_cnt = 0;
	omap_serial_init_port(&bdata);
#endif

#ifdef CONFIG_OMAP3LOGIC_UART_C
	printk(KERN_INFO "Setup pinmux and enable UART C\n");
	omap_mux_init_signal("uart3_tx_irtx.uart3_tx_irtx", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("uart3_rts_sd.uart3_rts_sd", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("uart3_cts_rctx.uart3_cts_rctx", OMAP_PIN_INPUT);
	omap_mux_init_signal("uart3_rx_irrx.uart3_rx_irrx", OMAP_PIN_INPUT);

	// Taken from serial.c:omap_serial_init()
	bdata.id = 2;
	bdata.flags = 0;
	bdata.pads = NULL;
	bdata.pads_cnt = 0;
	omap_serial_init_port(&bdata);
#endif

	board_mmc_init();
	board_smsc911x_init();

	/* Assume NOR is only on CS2 (if its there) */
	omap3logic_nor_init(1<<2, SZ_8M);
	omap3logic_nand_init();

	/* Initialixe EHCI port */
	omap3logic_usb_init();

	/* Initialise OTG MUSB port */
	omap3logic_musb_init();

	/* Initialise ComapctFlash */
	omap3logic_cf_init();

	/* Initialize hdq (Dallas 1-wire) */
	omap3logic_hdq_init();

	/* Ensure SDRC pins are mux'd for self-refresh */
	omap_mux_init_signal("sdrc_cke0", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("sdrc_cke1", OMAP_PIN_OUTPUT);

	omap3logic_pm_init();

	/* Smart reflex must be enabled for higher OPP levels! */
	omap_enable_smartreflex_on_init();
#if 1
	omap3logic_opp_init();
#endif
#if defined(CONFIG_VIDEO_OMAP3)
	dm3730logic_camera_init();
	omap3_init_camera(&omap3logic_isp_platform_data);
#endif
#if !(defined(CONFIG_GPIO_TWL4030) || defined(CONFIG_GPIO_TWL4030_MODULE))
	omap3logic_led_init();
#endif
}

MACHINE_START(OMAP3_TORPEDO, "Logic OMAP35x Torpedo board")
	.boot_params	= 0x80000100,
	.reserve	= omap_reserve,
	.map_io		= omap3_map_io,
	.init_early	= omap3logic_init_early,
	.init_irq	= omap_init_irq,
	.init_machine	= omap3logic_init,
	.timer		= &omap_timer,
MACHINE_END

MACHINE_START(OMAP3530_LV_SOM, "Logic OMAP35x SOM LV board")
	.boot_params	= 0x80000100,
	.reserve	= omap_reserve,
	.map_io		= omap3_map_io,
	.init_early	= omap3logic_init_early,
	.init_irq	= omap_init_irq,
	.init_machine	= omap3logic_init,
	.timer		= &omap_timer,
MACHINE_END

MACHINE_START(DM3730_SOM_LV, "Logic DM37x SOM LV board")
	.boot_params	= 0x80000100,
	.reserve	= omap_reserve,
	.map_io		= omap3_map_io,
	.init_early	= omap3logic_init_early,
	.init_irq	= omap_init_irq,
	.init_machine	= omap3logic_init,
	.timer		= &omap_timer,
MACHINE_END

MACHINE_START(DM3730_TORPEDO, "Logic DM37x Torpedo board")
	.boot_params	= 0x80000100,
	.reserve	= omap_reserve,
	.map_io		= omap3_map_io,
	.init_early	= omap3logic_init_early,
	.init_irq	= omap_init_irq,
	.init_machine	= omap3logic_init,
	.timer		= &omap_timer,
MACHINE_END
