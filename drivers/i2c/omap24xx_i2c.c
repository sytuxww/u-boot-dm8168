/*
 * Basic I2C functions
 *
 * Copyright (c) 2004 Texas Instruments
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Jian Zhang jzhang@ti.com, Texas Instruments
 *
 * Copyright (c) 2003 Wolfgang Denk, wd@denx.de
 * Rewritten to fit into the current U-Boot framework
 *
 * Adapted for OMAP2420 I2C, r-woodruff2@ti.com
 *
 */

#include <common.h>

#include <asm/arch/i2c.h>
#include <asm/io.h>

static void wait_for_bb (void);
static u16 wait_for_pin (void);
static void flush_fifo(void);

static struct i2c *i2c_base = (struct i2c *)I2C_DEFAULT_BASE;

static unsigned int bus_initialized[I2C_BUS_MAX];
static unsigned int current_bus;

/******************************************************************************
** 函数名	  :	 i2c_init
** 功能描述 :  初始化OMAP系列的设备的I2C模块，设置总线速度speed和从地址slaveadd。
** 输　入   : 
**　　    		int speed		 总线速度
**　　    		int slaveadd   设备从地址
** 输　出: 
**　　　  		无
** 全局变量:
**				bus_initialized   总线初始化标志
**				current_bus		目前使用的总线号
** 调用模块:
**				flush_fifo()		清空FIFO缓冲
** 作　者  :  邢伟伟 
** 日　期  :  2011年11月3日
**----------------------------------------------------------------------------
** 修改人:
** 日　期:
**----------------------------------------------------------------------------
******************************************************************************/
void i2c_init (int speed, int slaveadd)
{
	int psc, fsscll, fssclh;
	int hsscll = 0, hssclh = 0;
	u32 scll, sclh;

	/* Only handle standard, fast and high speeds */
	if ((speed != OMAP_I2C_STANDARD) &&
	    (speed != OMAP_I2C_FAST_MODE) &&
	    (speed != OMAP_I2C_HIGH_SPEED)) {
		printf("Error : I2C unsupported speed %d\n", speed);
		return;
	}

	psc = I2C_IP_CLK / I2C_INTERNAL_SAMPLING_CLK;
	psc -= 1;
	if (psc < I2C_PSC_MIN) {
		printf("Error : I2C unsupported prescalar %d\n", psc);
		return;
	}

	if (speed == OMAP_I2C_HIGH_SPEED) {
		/* High speed */

		/* For first phase of HS mode */
		fsscll = fssclh = I2C_INTERNAL_SAMPLING_CLK /
			(2 * OMAP_I2C_FAST_MODE);

		fsscll -= I2C_HIGHSPEED_PHASE_ONE_SCLL_TRIM;
		fssclh -= I2C_HIGHSPEED_PHASE_ONE_SCLH_TRIM;
		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printf("Error : I2C initializing first phase clock\n");
			return;
		}

		/* For second phase of HS mode */
		hsscll = hssclh = I2C_INTERNAL_SAMPLING_CLK / (2 * speed);

		hsscll -= I2C_HIGHSPEED_PHASE_TWO_SCLL_TRIM;
		hssclh -= I2C_HIGHSPEED_PHASE_TWO_SCLH_TRIM;
		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printf("Error : I2C initializing second phase clock\n");
			return;
		}

		scll = (unsigned int)hsscll << 8 | (unsigned int)fsscll;
		sclh = (unsigned int)hssclh << 8 | (unsigned int)fssclh;

	} else {
		/* Standard and fast speed */
		fsscll = fssclh = I2C_INTERNAL_SAMPLING_CLK / (2 * speed);

		fsscll -= I2C_FASTSPEED_SCLL_TRIM;
		fssclh -= I2C_FASTSPEED_SCLH_TRIM;
		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printf("Error : I2C initializing clock\n");
			return;
		}

		scll = (unsigned int)fsscll;
		sclh = (unsigned int)fssclh;
	}
	/* 设置总线初始化标志 */
	bus_initialized[current_bus] = 1;
	if (readw (I2C_CON) & I2C_CON_EN) {
		writew (0, I2C_CON);
		udelay (50000);
	}
	/* 设置预分频和SCL脉宽 */
	writew(psc, I2C_PSC);
	writew(scll, I2C_SCLL);
	writew(sclh, I2C_SCLH);

	/* own address */
	/* 设置自己的地址 */
	writew (slaveadd, I2C_OA);
	writew (I2C_CON_EN, I2C_CON);

	/* have to enable intrrupts or OMAP i2c module doesn't work */
	writew (I2C_IE_XRDY_IE | I2C_IE_RRDY_IE | I2C_IE_ARDY_IE |
		I2C_IE_NACK_IE | I2C_IE_AL_IE, I2C_IE);
	udelay (1000);
	flush_fifo();
	writew (0xFFFF, I2C_STAT);
	writew (0, I2C_CNT);
}

/******************************************************************************
** 函数名	  :	 i2c_read_byte
** 功能描述 :  从devaddr的设备中读取regoffset的值到value中。
** 输　入   : 
**　　    		u8 devaddr	 	设备地址
**　　    		u8 regoffset  寄存器地址
**				u8 *value		读取的值
** 输　出: 
**　　　  		1 读取错误
**				0 读取成功
** 全局变量:
**
** 调用模块:
**				flush_fifo()		清空FIFO缓冲
**				wait_for_bb()		等待到bus不忙
**				wait_for_pin()	
**
** 作　者  :  邢伟伟 
** 日　期  :  2011年11月3日
**----------------------------------------------------------------------------
** 修改人:
** 日　期:
**----------------------------------------------------------------------------
******************************************************************************/
static int i2c_read_byte (u8 devaddr, u8 regoffset, u8 * value)
{
	int i2c_error = 0;
	u16 status;

	/* wait until bus not busy */
	wait_for_bb ();

	/* one byte only */
	writew (1, I2C_CNT);
	/* set slave address */
	writew (devaddr, I2C_SA);
	/* no stop bit needed here */
	writew (I2C_CON_EN | I2C_CON_MST | I2C_CON_TRX | I2C_CON_STT, I2C_CON);
	status = wait_for_pin ();

	if (status & I2C_STAT_XRDY) {
		/* Important: have to use byte access */
		writew (regoffset, I2C_DATA);
		udelay (20000);
		if (readw (I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}

	if (!i2c_error) {
		/* free bus, otherwise we can't use a combined transction */
		writew (I2C_CON_EN, I2C_CON);
		while (readw (I2C_STAT) & ((I2C_STAT_RRDY) | (I2C_STAT_ARDY))) {
			udelay (10000);
			/* Have to clear pending interrupt to clear I2C_STAT */
			writew (0xFFFF, I2C_STAT);
		}

		/* set slave address */
		writew (devaddr, I2C_SA);
		/* read one byte from slave */
		writew (1, I2C_CNT);
		/* need stop bit here */
		writew (I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_STP,
			I2C_CON);

		status = wait_for_pin ();
		if (status & I2C_STAT_RRDY) {
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
			*value = readb (I2C_DATA);
#else
			*value = readw (I2C_DATA);
#endif
			udelay (20000);
		} else {
			i2c_error = 1;
		}

		if (!i2c_error) {
			writew (I2C_CON_EN, I2C_CON);
			while (readw (I2C_STAT)
			       || (readw (I2C_CON) & I2C_CON_MST)) {
				udelay (10000);
				writew (0xFFFF, I2C_STAT);
			}
		}
	}
	flush_fifo();
	writew (0xFFFF, I2C_STAT);
	writew (0, I2C_CNT);
	return i2c_error;
}

/******************************************************************************
** 函数名	  :	 i2c_write_byte
** 功能描述 :  向devaddr的设备的regoffset写入value值。
** 输　入   : 
**　　    		u8 devaddr	 	设备地址
**　　    		u8 regoffset  寄存器地址
**				u8 value		写入的值
** 输　出: 
**　　　  		1 写入错误
**				0 写入成功
** 全局变量:
**
** 调用模块:
**				flush_fifo()		清空FIFO缓冲
**				wait_for_bb()		等待到bus不忙
**				wait_for_pin()	
**
** 作　者  :  邢伟伟 
** 日　期  :  2011年11月3日
**----------------------------------------------------------------------------
** 修改人:
** 日　期:
**----------------------------------------------------------------------------
******************************************************************************/
static int i2c_write_byte (u8 devaddr, u8 regoffset, u8 value)
{
	int i2c_error = 0;
	u16 status, stat;

	/* wait until bus not busy */
	wait_for_bb ();

	/* two bytes */
	writew (2, I2C_CNT);
	/* set slave address */
	writew (devaddr, I2C_SA);
	/* stop bit needed here */
	writew (I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX |
		I2C_CON_STP, I2C_CON);

	/* wait until state change */
	status = wait_for_pin ();

	if (status & I2C_STAT_XRDY) {
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX) || defined(CONFIG_TI81XX)
		/* send out 1 byte */
		writeb (regoffset, I2C_DATA);
		writew (I2C_STAT_XRDY, I2C_STAT);

		status = wait_for_pin ();
		if ((status & I2C_STAT_XRDY)) {
			/* send out next 1 byte */
			writeb (value, I2C_DATA);
			writew (I2C_STAT_XRDY, I2C_STAT);
		} else {
			i2c_error = 1;
		}
#else
		/* send out two bytes */
		writew ((value << 8) + regoffset, I2C_DATA);
#endif
		/* must have enough delay to allow BB bit to go low */
		udelay (50000);
		if (readw (I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}

	if (!i2c_error) {
		int eout = 200;

		writew (I2C_CON_EN, I2C_CON);
		while ((stat = readw (I2C_STAT)) || (readw (I2C_CON) & I2C_CON_MST)) {
			udelay (1000);
			/* have to read to clear intrrupt */
			writew (0xFFFF, I2C_STAT);
			if(--eout == 0) /* better leave with error than hang */
				break;
		}
	}
	flush_fifo();
	writew (0xFFFF, I2C_STAT);
	writew (0, I2C_CNT);
	return i2c_error;
}

/******************************************************************************
** 函数名	  :	 flush_fifo
** 功能描述 :  清除设备的I2C模块的FIFO.
** 输　入   : 
**　　    		无
** 输　出: 
**　　　  		无
** 全局变量:
**
** 调用模块:
**				
**
** 作　者  :  邢伟伟 
** 日　期  :  2011年11月3日
**----------------------------------------------------------------------------
** 修改人:
** 日　期:
**----------------------------------------------------------------------------
******************************************************************************/
static void flush_fifo(void)
{	u16 stat;

	/* note: if you try and read data when its not there or ready
	 * you get a bus error
	 */
	while(1){
		stat = readw(I2C_STAT);
		if(stat == I2C_STAT_RRDY){
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX) || defined(CONFIG_TI81XX)
			readb(I2C_DATA);
#else
			readw(I2C_DATA);
#endif
			writew(I2C_STAT_RRDY,I2C_STAT);
			udelay(1000);
		}else
			break;
	}
}

/******************************************************************************
** 函数名	  :	 i2c_probe
** 功能描述 :  通过尝试读取一个字节来探测地址为chip的I2C设备。
** 输　入   : 
**　　    		uchar chip 芯片地址
** 输　出: 
**　　　  		1 探测失败
**				0 探测成功
** 全局变量:
**
** 调用模块:
**				flush_fifo()		清空FIFO缓冲
**				wait_for_bb()		等待到bus不忙
**				wait_for_pin()				
**
** 作　者  :  邢伟伟 
** 日　期  :  2011年11月3日
**----------------------------------------------------------------------------
** 修改人:
** 日　期:
**----------------------------------------------------------------------------
******************************************************************************/
int i2c_probe (uchar chip)
{
	int res = 1; /* default = fail */

	if (chip == readw (I2C_OA)) {
		return res;
	}

	/* wait until bus not busy */
	wait_for_bb ();

	/* try to read one byte */
	writew (1, I2C_CNT);
	/* set slave address */
	writew (chip, I2C_SA);
	/* stop bit needed here */
	writew (I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_STP, I2C_CON);
	/* enough delay for the NACK bit set */
	udelay (50000);

	if (!(readw (I2C_STAT) & I2C_STAT_NACK)) {
		res = 0;      /* success case */
		flush_fifo();
		writew(0xFFFF, I2C_STAT);
	} else {
		writew(0xFFFF, I2C_STAT);	 /* failue, clear sources*/
		writew (readw (I2C_CON) | I2C_CON_STP, I2C_CON); /* finish up xfer */
		udelay(20000);
		wait_for_bb ();
	}
	flush_fifo();
	writew (0, I2C_CNT); /* don't allow any more data in...we don't want it.*/
	writew(0xFFFF, I2C_STAT);
	return res;
}

/******************************************************************************
** 函数名	  :	 i2c_read
** 功能描述 :  通过i2c_read_byte()读取地址addr的设备一定长度的缓冲区域。
** 输　入   : 
**　　    		uchar chip 芯片地址
**				uint  addr 芯片内部地址
**				int   alen 地址长度
**				uchar *buffer 读取的缓冲区
**				int   len  读取的长度
**
** 输　出: 
**　　　  		1 读取失败
**				0 读取成功
** 全局变量:
**
** 调用模块:
**				i2c_read_byte()		读取一个字节
**				i2c_init()				初始化
**
** 作　者  :  邢伟伟 
** 日　期  :  2011年11月3日
**----------------------------------------------------------------------------
** 修改人:
** 日　期:
**----------------------------------------------------------------------------
******************************************************************************/
int i2c_read (uchar chip, uint addr, int alen, uchar * buffer, int len)
{
	int i;

	if (alen > 1) {
		printf ("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (addr + len > 256) {
		printf ("I2C read: address out of range\n");
		return 1;
	}

	for (i = 0; i < len; i++) {
		if (i2c_read_byte (chip, addr + i, &buffer[i])) {
			printf ("I2C read: I/O error\n");
			i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
			return 1;
		}
	}

	return 0;
}

/******************************************************************************
** 函数名	  :	 i2c_write
** 功能描述 :  通过i2c_write_byte()写入地址addr的设备一定长度的缓冲区域。
** 输　入   : 
**　　    		uchar chip 芯片地址
**				uint  addr 芯片内部地址
**				uchar *buffer 读取的缓冲区
**				int   len  读取的长度
**
** 输　出: 
**　　　  		1 读取失败
**				0 读取成功
** 全局变量:
**
** 调用模块:
**				i2c_write_byte()		写入一个字节
**				i2c_init()				初始化
**
** 作　者  :  邢伟伟 
** 日　期  :  2011年11月3日
**----------------------------------------------------------------------------
** 修改人:
** 日　期:
**----------------------------------------------------------------------------
******************************************************************************/
int i2c_write (uchar chip, uint addr, int alen, uchar * buffer, int len)
{
	int i;

	if (alen > 1) {
		printf ("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (addr + len > 256) {
		printf ("I2C read: address out of range\n");
		return 1;
	}

	for (i = 0; i < len; i++) {
		if (i2c_write_byte (chip, addr + i, buffer[i])) {
			printf ("I2C read: I/O error\n");
			i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
			return 1;
		}
	}

	return 0;
}

/******************************************************************************
** 函数名	  :	 wait_for_bb
** 功能描述 :  等待I2C总线空闲。
** 输　入   : 
**　　    		无
**
** 输　出: 
**　　　  		无
** 全局变量:
**
** 调用模块:
**				
**
** 作　者  :  邢伟伟 
** 日　期  :  2011年11月3日
**----------------------------------------------------------------------------
** 修改人:
** 日　期:
**----------------------------------------------------------------------------
******************************************************************************/
static void wait_for_bb (void)
{
	int timeout = 10;
	u16 stat;

	writew(0xFFFF, I2C_STAT);	 /* clear current interruts...*/
	while ((stat = readw (I2C_STAT) & I2C_STAT_BB) && timeout--) {
		writew (stat, I2C_STAT);
		udelay (50000);
	}

	if (timeout <= 0) {
		printf ("timed out in wait_for_bb: I2C_STAT=%x\n",
			readw (I2C_STAT));
	}
	writew(0xFFFF, I2C_STAT);	 /* clear delayed stuff*/
}

/******************************************************************************
** 函数名	  :	 wait_for_pin
** 功能描述 :  等待I2C总线空闲。
** 输　入   : 
**　　    		无
**
** 输　出: 
**　　　  		无
** 全局变量:
**
** 调用模块:
**				
**
** 作　者  :  邢伟伟 
** 日　期  :  2011年11月3日
**----------------------------------------------------------------------------
** 修改人:
** 日　期:
**----------------------------------------------------------------------------
******************************************************************************/
static u16 wait_for_pin (void)
{
	u16 status;
	int timeout = 10;

	do {
		udelay (1000);
		status = readw (I2C_STAT);
       } while (  !(status &
                   (I2C_STAT_ROVR | I2C_STAT_XUDF | I2C_STAT_XRDY |
                    I2C_STAT_RRDY | I2C_STAT_ARDY | I2C_STAT_NACK |
                    I2C_STAT_AL)) && timeout--);

	if (timeout <= 0) {
		printf ("timed out in wait_for_pin: I2C_STAT=%x\n",
			readw (I2C_STAT));
			writew(0xFFFF, I2C_STAT);
}
	return status;
}

/******************************************************************************
** 函数名	  :	 i2c_set_bus_num
** 功能描述 :  设置总线号，并更新寄存器地址
** 输　入   : 
**　　    		int bus 总线号
**
** 输　出: 
**　　　  		无
** 全局变量:
**				current_bus 		目前使用总线号
**				bus_initialized	初始化标志
** 调用模块:
**				i2c_init()			初始化总线
**
** 作　者  :  邢伟伟 
** 日　期  :  2011年11月3日
**----------------------------------------------------------------------------
** 修改人:
** 日　期:
**----------------------------------------------------------------------------
******************************************************************************/
int i2c_set_bus_num(unsigned int bus)
{
	if ((bus < 0) || (bus >= I2C_BUS_MAX)) {
		printf("Bad bus: %d\n", bus);
		return -1;
	}

#if I2C_BUS_MAX==3
	if (bus == 2)
		i2c_base = (struct i2c *)I2C_BASE3;
	else
#endif
	if (bus == 1)
		i2c_base = (struct i2c *)I2C_BASE2;
	else
		i2c_base = (struct i2c *)I2C_BASE1;

	current_bus = bus;

	if(!bus_initialized[current_bus])
		i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	return 0;
}
