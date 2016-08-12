/*************************************************************************
    > File Name: nand_flash.c
    > Author: ZHAOCHAO
    > Mail: 479680168@qq.com 
    > Created Time: Sun 31 Jul 2016 10:41:18 PM PDT
 ************************************************************************/

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_NAND) && !defined(CFG_NAND_LEGACY)
#include <s3c2410.h>
#include <nand.h>


DECLARE_GLOBAL_DATA_PTR;

#define S3C2440_NFSTAT_READY	(1<<0)
#define S3C2440_NFCONT_nFCE		(1<<1)




/* S3C2440: NAND FLASH CS */
static void s3c2440_nand_select_chip(struct mtd_info *mtd, int chip)
{
	S3C2440_NAND * const s3c2440nand = S3C2440_GetBase_NAND();

	if (chip == -1) {
		s3c2440nand->NFCONT |= S3C2440_NFCONT_nFCE;		// DISABLE CS 
	}
	else{
		s3c2440nand->NFCONT &= ~S3C2440_NFCONT_nFCE;	// EBABLE CS  
	}
}



/* S3C2440: COMMAND AND CONTROL FUNCTION */
static void s3c2440_nand_hwcontrol(struct mtd_info *mtd, int cmd)
{
	S3C2440_NAND * const s3c2440nand = S3C2440_GetBase_NAND();

	struct nand_chip *chip = mtd->priv;

	switch (cmd) {

		case NAND_CTL_SETNCE:
		case NAND_CTL_CLRNCE:
			printf("%s: called for NEC\n", __FUNCTION__);
			break;

		case NAND_CTL_SETCLE:
			chip->IO_ADDR_W = (void *)&s3c2440nand->NFCMD;
			break;

		case NAND_CTL_SETALE:
			chip->IO_ADDR_W = (void *)&s3c2440nand->NFADDR;
			break;

			/* NAND_CTL_CLRCLE: */
			/* NAND_CTL_CLRALE: */
		default:
			chip->IO_ADDR_W = (void *)&s3c2440nand->NFDATA;
			break;
	}
	
}



/* S3C2440: CHECK THE STATUS OF NAND */
/* return:  1=busy, 0=ready			 */
static int s3c2440_nand_devready(struct mtd_info *mtd)
{
	S3C2440_NAND * const s3c2440nand = S3C2440_GetBase_NAND();

	return (s3c2440nand->NFSTAT & S3C2440_NFSTAT_READY);
}


/* NAND HARDWARE INITIAL, ENABLE NAND CONTROLER */
static void s3c2440_nand_inithw(void)
{
	S3C2440_NAND * const s3c2440nand = S3C2440_GetBase_NAND();

	#define		TACLS		7
	#define		TWRPH0		7
	#define		TWRPH1		7

	/* FOR S3C2440   */
	s3c2440nand->NFCONF = (TACLS<<12) | (TWRPH0<<8) | (TWRPH1<<4) | (0<<0);
	/* INITIAL ECC, ENABLE NAND FLASH CONTROLER, ENABLE CS */
	s3c2440nand->NFCONT = (1<<4) | (0<<1) | (1<<0);
}



/* INITIAL NAND FLASH HARDWARE, INITIAL INTERFACE FUNCTION */
void board_nand_init(struct nand_chip *chip)
{
	S3C2440_NAND * const s3c2440nand = S3C2440_GetBase_NAND();

	s3c2440_nand_inithw();		/* INITIAL HARDWARE */

	chip->IO_ADDR_R = (void *)&s3c2440nand->NFDATA;
	chip->IO_ADDR_W	= (void *)&s3c2440nand->NFDATA;
	chip->hwcontrol = s3c2440_nand_hwcontrol;
	chip->dev_ready = s3c2440_nand_devready;
	chip->select_chip = s3c2440_nand_select_chip;
	chip->options = 0; /* Bit width = 8 */

	chip->eccmode = NAND_ECC_SOFT; /* THE CHECK MODE OF ECC:  SOFTWARE ECC */
} 
#endif
