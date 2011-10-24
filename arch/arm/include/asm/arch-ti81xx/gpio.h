/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * This work is derived from the linux 2.6.27 kernel source
 * To fetch, use the kernel repository
 * git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux-2.6.git
 * Use the v2.6.27 tag.
 *
 * Below is the original's header including its copyright
 *
 *  linux/arch/arm/plat-omap/gpio.c
 *
 * Support functions for OMAP GPIO
 *
 * Copyright (C) 2003-2005 Nokia Corporation
 * Written by Juha Yrjölä <juha.yrjola@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2011-2011 Wuxi Xinje Corporation
 * Written by Luwei <sytu_xww@yahoo.com.cn>
 * Support funtion for TI81xx
 *
 */
#ifndef _GPIO_H
#define _GPIO_H

#define TI81XX_GPIO1_BASE  					0x48032000
#define TI81XX_GPIO2_BASE  					0x4804C000

#define TI81XX_GPIO_REVISION				0x0000
#define TI81XX_GPIO_SYSCONFIG				0x0010
#define TI81XX_GPIO_EOI							0x0020
#define TI81XX_GPIO_IRQSTATUS_RAW0  0x0024
#define TI81XX_GPIO_IRQSTATUS_RAW1	0x0028
#define TI81XX_GPIO_IRQSTATUS1			0x002C
#define TI81XX_GPIO_IRQSTATUS2			0x0030
#define TI81XX_GPIO_IRQSTATUS_SET0	0x0034
#define TI81XX_GPIO_IRQSTATUS_SET1	0x0038
#define TI81XX_GPIO_IRQSTATUS_CLR0	0x003C
#define TI81XX_GPIO_IRQSTATUS_CLR1	0x0040
#define TI81XX_GPIO_IRQWAKEN0				0x0044
#define TI81XX_GPIO_IRQWAKEN1				0x0048

#define TI81XX_GPIO_SYSSTATUS				0x0114
#define TI81XX_GPIO_CTRL						0x0130
#define TI81XX_GPIO_OE							0x0134
#define TI81XX_GPIO_DATAIN					0x0138
#define TI81XX_GPIO_DATAOUT					0x013c
#define TI81XX_GPIO_LEVELDETECT0		0x0140
#define TI81XX_GPIO_LEVELDETECT1		0x0144
#define TI81XX_GPIO_RISINGDETECT		0x0148
#define TI81XX_GPIO_FALLINGDETECT		0x014c
#define TI81XX_GPIO_DEBOUNCE_EN			0x0150
#define TI81XX_GPIO_DEBOUNCE_VAL		0x0154
#define TI81XX_GPIO_CLEARDATAOUT		0x0190
#define TI81XX_GPIO_SETDATAOUT			0x0194

struct gpio_bank {
	void *base;
	int method;
};

#define METHOD_GPIO_24XX	4
#define METHOD_GPIO_81XX  8

/* This is the interface */

/* Request a gpio before using it */
/* 使用gpio之前申请一个IO */
int omap_request_gpio(int gpio);
/* Reset and free a gpio after using it */
/* 使用玩之后释放IO */
void omap_free_gpio(int gpio);
/* Sets the gpio as input or output */
/* 设置GPIO输入或者输出模式 */
void omap_set_gpio_direction(int gpio, int is_input);
/* Set or clear a gpio output */
/* 设置或清除GPIO输出 */
void omap_set_gpio_dataout(int gpio, int enable);
/* Get the value of a gpio input */
/* 取得GPIO的输入值 */
int omap_get_gpio_datain(int gpio);

#endif /* _GPIO_H_ */
