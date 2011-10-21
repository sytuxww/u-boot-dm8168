/*
 * Copyright (C) 2011, 无锡信捷电气有限公司
 *
 * 这是uboot中运行的一个menu命令，完成系统的一键更新。
 * 本菜单需要TFTP服务器支持。
 *
 * 1 从TFTP服务器下载u-boot.noxip.bin到Flash中
 * 2 从TFTP服务器下载uImage镜像到Flash中
 * 3 从TFTP服务器下载文件系统更新至Flash中
 * 4 从NAND启动Linux，NFS挂载文件系统
 * 5 从TFTP启动Linux，NFS挂载文件系统
 * 6 从NAND启动Linxu，NAND挂载文件系统
 * 7 从TFTP启动Linux，NAND挂载文件系统
 * 8 从TFTP服务器下载Android镜像更新至Flash
 * r 重新启动Uboot
 * q 退出菜单，回到正常uboot菜单
 */

#include <common.h>
#include <command.h>
#include <nand.h>

extern char console_buffer[];
extern int readline (const char *const prompt);
/*
 * static char awaitkey(unsigned long delay, int* error_p)
 * 
 * 说明：等待终端输入一个按键，并根据按键选择执行菜单。
 *
 *
 */
static char awaitkey(unsigned long delay, int* error_p)
{
     int i;
     char c;
     if (delay == -1) {
         while (1) {
             if (tstc()) /* we got a key press */
                return getc();
         }
     }
     else {       
          for (i = 0; i < delay; i++) {
       if (tstc()) /* we got a key press */
        return getc();
             udelay (10*1000);
         }
     }
     if (error_p)
         *error_p = -1;
     return 0;
 } 
/**
 * Parses a string into a number.  The number stored at ptr is
 * potentially suffixed with K (for kilobytes, or 1024 bytes),
 * M (for megabytes, or 1048576 bytes), or G (for gigabytes, or
 * 1073741824).  If the number is suffixed with K, M, or G, then
 * the return value is the number multiplied by one kilobyte, one
 * megabyte, or one gigabyte, respectively.
 *
 * @param ptr where parse begins
 * @param retptr output pointer to next char after parse completes (output)
 * @return resulting unsigned int
 */
static unsigned long memsize_parse2 (const char *const ptr, const char **retptr)
{
	unsigned long ret = simple_strtoul(ptr, (char **)retptr, 0);
    int sixteen = 1;

	switch (**retptr) {
		case 'G':
		case 'g':
			ret <<= 10;
		case 'M':
		case 'm':
			ret <<= 10;
		case 'K':
		case 'k':
			ret <<= 10;
			(*retptr)++;
            sixteen = 0;
		default:
			break;
	}

    if (sixteen)
        return simple_strtoul(ptr, NULL, 16);
    
	return ret;
}


/*
 * void main_menu_usage(void)
 * 
 * 说明：显示主菜单。
 */
void main_menu_usage(void)
{
    printf("\r\n######################################################\r\n");
    printf(    "#####                 无锡信捷电气有限公司       #####\r\n");
    printf(    "#####              AM389x/AM387 bootloader       #####\r\n");
    printf(    "#####                  by xww                    #####\r\n");
    printf(    "######################################################\r\n");
    printf("\r\n[1] 更新uboot              0x00000000\r\n");
    printf("\r\n#####                    linux部分                #####\r\n");
    printf("\r\n[2] 更新Linux内核镜像      0x00280000\r\n");
    printf(    "[3] 更新Linux文件系统      0x006C0000\r\n");
    printf(    "[4] 从NAND启动Linux,   文件系统--NFS\r\n");
    printf(    "[5] 从NET 启动Linux，  文件系统--NFS\r\n");
    printf(    "[6] 从NAND启动Linux,   文件系统--NAND\r\n");
    printf(    "[7] 从NET 启动Linux，  文件系统--NAND\r\n");
    printf("\r\n#####                    android                   #####\r\n");
    printf("\r\n[8] 更新Android内核镜像      0x00280000\r\n");
    printf(    "[9] 更新Android文件系统      0x006C0000\r\n");
    printf(    "[a] 启动Android系统，  文件系统为SD中\r\n");
    printf(    "[b] 启动Android系统，  文件系统为nand中\r\n");
    printf("\r\n[r] 重新启动u-boot\r\n");
    printf(    "[q] 跳出菜单\r\n");
    printf(    "输入你的选择: ");
}

/*
 * void menu_shell(void)
 * 
 * 说明：菜单循环，根据输入执行不同菜单功能。
 */
void menu_shell(void)
{
    char c;
    char cmd_buf[500];
    while (1)
    {
        main_menu_usage();
        c = awaitkey(-1, NULL);
        printf("%c\n", c);
        switch (c)
        {
	    case '1'://Download u-boot to Nand Flash
	    {
		printf("从TFTP服务器下载uboot.noxip.bin到NAND 0x0～0x260000...\n");
		printf("0x00000000～0x00260000-------->u-boot.bin\n");
		printf("0x00260000～0x00440000-------->env\n");
		printf("0x00440000～0x006C0000-------->uImage-linux\n");
		printf("0x006C0000～0x0D000000-------->linux_jffs2.bin\n");
                strcpy(cmd_buf, 
		"mw.b 0x81000000 0xFF 0x260000;\
		tftp 0x81000000 u-boot.noxip.bin;\
		nand erase 0x0 0x260000;\
		nandecc hw 2;\
		nand write.i 0x81000000 0x0 0x260000;\
		nandecc hw 0");
                run_command(cmd_buf, 0);
                break;
	    }
            case '2'://Download Linux kernel uImage
            {
		printf("从TFTP服务器下载linux镜像uImage-linux到NAND 0x440000～0x260000...\n");
		printf("0x00000000～0x00260000-------->u-boot.bin\n");
		printf("0x00260000～0x00440000-------->env\n");
		printf("0x00440000～0x006C0000-------->uImage-linux\n");
		printf("0x006C0000～0x0D000000-------->linux_jffs2.bin\n");
                strcpy(cmd_buf, 
		"mw.b 0x81000000 0xFF 0x440000;\
		tftp 0x81000000 uImage-linux;\
		nand erase 0x00280000 0x440000;\
		nandecc hw 2;\
		nand write 0x81000000 0x280000 0x440000;\
		nandecc hw 0");
               run_command(cmd_buf, 0);
		break;
            }
            
            case '3'://Download root_jffs2 image
            {
		printf("从TFTP服务器下载linux文件系统镜像linux_jffs2.bin到NAND 0x6C0000～0xD000000...\n");
		printf("0x00000000～0x00260000-------->u-boot.bin\n");
		printf("0x00260000～0x00440000-------->env\n");
		printf("0x00440000～0x006C0000-------->uImage_linux.bin\n");
		printf("0x006C0000～0x0D000000-------->linux_jffs2.bin\n");
                strcpy(cmd_buf,
		"mw.b 0x81000000 0xFF 0x0C820000;\
		tftp 0x81000000 linux_jffs2.bin;\
		nand erase 0x006C0000 0x0C820000;\
		nandecc hw 2;\
		nand write 0x81000000 0x006C0000 0x0C820000;\
		nandecc hw 0");
                run_command(cmd_buf, 0);
                break;
            }

            case '4'://nfs
            {
		printf("从NAND读取linux镜像并从NFS服务器加载文件系统...\n");
                strcpy(cmd_buf,
		"dhcp;\
		run addip;\
		nandecc hw 2;\
		nand read 0x81000000 0x280000 0x440000;\
		nandecc hw 0;\
		bootm 0x81000000");
                run_command(cmd_buf, 0);
                break;
            }
            case '5'://nfs
            {
		printf("从TFTP读取linux镜像uImage-linux并从NFS服务器加载文件系统...\n");
                strcpy(cmd_buf, 
		"dhcp;\
		run addip;\
		tftp 81000000 uImage-linux;\
		bootm");
                run_command(cmd_buf, 0);
                break;
            }
            case '6'://从nand启动系统，nand挂载文件系统
            {
                printf("从NAND读取linux镜像并从NAND加载文件系统...\n");
                strcpy(cmd_buf, 
		"dhcp;\
		run addip;\
		nandecc hw 2;\
		nand read 0x81000000 0x280000 0x440000;\
		nandecc hw 0;\
		setenv bootargs 'mem=128M console=ttyO2,115200n8 noinitrd root=/dev/mtdblock7 rw rootfstype=jffs2 ip=off';\
		bootm 0x81000000");
                run_command(cmd_buf, 0);
                break;
            }
            case '7'://从网络启动系统，nand挂载文件系统
            {
                printf("从TFTP读取linux内核镜像uImage-linux并从NAND加载文件系统...\n");
                strcpy(cmd_buf, 
		"dhcp;\
		run addip;\
		tftp 81000000 uImage-linux;\
		setenv bootargs 'mem=128M console=ttyO2,115200n8 noinitrd root=/dev/mtdblock7 rw rootfstype=jffs2 ip=off';\
		bootm 0x81000000");
                run_command(cmd_buf, 0);
                break;
            }
            case '8'://更新android镜像到nand
            {
                printf("从TFTP读取Android内核镜像uImage-Android并更新至NAND 0x440000～0x260000...\n");
		printf("0x00000000～0x00260000-------->u-boot\n");
		printf("0x00260000～0x00440000-------->env\n");
		printf("0x00440000～0x006C0000-------->android\n");
		printf("0x006C0000～0x0D000000-------->fs\n");
                strcpy(cmd_buf, 
		"dhcp;\
		run addip;\
		tftp 81000000 uImage-Android;\
		nand erase 0x00280000 0x00440000;\
		nand write 0x81000000 0x00280000 0x00440000;\
		bootm 0x81000000");
                run_command(cmd_buf, 0);
                break;
            }
            case '9'://更新android文件系统镜像到nand
            {
                printf("从TFTP读取Android文件系统镜像android_jffs2.bin并更新至NAND 0x006C0000～0x0D000000...\n");
		printf("0x00000000～0x00260000-------->u-boot.bin\n");
		printf("0x00260000～0x00440000-------->env\n");
		printf("0x00440000～0x006C0000-------->uImage_Android.bin\n");
		printf("0x006C0000～0x0D000000-------->android_jffs2.bin\n");
                strcpy(cmd_buf, 
		"dhcp;\
		run addip;\
		tftp 81000000 android_jffs2.bin;\
		nand erase 0x006C0000 0x0C820000;\
		nandecc hw 2;\
		nand write 0x81000000 0x006C0000 0x0C820000;\
		nandecc hw 0");
                run_command(cmd_buf, 0);
                break;
            }
            case 'a'://启动android系统
            {
                printf("从TFTP读取Android镜像并以SD卡为文件系统启动...\n");
                strcpy(cmd_buf, 
		"dhcp;\
		run addip;\
		tftp 81000000 uImage-Android;\
		setenv bootargs 'mem=166M@0x80000000 mem=768M@0x90000000 console=ttyO2,115200n8 androidboot.console=ttyO2  root=/dev/mmcblk0p2 rw rootfstype=ext3 rootdelay=1 init=/init ip=off';\
		bootm 0x81000000");
                run_command(cmd_buf, 0);
                break;
            }
            case 'b'://启动android系统
            {
                printf("从TFTP读取Android镜像并以nand为文件系统启动...\n");
                strcpy(cmd_buf, 
		"dhcp;\
		run addip;\
		tftp 81000000 uImage-Android;\
		setenv bootargs 'mem=166M@0x80000000 mem=768M@0x90000000 console=ttyO2,115200n8 androidboot.console=ttyO2  root=/dev/mtdblock7 rw rootfstype=jffs2 rootdelay=1 init=/init ip=off';\
		run addip;\
		bootm 0x81000000");
                run_command(cmd_buf, 0);
                break;
            }
            case 'r':
            {
		strcpy(cmd_buf, "reset");
		run_command(cmd_buf, 0);
                break;
            }
            
            case 'q':
            {
                return;    
                break;
            }
	default:
	break;

        }
                
    }
}
/*
 * do_menu
 * 
 * 说明：uboot命令。
 */
int do_menu (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    menu_shell();
    return 0;
}
U_BOOT_CMD(
 menu,3,0,do_menu,
 "menu - display a menu, to select the items to do something",
 " - display a menu, to select the items to do something"
);


