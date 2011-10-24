/*
 * Copyright (C) 2011, 无锡信捷电气有限公司
 *
 */

#include <common.h>
#include <command.h>
#include <asm/arch/gpio.h>


/*
 * do_menu
 * 
 * 说明：uboot命令。
 */
int do_gpio (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	  char gpio;
    if (argc < 2)
    	goto usage;
    simple_strtoul(argv[1], &gpio, 16);
    omap_request_gpio(gpio);
    omap_set_gpio_direction(gpio,0);
    if (strncmp(argv[2],"on",2) == 0)
    {
    	omap_set_gpio_dataout(gpio,1);
    }
    else if (strncmp(argv[2],"off",3) == 0)
    {
    	omap_set_gpio_dataout(gpio,0);
    }
    omap_free_gpio(gpio);
    return 0;
    
usage:
			cmd_usage(cmdtp);
			return -1;
}

U_BOOT_CMD(
 gpio,3,0,do_gpio,
 "gpio - set or clear gpio in board!",
 " - set    gpio num on\n"
 " - clear  gpio num off\n"
);


