/*
 * (C) Copyright 2011 无锡信捷电气有限公司
 *		luwei <sytu_xww@yahoo.com.cn>
 *
 * (C) Copyright 2010 Texas Instruments, <www.ti.com>
 *
 * Author :
 *     Mansoor Ahamed <mansoor.ahamed@ti.com>
 *
 * Initial Code from:
 *     Manikandan Pillai <mani.pillai@ti.com>
 *     Richard Woodruff <r-woodruff2@ti.com>
 *     Syed Mohammed Khasim <khasim@ti.com>
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
 */

// 0  2011年10月12日  添加了三星和美光的两种NAND的选择，两者差异为美光为16位数据，而
//					  三星为8位数据线，且两者的时序选择不一样。
// 1  2011年10月21日  修改三星和美光的配置，并添加enable_gpmc_cs_config_type(),给nand_base.c
//						提供API，用于检测NAND的生产ID并根据ID选择不同的时序配置，以支持多个NAND。
#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/mem.h>
#include <asm/arch/sys_proto.h>
#include <command.h>
#include <linux/mtd/nand.h>

/*
 * Only One NAND allowed on board at a time.
 * The GPMC CS Base for the same
 */
unsigned int boot_flash_base;
unsigned int boot_flash_off;
unsigned int boot_flash_sec;
unsigned int boot_flash_type;
volatile unsigned int boot_flash_env_addr;

struct gpmc *gpmc_cfg;

#if defined(CONFIG_CMD_NAND)

#if defined(CONFIG_NAND_MICRON)||(CONFIG_NAND_AUTO)
static const u32 gpmc_m_nand_micron[GPMC_MAX_REG] = {
	M_NAND_GPMC_CONFIG1,
	M_NAND_GPMC_CONFIG2,
	M_NAND_GPMC_CONFIG3,
	M_NAND_GPMC_CONFIG4,
	M_NAND_GPMC_CONFIG5,
	M_NAND_GPMC_CONFIG6, 0
};
#endif

#if defined(CONFIG_NAND_SAMSUNG)||(CONFIG_NAND_AUTO)
static const u32 gpmc_m_nand_samsung[GPMC_MAX_REG] = {
	SMNAND_GPMC_CONFIG1,
	SMNAND_GPMC_CONFIG2,
	SMNAND_GPMC_CONFIG3,
	SMNAND_GPMC_CONFIG4,
	SMNAND_GPMC_CONFIG5,
	SMNAND_GPMC_CONFIG6, 0
};
#endif
#define GPMC_CS 0
#endif

#if defined(CONFIG_NAND_AUTO)
/* 
 * 函数: enable_gpmc_cs_config_type
 *
 * 参数: 
 *		const u32 nand_type  NAND厂家
 *
 * 返回值: 无
 *
 * 描述:
 * 		根据NAND厂家型号选择不同的配置参数重新配置GPMC。
 */
void enable_gpmc_cs_config_type(const u32 nand_maf_id)
{
	const u32 *gpmc_config = NULL;
	u32 base = 0;
	u32 size = 0;

	gpmc_config = gpmc_m_nand_micron;

	base = PISMO1_NAND_BASE;
	size = PISMO1_NAND_SIZE;
	
	switch(nand_maf_id)
	{
		case NAND_MFR_SAMSUNG:
		gpmc_config = gpmc_m_nand_samsung;
		break;
		case NAND_MFR_MICRON:
		gpmc_config = gpmc_m_nand_micron;
		break;
		default:
		break;
	}
	enable_gpmc_cs_config(gpmc_config, &gpmc_cfg->cs[0], base, size);
}
#endif
/* 
 * 函数: enable_gpmc_cs_config
 *
 * 参数: 
 *		const u32 *gpmc_config  GPMC参数设置
 *		struct gpmc_cs *cs			CPU寄存器
 *		u32 base								基地址
 *		u32 size								大小
 *
 * 返回值: 无
 *
 * 描述:
 * 		根据gpmc_config的参数设置，写入GPMC对应的cs寄存器，并设置NAND基地址和大小。
 */
void enable_gpmc_cs_config(const u32 *gpmc_config, struct gpmc_cs *cs, u32 base,
			u32 size)
{
	writel(0, &cs->config7);
	sdelay(1000);
	/* Delay for settling */
	writel(gpmc_config[0], &cs->config1);
	writel(gpmc_config[1], &cs->config2);
	writel(gpmc_config[2], &cs->config3);
	writel(gpmc_config[3], &cs->config4);
	writel(gpmc_config[4], &cs->config5);
	writel(gpmc_config[5], &cs->config6);
	/* Enable the config */
	//设置NAND基地址和大小，并使能
	writel((((size & 0xF) << 8) | ((base >> 24) & 0x3F) |
		(1 << 6)), &cs->config7);
	sdelay(2000);
}

/* 
 * 函数: gpmc_init
 *
 * 参数: 
 *		无
 *
 * 返回值: 无
 *
 * 描述:
 * 		设置GPMC
 *		Init GPMC for x16, MuxMode (SDRAM in x32).
 * 		This code can only be executed from SRAM or SDRAM.
 * 		初始化GPMC总线，x16总线
 *
 * TODO 修改为x8 for K9F2G08U0B 
 */
void gpmc_init(void)
{
	/* putting a blanket check on GPMC based on ZeBu for now */
	/* GPMC_BASE = 0x50000000 gpmc的寄存器地址 */
	gpmc_cfg = (struct gpmc *)GPMC_BASE;

#ifdef CONFIG_NOR_BOOT
	/* env setup */
	boot_flash_base = CONFIG_SYS_FLASH_BASE;
	boot_flash_off = CONFIG_ENV_OFFSET;
	boot_flash_sec = NOR_SECT_SIZE;
	boot_flash_env_addr = boot_flash_base + boot_flash_off;
#else
#if defined(CONFIG_CMD_NAND) || defined(CONFIG_CMD_ONENAND)
	const u32 *gpmc_config = NULL;
	u32 base = 0;
	u32 size = 0;
#if defined(CONFIG_ENV_IS_IN_NAND) || defined(CONFIG_ENV_IS_IN_ONENAND)
	u32 f_off = CONFIG_SYS_MONITOR_LEN;
	u32 f_sec = 0;
#endif
#endif
	//-------------------------------------------------------------------//
	//
	//					全局设置 
	//
	//-------------------------------------------------------------------//
	/* SIDLEMODE = 0x1 No-idle. An idle request is never acknowledged */
	writel(0x00000008, &gpmc_cfg->sysconfig);
	/* WAIT0EDGEDETECTIONSTATUS = 1 复位 */
	writel(0x00000100, &gpmc_cfg->irqstatus);
	/* WAIT1EDGEDETECTIONENABLE = 1 使能边沿触发中断wait1 */
	writel(0x00000200, &gpmc_cfg->irqenable);
	/* 
	 * LIMITEDADDRESS = 1 限制地址使能 
	 * WRITEPROTECT = 1   WP管脚为高
	 */
	writel(0x00000012, &gpmc_cfg->config);
	/*
	 * Disable the GPMC0 config set by ROM code
	 */
	writel(0, &gpmc_cfg->cs[0].config7);
	sdelay(1000);

	//-------------------------------------------------------------------//
	//
	//					读写时序设置 
	//
	//-------------------------------------------------------------------//
#if defined(CONFIG_CMD_NAND)	/* CS 0 */
	//gpmc_config = gpmc_m_nand;
	/* 默认为美光的设置 */

#if defined(CONFIG_NAND_MICRON)
	gpmc_config = gpmc_m_nand_micron;
#endif

#if defined(CONFIG_NAND_SAMSUNG)
	gpmc_config = gpmc_m_nand_samsung;
#endif

	base = PISMO1_NAND_BASE;
	size = PISMO1_NAND_SIZE;
	enable_gpmc_cs_config(gpmc_config, &gpmc_cfg->cs[0], base, size);
#if defined(CONFIG_ENV_IS_IN_NAND)
	f_off = MNAND_ENV_OFFSET;
	f_sec = (128 << 10);	/* 128 KiB */
	/* env setup */
	boot_flash_base = base;
	boot_flash_off = f_off;
	boot_flash_sec = f_sec;
	boot_flash_env_addr = f_off;
#endif
#endif

#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
	/* env setup */
	boot_flash_base = 0x0;
	boot_flash_off = CONFIG_ENV_OFFSET;
	boot_flash_sec = CONFIG_ENV_SECT_SIZE;
	boot_flash_env_addr = CONFIG_ENV_OFFSET;
#endif

#endif
}



