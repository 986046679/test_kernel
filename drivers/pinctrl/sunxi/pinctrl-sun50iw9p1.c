/*
 * Allwinner sun50iw9p1 SoCs pinctrl driver.
 *
 * Copyright(c) 2012-2016 Allwinnertech Co., Ltd.
 * Author: WimHuang <huangwei@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun50iw9p1_pins[] = {
	//Register Name: PA_CFG0
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac1"),		/* RXD1 */
		SUNXI_FUNCTION(0x4, "twi0"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 0),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac1"),		/* RXD0 */
		SUNXI_FUNCTION(0x4, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x5, "Vdevice"),     /* For Test */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 1),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac1"),		/* CRTS_DV */
		SUNXI_FUNCTION(0x4, "twi1"),		/* SCK */
		SUNXI_FUNCTION(0x5, "Vdevice"),     /* For Test */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 2),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac1"),		/* RXER */
		SUNXI_FUNCTION(0x4, "twi1"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 3),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac1"),		/* TXD1*/
		SUNXI_FUNCTION(0x4, "ac_adcy"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 4),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac1"),		/* TXD0 */
		SUNXI_FUNCTION(0x3, "h_i2s0"),		/* DOUT0 */
		SUNXI_FUNCTION(0x4, "ac_adcx"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 5),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac1"),		/* TXCK */
		SUNXI_FUNCTION(0x3, "h_i2s0"),		/* MCLK */
		SUNXI_FUNCTION(0x4, "ac_mclk"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 6),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac1"),		/* TXEN */
		SUNXI_FUNCTION(0x3, "h_i2s0"),		/* BCLK */
		SUNXI_FUNCTION(0x4, "ac_sync"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 7),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PA_CFG1
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac1"),		/* MDC */
		SUNXI_FUNCTION(0x3, "h_i2s0"),		/* LRCK */
		SUNXI_FUNCTION(0x4, "ac_adcl"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 8),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac1"),		/* MDIO */
		SUNXI_FUNCTION(0x3, "h_i2s0"),		/* DIN0 */
		SUNXI_FUNCTION(0x4, "ac_adcr"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 9),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi3"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 10),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi3"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 11),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pwm5"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 12),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	/* HOLE */
	//Register Name: PC_CFG0
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* WE */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* DS */
		SUNXI_FUNCTION(0x4, "spi0"),		/* CLK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 0),  /* PC_EINT0 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* ALE */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* RST */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 1),  /* PC_EINT1 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* CLE */
		SUNXI_FUNCTION(0x4, "spi0"),		/* MOSI */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 2),  /* PC_EINT2 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* CE1 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* CS0 */
		SUNXI_FUNCTION(0x5, "boot"),		/* SEL1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 3),  /* PC_EINT3 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* CE0 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* MISO */
		SUNXI_FUNCTION(0x5, "boot"),		/* SEL2 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 4),  /* PC_EINT4 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* RE */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* CLK */
		SUNXI_FUNCTION(0x5, "boot"),		/* SEL3 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 5),  /* PC_EINT5 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* RB0 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* CMD */
		SUNXI_FUNCTION(0x5, "boot"),		/* SEL4 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 6),  /* PC_EINT6 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* RB1 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* CS1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 7),  /* PC_EINT7 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PC_CFG1
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ7 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D3 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 8),  /* PC_EINT8 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ6 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D4 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 9),  /* PC_EINT9 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ5 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D0 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 10),  /* PC_EINT10 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ4 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D5 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 11),  /* PC_EINT11 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQS */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 12),  /* PC_EINT12 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ3 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 13),  /* PC_EINT13 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ2 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D6 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 14),  /* PC_EINT14 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ1 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D2 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* WP */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 15),  /* PC_EINT15 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "nand0"),		/* DQ0 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D7 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* HOLD */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 16),  /* PC_EINT16 */
		SUNXI_FUNCTION(0x7, "io_disabled")),

	/* HOLE */
	//Register Name: PD_CFG0
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D0 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* VP0 */
		SUNXI_FUNCTION(0x4, "ts0"),			/* CLK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 0),  /* PD_EINT0 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D1 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* VN0 */
		SUNXI_FUNCTION(0x4, "ts0"),			/* ERR */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 1),  /* PD_EINT1 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D2 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* VP1 */
		SUNXI_FUNCTION(0x4, "ts0"),			/* SYNC */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 2),  /* PD_EINT2 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D3 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* VN1 */
		SUNXI_FUNCTION(0x4, "ts0"),			/* DVLD */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 3),  /* PD_EINT3 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D4 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* VP2 */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D0 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 4),  /* PD_EINT4 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D5 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* VN2 */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 5),  /* PD_EINT5 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D6 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* VPC */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D2 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 6),  /* PD_EINT6 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D7 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* VNC */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D3 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 7),  /* PD_EINT7 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PD_CFG1
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D8 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* VP3 */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D4 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 8),  /* PD_EINT8 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D9 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* VN3 */
		SUNXI_FUNCTION(0x4, "ts0"), 		/* D5 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 9),  /* PD_EINT9 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D10 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* VP0 */
		SUNXI_FUNCTION(0x4, "ts0"), 		/* D6 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 10),  /* PD_EINT10 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D11 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* VN0 */
		SUNXI_FUNCTION(0x4, "ts0"), 		/* D7 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 11),  /* PD_EINT11 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D12 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* VP1 */
		SUNXI_FUNCTION(0x4, "sim0"), 		/* VPPEN */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 12),  /* PD_EINT12 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D13 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* VN1 */
		SUNXI_FUNCTION(0x4, "sim0"),		/* VPPPP */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 13),  /* PD_EINT13 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D14 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* VP2 */
		SUNXI_FUNCTION(0x4, "sim0"),		/* PWREN */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 14),  /* PD_EINT14 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D15 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* VN2 */
		SUNXI_FUNCTION(0x4, "sim0"),		/* CLK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 15),  /* PD_EINT15 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PD_CFG2
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D16 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* VPC */
		SUNXI_FUNCTION(0x4, "sim0"),		/* DATA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 16),  /* PD_EINT16 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D17 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* VNC */
		SUNXI_FUNCTION(0x4, "sim0"),		/* RST */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 17),  /* PD_EINT17 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D18 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* VP3 */
		SUNXI_FUNCTION(0x4, "sim0"),		/* DET */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 18),  /* PD_EINT18 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D19 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* VN3 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 19),  /* PD_EINT19 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D20 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 20),  /* PD_EINT20 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D21 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 21),  /* PD_EINT21 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D22 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 22),  /* PD_EINT22 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D23 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 23),  /* PD_EINT23 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PD_CFG3
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* CLK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 24),  /* PD_EINT24 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* DE */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 25),  /* PD_EINT25 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* HSYNC */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 26),  /* PD_EINT26 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* VSYNC */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 27),  /* PD_EINT27 */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pwm0"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 28),  /* PD_EINT28 */
		SUNXI_FUNCTION(0x7, "io_disabled")),

	/* HOLE */
	//Register Name: PE_CFG0
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  PCLK  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 0),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi_mclk1"),  /*  MCLK  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 1),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  HSYNC  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 2),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  VSYNC  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 3),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D0  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 4),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D1  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 5),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D2  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 6),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D3  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 7),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PE_CFG1
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D4  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 8),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D5  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 9),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D6  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 10),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D7  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 11),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D8  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 12),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D9  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 13),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D10  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 14),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D11  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 15),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PE_CFG2
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D12  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 16),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D13  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 17),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D14  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 18),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi1"),  /*  D15  */
		SUNXI_FUNCTION(0x3, "jtag"),   /*  MS  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 19),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi_cci1"),  /*  SCK  */
		SUNXI_FUNCTION(0x3, "jtag"),   /*  CK  */
		SUNXI_FUNCTION(0x5, "twi2"),   /*  SCK  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 20),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi_cci1"),  /*  SDA */
		SUNXI_FUNCTION(0x3, "jtag"),   /*  DO   */
		SUNXI_FUNCTION(0x5, "twi2"),   /*  SDA  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 21),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi_fsin0"),    /*  FSIN0 */
		SUNXI_FUNCTION(0x3, "jtag"),   /*  DI   */
		SUNXI_FUNCTION(0x4, "TCON"),   /*  TRIG0  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 22),
		SUNXI_FUNCTION(0x7, "io_disabled")),

	/* HOLE */
	//Register Name: PF_CFG0
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),   /*  D1  */
		SUNXI_FUNCTION(0x3, "jtag"),   /*  MS1  */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 0),  /*  PF_EINT0  */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* D0 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* DI1 */  //@Martin: jtag0? why previous is DI and here is DI1?
		SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 1),  /*  PF_EINT1  */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* CLK */
		SUNXI_FUNCTION(0x3, "uart0"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 2),  /*  PF_EINT2  */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* CMD */
		SUNXI_FUNCTION(0x3, "jtag"),		/* DO1 */  //@Martin: jtag0? why previous is DI and here is DI1?
		SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 3),  /*  PF_EINT3  */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* D3 */
		SUNXI_FUNCTION(0x3, "uart0"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 4),  /*  PF_EINT4  */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* D2 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* CK1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 5),  /*  PF_EINT5  */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "jtag"),		/* SEL */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 6),  /*  PF_EINT6  */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	/* HOLE */
	//Register Name: PG_CFG0
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* CLK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 0),  /*  PG_EINT0  */
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* CMD */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 1),  /*  PG_EINT1	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D0 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 2),  /*  PG_EINT2	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 3),  /*  PG_EINT3	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D2 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 4),  /*  PG_EINT4	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D3 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 5),  /*  PG_EINT5	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),  /* TX */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 6),  /*  PG_EINT6	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 7),  /*  PG_EINT7	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PG_CFG1
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* RTS */
		SUNXI_FUNCTION(0x3, "pll0"),		/* STA_DB */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 8),  /*  PG_EINT8	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* CTS */
		SUNXI_FUNCTION(0x3, "pll0"),		/* TEST_GPIO */
		SUNXI_FUNCTION(0x5, "ac"),		/* ADCY */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 9),  /*  PG_EINT9	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "h_i2s2"),		/* MCLK */
		SUNXI_FUNCTION(0x3, "x32kfout"),
		SUNXI_FUNCTION(0x5, "ac"),		/* MCLK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 10),  /*  PG_EINT10	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "h_i2s2"),		/* BCLK */
		SUNXI_FUNCTION(0x4, "bist0"),		/* RESULT0 */
		SUNXI_FUNCTION(0x5, "ac"),		/* SYNC */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 11),  /*  PG_EINT11	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "h_i2s2"),		/* LRLK */
		SUNXI_FUNCTION(0x4, "bist0"),		/* RESULT1 */
		SUNXI_FUNCTION(0x5, "ac"),		/* ADCL */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 12),  /*  PG_EINT12	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "h_i2s2"),		/* DOUT0 */
		SUNXI_FUNCTION(0x3, "h_i2s2"),		/* DIN1 */
		SUNXI_FUNCTION(0x4, "bist0"),		/* RESULT2 */
		SUNXI_FUNCTION(0x5, "ac"),		/* ADCR */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 13),  /*  PG_EINT13	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "h_i2s2"),		/* DIN0 */
		SUNXI_FUNCTION(0x3, "h_i2s2"),		/* DOUT1 */
		SUNXI_FUNCTION(0x4, "bist0"),		/* RESULT3 */
		SUNXI_FUNCTION(0x5, "ac"),			/* ADCX */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 14),  /*  PG_EINT14	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* TX */
		SUNXI_FUNCTION(0x5, "twi4"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 15),  /*  PG_EINT15	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PG_CFG2
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* RX */
		SUNXI_FUNCTION(0x5, "twi4"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 16),  /*  PG_EINT16	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* RTS */
		SUNXI_FUNCTION(0x3, "csi_cci0"),		/* SCK */
		SUNXI_FUNCTION(0x5, "twi3"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 17),  /*  PG_EINT17	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* CTS */
		SUNXI_FUNCTION(0x3, "csi_cci0"),		/* SDA */
		SUNXI_FUNCTION(0x5, "twi3"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 18),  /*  PG_EINT18	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "csi_mclk0"),		/* MCLK */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 19),  /*  PG_EINT19	*/
		SUNXI_FUNCTION(0x7, "io_disabled")),

	/* HOLE */
	//Register Name: PH_CFG0
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),	/* TX */
		SUNXI_FUNCTION(0x4, "pwm3"),
		SUNXI_FUNCTION(0x5, "twi1"),	/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 0),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart0"),	/* RX */
		SUNXI_FUNCTION(0x4, "pwm4"),
		SUNXI_FUNCTION(0x5, "twi1"),	/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 1),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart5"),	/* TX */
		SUNXI_FUNCTION(0x3, "spdif"),	/* CLK */
		SUNXI_FUNCTION(0x4, "pwm2"),
		SUNXI_FUNCTION(0x5, "twi2"),	/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 2),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart5"),	/* RX */
		SUNXI_FUNCTION(0x3, "spdif"),	/* IN */
		SUNXI_FUNCTION(0x4, "pwm1"),
		SUNXI_FUNCTION(0x5, "twi2"),	/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 3),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "spdif"),	/* OUT */
		SUNXI_FUNCTION(0x5, "twi3"),	/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 4),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* TX */
		SUNXI_FUNCTION(0x3, "h_i2s3"),		/* MCLK */
		SUNXI_FUNCTION(0x4, "spi1"),		/* CS0 */
		SUNXI_FUNCTION(0x5, "twi3"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 5),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* RX */
		SUNXI_FUNCTION(0x3, "h_i2s3"),		/* BCLK */
		SUNXI_FUNCTION(0x4, "spi1"),		/* CLK */
		SUNXI_FUNCTION(0x5, "twi4"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 6),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* RTS */
		SUNXI_FUNCTION(0x3, "h_i2s3"),		/* LRLK */
		SUNXI_FUNCTION(0x4, "spi1"),		/* MOSI */
		SUNXI_FUNCTION(0x5, "twi4"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 7),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PH_CFG1
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* CTS */
		SUNXI_FUNCTION(0x3, "h_i2s3"),		/* DOUT0 */
		SUNXI_FUNCTION(0x4, "spi1"),		/* MISO */
		SUNXI_FUNCTION(0x5, "h_i2s3"),		/* DIN1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 8),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "h_i2s3"),		/* DIN0 */
		SUNXI_FUNCTION(0x4, "spi1"),		/* CS1 */
		SUNXI_FUNCTION(0x5, "h_i2s3"),		/* DOUT1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 9),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "ir"),		/* RX */
		SUNXI_FUNCTION(0x4, "tcon"),	/* TRIG1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 10),
		SUNXI_FUNCTION(0x7, "io_disabled")),

	/* HOLE */
	//Register Name: PI_CFG0
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RXD3 / RMII_NULL */
		SUNXI_FUNCTION(0x3, "dmic"),		/* CLK */
		SUNXI_FUNCTION(0x4, "h_i2s0"),		/* MCLK */
		SUNXI_FUNCTION(0x5, "hddc"),		/* SCL */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 0),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RXD2 / RMII_NULL */
		SUNXI_FUNCTION(0x3, "dmic"),		/* DATA0 */
		SUNXI_FUNCTION(0x4, "h_i2s0"),		/* BCLK */
		SUNXI_FUNCTION(0x5, "hddc"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 1),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RXD1 / RMII_RXD1 */
		SUNXI_FUNCTION(0x3, "dmic"),		/* DATA1 */
		SUNXI_FUNCTION(0x4, "h_i2s0"),		/* LRLK */
		SUNXI_FUNCTION(0x5, "hcec"),		/* CEC */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 2),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RXD0 / RMII_RXD0 */
		SUNXI_FUNCTION(0x3, "dmic"),		/* DATA2 */
		SUNXI_FUNCTION(0x4, "h_i2s0"),		/* DOUT0 */
		SUNXI_FUNCTION(0x5, "h_i2s0"),		/* DIN1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 3),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RXCK / RMII_NULL */
		SUNXI_FUNCTION(0x3, "dmic"),		/* DATA3 */
		SUNXI_FUNCTION(0x4, "h_i2s0"),		/* DIN0 */
		SUNXI_FUNCTION(0x5, "h_i2s0"),		/* DOUT1 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 4),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* RXCT / RMII_CRS_DV */
		SUNXI_FUNCTION(0x3, "uart2"),		/* TX */
		SUNXI_FUNCTION(0x4, "ts0"),			/* CLK */
		SUNXI_FUNCTION(0x5, "twi0"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 5),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* NULL / RMII_RXER */
		SUNXI_FUNCTION(0x3, "uart2"),		/* RX */
		SUNXI_FUNCTION(0x4, "ts0"),			/* ERR */
		SUNXI_FUNCTION(0x5, "twi0"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 6),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* TXD3 / RMII_NULL */
		SUNXI_FUNCTION(0x3, "uart2"),		/* RTS */
		SUNXI_FUNCTION(0x4, "ts0"),			/* SYNC */
		SUNXI_FUNCTION(0x5, "twi1"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 7),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PI_CFG1
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* TXD2 / RMII_NULL */
		SUNXI_FUNCTION(0x3, "uart2"),		/* RTS */
		SUNXI_FUNCTION(0x4, "ts0"),			/* DVLD */
		SUNXI_FUNCTION(0x5, "twi1"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 8),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* TXD1 / RMII_TXD1 */
		SUNXI_FUNCTION(0x3, "uart3"),		/* TX */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D0 */
		SUNXI_FUNCTION(0x5, "twi2"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 9),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* TXD0 / RMII_TXD0 */
		SUNXI_FUNCTION(0x3, "uart3"),		/* RX */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D1 */
		SUNXI_FUNCTION(0x5, "twi2"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 10),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* TXCK / RMII_TXCK */
		SUNXI_FUNCTION(0x3, "uart3"),		/* RTS */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D2 */
		SUNXI_FUNCTION(0x5, "pwm1"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 11),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* TXCTL / RMII_TXEN */
		SUNXI_FUNCTION(0x3, "uart3"),		/* CTS */
		SUNXI_FUNCTION(0x4, "ts0"),	        /* D3 */
		SUNXI_FUNCTION(0x5, "pwm2"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 12),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),		/* CLKIN / RMII_NULL */
		SUNXI_FUNCTION(0x3, "uart4"),		/* TX */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D4 */
		SUNXI_FUNCTION(0x5, "pwm3"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 13),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),
		SUNXI_FUNCTION(0x3, "uart4"),		/* RX */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D5 */
		SUNXI_FUNCTION(0x5, "pwm4"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 14),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "gmac0"),
		SUNXI_FUNCTION(0x3, "uart4"),		/* RTS */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D6 */
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 15),
		SUNXI_FUNCTION(0x7, "io_disabled")),
	//Register Name: PI_CFG2
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ephy_25m"),
		SUNXI_FUNCTION(0x3, "uart4"),		/* CTS */
		SUNXI_FUNCTION(0x4, "ts0"),			/* D7 */
		SUNXI_FUNCTION(0x5, "x32kfout"),
		SUNXI_FUNCTION_IRQ_BANK(0x6, 7, 16),
		SUNXI_FUNCTION(0x7, "io_disabled")),
};

static const unsigned sun50iw9p1_irq_bank_base[] = {
	SUNXI_PIO_BANK_BASE(PA_BASE, 0),
	SUNXI_PIO_BANK_BASE(PC_BASE, 1),
	SUNXI_PIO_BANK_BASE(PD_BASE, 2),
	SUNXI_PIO_BANK_BASE(PE_BASE, 3),
	SUNXI_PIO_BANK_BASE(PF_BASE, 4),
	SUNXI_PIO_BANK_BASE(PG_BASE, 5),
	SUNXI_PIO_BANK_BASE(PH_BASE, 6),
	SUNXI_PIO_BANK_BASE(PI_BASE, 7),
};


static const unsigned sun50iw9p1_bank_base[] = {
	SUNXI_PIO_BANK_BASE(PA_BASE, 0),
	SUNXI_PIO_BANK_BASE(PC_BASE, 1),
	SUNXI_PIO_BANK_BASE(PD_BASE, 2),
	SUNXI_PIO_BANK_BASE(PE_BASE, 3),
	SUNXI_PIO_BANK_BASE(PF_BASE, 4),
	SUNXI_PIO_BANK_BASE(PG_BASE, 5),
	SUNXI_PIO_BANK_BASE(PH_BASE, 6),
	SUNXI_PIO_BANK_BASE(PI_BASE, 7),
};

static const struct sunxi_pinctrl_desc sun50iw9p1_pinctrl_data = {
	.pins = sun50iw9p1_pins,
	.npins = ARRAY_SIZE(sun50iw9p1_pins),
	.pin_base = 0,
	.banks = ARRAY_SIZE(sun50iw9p1_bank_base),
	.bank_base = sun50iw9p1_bank_base,
	.irq_banks = ARRAY_SIZE(sun50iw9p1_irq_bank_base),
	.irq_bank_base = sun50iw9p1_irq_bank_base,
};

static int sun50iw9p1_pinctrl_probe(struct platform_device *pdev)
{
	return sunxi_pinctrl_init(pdev, &sun50iw9p1_pinctrl_data);
}

static struct of_device_id sun50iw9p1_pinctrl_match[] = {
	{ .compatible = "allwinner,sun50iw9p1-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun50iw9p1_pinctrl_match);

static struct platform_driver sun50iw9p1_pinctrl_driver = {
	.probe	= sun50iw9p1_pinctrl_probe,
	.driver	= {
		.name		= "sun50iw9p1-pinctrl",
		.owner		= THIS_MODULE,
		.pm		= &sunxi_pinctrl_pm_ops,
		.of_match_table	= sun50iw9p1_pinctrl_match,
	},
};

static int __init sun50iw9p1_pio_init(void)
{
	int ret;
	ret = platform_driver_register(&sun50iw9p1_pinctrl_driver);
	if (ret) {
		pr_err("register sun50iw9p1 pio controller failed\n");
		return -EINVAL;
	}
	return 0;
}
postcore_initcall(sun50iw9p1_pio_init);

MODULE_AUTHOR("MartinWu<wuyan.martin@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner sun50iw9p1 pio pinctrl driver");
MODULE_LICENSE("GPL");
