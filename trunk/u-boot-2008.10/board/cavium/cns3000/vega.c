/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
/*
 * The RealView Emulation BaseBoard provides timers and soft reset
 * - the cpu code does not need to provide these.
 */
#include <common.h>
#include <i2c.h>

DECLARE_GLOBAL_DATA_PTR;

static ulong timestamp;
static ulong lastdec;

#define READ_TIMER (*(volatile ulong *)(CFG_TIMERBASE))

static void timer_init(void);

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
    printf("Boot reached stage %d\n", progress);
}
#endif

#define COMP_MODE_ENABLE ((unsigned int)0x0000EAEF)

static inline void delay (unsigned long loops)
{
	__asm__ volatile ("1:\n"
		"subs %0, %1, #1\n"
		"bne 1b":"=r" (loops):"0" (loops));
}

/*
 * Miscellaneous platform dependent initialisations
 */

int board_init (void)
{
	gd->bd->bi_arch_number = 2635;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x00000100;

	gd->flags = 0;

	icache_enable ();
	timer_init();
#ifdef CONFIG_AHCI_CNS3000
	//dram_init();
	scsi_init();
#endif
	return 0;
}


int misc_init_r (void)
{
	int i;
	uint8_t eeprom_enetaddr[6];
	uint8_t env_enetaddr[6];
	uint32_t serial = 0;
	uint8_t date[4];
	uint8_t model[16];

	char ethaddr[20];
  char *tmp = getenv("ethaddr");
  char *tmp1 = getenv("eth1addr");
  char *tmp2 = getenv("eth2addr");
  char *end;

	i2c_read(0x51, 0x30, 1, model, 16);
	i2c_read(0x51, 0x20, 1, date, 4);
	i2c_read(0x51, 0x18, 1, eeprom_enetaddr, 4);
	serial |= ((eeprom_enetaddr[0]) | (eeprom_enetaddr[1] << 8) |
						(eeprom_enetaddr[2] << 16) | (eeprom_enetaddr[3] << 24));


	printf("Gateworks Corporation Copyright 2010\n");
	printf("Model Number: %s\n", model);
	printf("Manufacturer Date: %02x-%02x-%02x%02x\n", date[0], date[1], date[2], date[3]);
	printf("Serial #: %i\n", serial);	

	i2c_read(0x51, 0x0, 1, eeprom_enetaddr, 6);

	for (i = 0; i < 6; i++) {
		env_enetaddr[i] = tmp ? simple_strtoul(tmp, &end, 16) : 0;
		if (tmp)
			tmp = (*end) ? end+1 : end;
	}

	if (!tmp) {
		sprintf(ethaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
			eeprom_enetaddr[0], eeprom_enetaddr[1],
			eeprom_enetaddr[2], eeprom_enetaddr[3],
			eeprom_enetaddr[4], eeprom_enetaddr[5]) ;	
		printf("### Setting environment from ROM MAC address = \"%s\"\n",
			ethaddr);
		setenv("ethaddr", ethaddr);
	}

	i2c_read(0x51, 0x6, 1, eeprom_enetaddr, 6);

	for (i = 0; i < 6; i++) {
		env_enetaddr[i] = tmp1 ? simple_strtoul(tmp1, &end, 16) : 0;
		if (tmp1)
			tmp1 = (*end) ? end+1 : end;
	}

	if (!tmp1) {
		sprintf(ethaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
			eeprom_enetaddr[0], eeprom_enetaddr[1],
			eeprom_enetaddr[2], eeprom_enetaddr[3],
			eeprom_enetaddr[4], eeprom_enetaddr[5]) ;	
		printf("### Setting environment from ROM MAC address = \"%s\"\n",
			ethaddr);
		setenv("eth1addr", ethaddr);
	}

	i2c_read(0x51, 0xc, 1, eeprom_enetaddr, 6);

	for (i = 0; i < 6; i++) {
		env_enetaddr[i] = tmp2 ? simple_strtoul(tmp2, &end, 16) : 0;
		if (tmp2)
			tmp2 = (*end) ? end+1 : end;
	}

	if (!tmp2) {
		sprintf(ethaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
			eeprom_enetaddr[0], eeprom_enetaddr[1],
			eeprom_enetaddr[2], eeprom_enetaddr[3],
			eeprom_enetaddr[4], eeprom_enetaddr[5]) ;	
		printf("### Setting environment from ROM MAC address = \"%s\"\n",
			ethaddr);
		setenv("eth2addr", ethaddr);
	}

	return (0);
}

/*************************************************************
 Routine:checkboard
 Description: Check Board Identity
*************************************************************/
int checkboard(void)
{
	return (0);
}

/******************************
 Routine:
 Description:
******************************/
int dram_init (void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
        gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

/*
 * Start the timer
 * U-Boot expects a 32 bit timer, running at CFG_HZ == 1000
 */
#include "cns3000.h"

#define PMU_REG_VALUE(addr) (*((volatile unsigned int *)(CNS3000_VEGA_PMU_BASE+addr)))
#define CLK_GATE_REG PMU_REG_VALUE(0) 
#define SOFT_RST_REG PMU_REG_VALUE(4) 
#define HS_REG PMU_REG_VALUE(8) 

static void timer_init(void)
{
	CLK_GATE_REG |= (1 << 14);
	SOFT_RST_REG &= (~(1 << 14));
	SOFT_RST_REG |= (1 << 14);
	//HS_REG |= (1 << 14);
	/*
	 * Now setup timer1
	 */	
	*(volatile ulong *)(CFG_TIMERBASE + 0x00) = CFG_TIMER_RELOAD;
	*(volatile ulong *)(CFG_TIMERBASE + 0x04) = CFG_TIMER_RELOAD;
	*(volatile ulong *)(CFG_TIMERBASE + 0x30) |= 0x0201;	/* Enabled,
								 * down counter,
								 * no interrupt,
								 * 32-bit,
								 */

	reset_timer_masked();
}

int interrupt_init (void){
	return 0;
}

/* delay x useconds AND perserve advance timstamp value */
/* ASSUMES timer is ticking at 1 msec			*/
void udelay (unsigned long usec)
{
	/* scott.patch */
#if 0
	delay(usec);
	return;
#endif

	ulong tmo, tmp;
#if 0
	tmo = usec/1000;
#else
	tmo = usec/10;
#endif

	tmp = get_timer (0);		/* get current timestamp */

	if( (tmo + tmp + 1) < tmp )	/* if setting this forward will roll time stamp */
		reset_timer_masked ();	/* reset "advancing" timestamp to 0, set lastdec value */
	else
		tmo += tmp;		/* else, set advancing stamp wake up time */

	while (get_timer_masked () < tmo)/* loop till event */
		/*NOP*/;
}

ulong get_timer (ulong base)
{
	return get_timer_masked () - base;
}

void reset_timer_masked (void)
{
	/* reset time */
	lastdec = READ_TIMER/1000;  /* capure current decrementer value time */
	timestamp = 0;	       	    /* start "advancing" time stamp from 0 */
}

/* ASSUMES 1MHz timer */
ulong get_timer_masked (void)
{
	ulong now = READ_TIMER/1000;	/* current tick value @ 1 tick per msec */

	//printf("now: %x\n", now);
	if (lastdec >= now) {		/* normal mode (non roll) */
		/* normal mode */
		timestamp += lastdec - now; /* move stamp forward with absolute diff ticks */
	} else {			/* we have overflow of the count down timer */
		/* nts = ts + ld + (TLV - now)
		 * ts=old stamp, ld=time that passed before passing through -1
		 * (TLV-now) amount of time after passing though -1
		 * nts = new "advancing time stamp"...it could also roll and cause problems.
		 */
		timestamp += lastdec + TIMER_LOAD_VAL - now;
	}
	lastdec = now;

	return timestamp;
}

/*
 *  u32 get_board_rev() for ARM supplied development boards
 */
ARM_SUPPLIED_GET_BOARD_REV

#if defined(CONFIG_CNS3000) && defined(CONFIG_FLASH_CFI_LEGACY)
ulong board_flash_get_legacy (ulong base, int banknum, flash_info_t *info)
{
	if (banknum == 0) {     /* non-CFI boot flash */
#if 0
		info->portwidth = FLASH_CFI_8BIT;
		info->chipwidth = FLASH_CFI_BY8;
		info->interface = FLASH_CFI_X8X16;
#endif
		info->portwidth = FLASH_CFI_16BIT;
		info->chipwidth = FLASH_CFI_BY16;
		info->interface = FLASH_CFI_X8X16;
		return 1;
	} else
		return 0;
}
#endif

