/*
 * Copyright (C) 2011, 无锡信捷电气有限公司
 *
 */

#include <common.h>
#include <command.h>
#include <asm/arch/gpio.h>

void gpio_on_off(int gpio,char is_on)
{
	omap_request_gpio(gpio);
	omap_set_gpio_direction(gpio,0);
	
	if(is_on)
		omap_set_gpio_dataout(gpio,1); 
	else
		omap_set_gpio_dataout(gpio,0);
	
	omap_free_gpio(gpio);
	
}
/*
 * do_gpio
 * 
 * 说明：uboot命令。
 */
int do_gpio (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	  int gpio;
    if (argc < 3)
    	goto usage;
    	
    gpio = simple_strtoul(argv[1], NULL , 10);
    printf("GPIO Number is %d!\n",gpio);

    if (strncmp(argv[2],"on",2) == 0)
    {
    	printf("GPIO %d on!\n",gpio);
      gpio_on_off(gpio,1); 
    }
    else if (strncmp(argv[2],"off",3) == 0)
    {
    	printf("GPIO %d off!\n",gpio);
      gpio_on_off(gpio,0);
    }
    else
    	goto usage;
    return 0;
    
usage:
			cmd_usage(cmdtp);
			return -1;
}

U_BOOT_CMD(
 gpio,3,1,do_gpio,
 "gpio - set or clear gpio in board!",
 "[num on|off] \n"
 " 	- set    gpio num on\n"
 " 	- clear  gpio num off\n"
);


