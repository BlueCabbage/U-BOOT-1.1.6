/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
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

#include <common.h>
#include <s3c2410.h>

/////////////////////////////////////////////////////////
#include <video_fb.h>


DECLARE_GLOBAL_DATA_PTR;

#define FCLK_SPEED 1

#if FCLK_SPEED==0		/* Fout = 203MHz, Fin = 12MHz for Audio */
#define M_MDIV	0xC3
#define M_PDIV	0x4
#define M_SDIV	0x1
#elif FCLK_SPEED==1		/* Fout = 202.8MHz */
#define M_MDIV	0xA1
#define M_PDIV	0x3
#define M_SDIV	0x1
#endif

#define USB_CLOCK 1

#if USB_CLOCK==0
#define U_M_MDIV	0xA1
#define U_M_PDIV	0x3
#define U_M_SDIV	0x1
#elif USB_CLOCK==1
#define U_M_MDIV	0x48
#define U_M_PDIV	0x3
#define U_M_SDIV	0x2
#endif

/* s3c2440: MPLL = (2*m*Fin)/(p*2^s), UPLL = (m*Fin)/(p*2^s) */
/* m = M (the value for divider M) + 8, p = P (the value for divider P) + 2 */
#define S3C2440_MPLL_405MHZ			((0x7f<<12)|(0x02<<4)|(0x01))
#define S3C2440_UPLL_48MHZ			((0x38<<12)|(0x02<<4)|(0x02))
#define S3C2440_CLKDIV				0X05	/* FCLK:HCLK:PCLK = 1:2:8 */


/* s3c2410: MPLL = (m*Fin)/(p*2^s), UPLL = (m*Fin)/(p*2^s) */
/* m = M (the value for divider M) + 8, p = P (the value for divider P) + 2 */
#define S3C2410_MPLL_200MHZ			((0x5c<<12)|(0x04<<4)|(0x00))
#define S3C2410_UPLL_48MHZ			((0x28<<12)|(0x01<<4)|(0x02))
#define S3C2410_CLKDIV				0X03	/* FCLK:HCLK:PCLK = 1:2:4 */


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
	S3C24X0_CLOCK_POWER * const clk_power = S3C24X0_GetBase_CLOCK_POWER();
	S3C24X0_GPIO * const gpio = S3C24X0_GetBase_GPIO();

//	/* to reduce PLL lock time, adjust the LOCKTIME register */
//	clk_power->LOCKTIME = 0xFFFFFF;
//
//	/* configure MPLL */
//	clk_power->MPLLCON = ((M_MDIV << 12) + (M_PDIV << 4) + M_SDIV);
//
//	/* some delay between MPLL and UPLL */
//	delay (4000);
//
//	/* configure UPLL */
//	clk_power->UPLLCON = ((U_M_MDIV << 12) + (U_M_PDIV << 4) + U_M_SDIV);
//
//	/* some delay between MPLL and UPLL */
//	delay (8000);

	/* set up the I/O ports */
	gpio->GPACON = 0x007FFFFF;

#if defined(CONFIG_S3C2440)
	gpio->GPBCON = 0x00295551;
#else
	gpio->GPBCON = 0x00044556;
#endif

	gpio->GPBUP = 0x000007FF;
	gpio->GPCCON = 0xAAAAAAAA;
//	gpio->GPCUP = 0x0000FFFF;
	gpio->GPCUP = 0XFFFFFFFF;
	gpio->GPDCON = 0xAAAAAAAA;
//	gpio->GPDUP = 0x0000FFFF;
	gpio->GPDUP = 0XFFFFFFFF;
	gpio->GPECON = 0xAAAAAAAA;
	gpio->GPEUP = 0x0000FFFF;
	gpio->GPFCON = 0x000055AA;
	gpio->GPFUP = 0x000000FF;
	gpio->GPGCON = 0xFF95FFBA;
	gpio->GPGUP = 0x0000FFFF;
	gpio->GPHCON = 0x002AFAAA;
	gpio->GPHUP = 0x000007FF;

	gpio->GPBDAT = 0X00000000;

	/* Support both s3c2440 and s3c2410 */
	if (0 & ((gpio->GSTATUS1 == 0X32410000) || (gpio->GSTATUS1 == 0X32410002 ))) {
		/* FLCK : HCLK: PCLK = 1:2:4 */
		clk_power->CLKDIVN = S3C2410_CLKDIV;
		/* set cpu async... mode */
		__asm__( "mrc p15, 0, r1, c1, c0, 0\n" /* read ctrl register */
				 "orr r1, r1, #0xc0000000\n"   /* asynchronous */
				 "mcr p15, 0, r1, c1, c0, 0\n" /* write ctrl register */
				 :::"r1"
				);

		/* SET PLL LOCK TIME */
		clk_power->LOCKTIME = 0XFFFFFF;
		
		/* SET MPLL */
		clk_power->MPLLCON = S3C2410_MPLL_200MHZ;

		/* AFTER SET MPLL, DELAY SOME TIME TO SET UPLL */
		delay(4000);

		/* SET UPLL */
		clk_power->UPLLCON = S3C2410_UPLL_48MHZ;

		/* DELAY SOME TIME */
		delay(8000);

		/* MACHINE TYPE ID */
		gd->bd->bi_arch_number = MACH_TYPE_SMDK2410;
	}
	else {
		/* FLCK : HCLK: PCLK = 1:4:8 */
		clk_power->CLKDIVN = S3C2440_CLKDIV;
		/* set cpu async... mode */
		__asm__( "mrc p15, 0, r1, c1, c0, 0\n" /* read ctrl register */
				 "orr r1, r1, #0xc0000000\n"   /* asynchronous */
				 "mcr p15, 0, r1, c1, c0, 0\n" /* write ctrl register */
				 :::"r1"
				);

		/* SET PLL LOCK TIME */
		clk_power->LOCKTIME = 0XFFFFFF;
		
		/* SET MPLL */
		clk_power->MPLLCON = S3C2440_MPLL_405MHZ;

		/* AFTER SET MPLL, DELAY SOME TIME TO SET UPLL */
		delay(4000);

		/* SET UPLL */
		clk_power->UPLLCON = S3C2440_UPLL_48MHZ;

		/* DELAY SOME TIME */
		delay(8000);

		/* MACHINE TYPE ID */
		gd->bd->bi_arch_number = MACH_TYPE_S3C2440;

	}

//	/* arch number of SMDK2410-Board */
//	gd->bd->bi_arch_number = MACH_TYPE_SMDK2410;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = 0x30000100;

	icache_enable();
	dcache_enable();

	return 0;
}

int dram_init (void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

#ifdef CONFIG_CMD_NET
int board_eth_init(bd_t *bis)
{
	#ifdef CONFIG_CS8900
		rc = cs8900_initialize(0, CONFIG_CS8900_BASE);
	#endif
	#ifdef CONFIG_DRIVER_DM9000
		rc = dm9000_initialize(bis);
	#endif
		return rc;
}
#endif


