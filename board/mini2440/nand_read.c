/*************************************************************************
    > File Name: nand_read.c
    > Author: ZHAOCHAO
    > Mail: 479680168@qq.com 
    > Created Time: Sat 06 Aug 2016 11:25:41 PM PDT
 ************************************************************************/

#include <config.h>

#include <common.h>
#include <linux/mtd/nand.h>


#define NFSTAT_BUSY 1
#define NF_BASE 0X4E000000			// the base addr of Nand Flash config register

#define __REGb(x)	(*(volatile unsigned char *)(x)) 
#define __REGi(x)	(*(volatile unsigned int *)(x))
#define __REGw(x)	(*(volatile unsigned short *)(x))

#define NFCONF		__REGi(NF_BASE + 0X00) 
#define NFCONT		__REGi(NF_BASE + 0X04)
#define NFCMD		__REGb(NF_BASE + 0X08)
#define NFADDR		__REGb(NF_BASE + 0X0C)
#define NFDATA		__REGb(NF_BASE + 0X10)
#define NFDATA16	__REGw(NF_BASE + 0X10)
#define NFSTAT		__REGb(NF_BASE + 0X20)


#define NAND_CHIP_ENABLE()	(NFCONT &= ~(1<<1))		// enable CS
#define NAND_CHIP_DISABLE()	(NFCONT |= (1<<1)) 
#define NAND_CLEAR_RB()		(NFSTAT |= BUSY)

#define nand_clear_RnB() 	(NFSTAT |= (1 << 2))
//#define NAND_DETECT_RB		{while(!(NFSTAT & BUSY));}


#define NAND_SECTOR_SIZE	2048
#define NAND_BLOCK_MASK		(NAND_SECTOR_SIZE - 1)



static inline void nand_wait(void)
{
	int i;
	while (!(NFSTAT & NFSTAT_BUSY))
		for (i = 0; i < 10; i ++);
}


struct boot_nand_t {
	int page_size;
	int block_size;
	int bad_block_offset;
	//unsigned long size;
};



static int is_bad_block(struct boot_nand_t * nand, unsigned long i)
{
	 unsigned char data;
	 unsigned long page_num;

	 nand_clear_RnB();

	 if (nand->page_size == 512) {

		  NFCMD = NAND_CMD_READOOB; /* 0x50 */

		  NFADDR = nand->bad_block_offset & 0xf;
		  NFADDR = (i >> 9) & 0xff;
		  NFADDR = (i >> 17) & 0xff;
		  NFADDR = (i >> 25) & 0xff;
	 } 

	 else if (nand->page_size == 2048) {
	  page_num = i >> 11; /* addr / 2048 */
	  NFCMD = NAND_CMD_READ0;
	  NFADDR = nand->bad_block_offset & 0xff;
	  NFADDR = (nand->bad_block_offset >> 8) & 0xff;
	  NFADDR = page_num & 0xff;
	  NFADDR = (page_num >> 8) & 0xff;
	  NFADDR = (page_num >> 16) & 0xff;
	  NFCMD = NAND_CMD_READSTART;
	 } else {
	  return -1;
	 }
	 nand_wait();
	 data = (NFDATA & 0xff);
	 if (data != 0xff)
	  return 1;
	 return 0;
}


static int nand_read_page_ll(struct boot_nand_t * nand, unsigned char *buf, unsigned long addr)
{
	 unsigned short *ptr16 = (unsigned short *)buf;
	 unsigned int i, page_num;

	 nand_clear_RnB();

	 NFCMD = NAND_CMD_READ0;

	 if ( nand->page_size == 512 ) {
	  /* Write Address */
		  NFADDR = addr & 0xff;
		  NFADDR = (addr >> 9) & 0xff;
		  NFADDR = (addr >> 17) & 0xff;
		  NFADDR = (addr >> 25) & 0xff;
	 } 

	 else if (nand->page_size == 2048) {
		  page_num = addr >> 11; /* addr / 2048 */
		  
		  /* Write Address */
		  NFADDR = 0;
		  NFADDR = 0;
		  NFADDR = page_num & 0xff;
		  NFADDR = (page_num >> 8) & 0xff;
		  NFADDR = (page_num >> 16) & 0xff;
		  NFCMD = NAND_CMD_READSTART;
	 } 

	 else {
	 	  return -1;
	 }

	 nand_wait();

	#if defined(CONFIG_S3C2410)

	 for (i = 0; i < nand->page_size; i++) {
		  *buf = (NFDATA & 0xff);
		  buf++;
	 }

	#elif defined(CONFIG_S3C2440) || defined(CONFIG_S3C2442)
	 for (i = 0; i < (nand->page_size>>1); i++) {
		  *ptr16 = NFDATA16;
		  ptr16++;
	 }

	#endif
	 return nand->page_size;
}


static unsigned short nand_read_id(void)
{
	 unsigned short res = 0;

	 NFCMD = NAND_CMD_READID;
	 NFADDR = 0;

	 res = NFDATA;
	 res = (res << 8) | NFDATA;
	 
	 return res;
}



extern unsigned int dynpart_size[];
/* low level nand read function */
int nand_read_ll(unsigned char *buf, unsigned long start_addr, int size)
{
	 int i, j;
	 unsigned short nand_id;
	 struct boot_nand_t nand;

	 /* chip Enable */
	 NAND_CHIP_ENABLE();

	 nand_clear_RnB();
	 
	 for (i = 0; i < 10; i++);

	 nand_id = nand_read_id();

	 if (0) { /* dirty little hack to detect if nand id is misread */
		  unsigned short * nid = (unsigned short *)0x31fffff0;
		  *nid = nand_id;
	 }

	 if (nand_id == 0xec76   /* Samsung K91208 */
	        ||nand_id == 0xad76 ) { /*Hynix HY27US08121A*/

		  nand.page_size = 512;
		  nand.block_size = 16 * 1024;
		  nand.bad_block_offset = 5;
		 // nand.size = 0x4000000;

	 } 

	 else if (nand_id == 0xecf1  /* Samsung K9F1G08U0B */
	     ||nand_id == 0xecda  /* Samsung K9F2G08U0B */
	     ||nand_id == 0xecd3 ) { /* Samsung K9K8G08 */

		  nand.page_size = 2048;
		  nand.block_size = 128 * 1024;
		  nand.bad_block_offset = nand.page_size;
		 // nand.size = 0x8000000;
	 } 

	 else {
	  	return -1; // hang
	 }


	 if ((start_addr & (nand.block_size-1)) || (size & ((nand.block_size-1))))
	  	return -1; /* invalid alignment */


	 for (i=start_addr; i < (start_addr + size);) {

		#ifdef CONFIG_S3C2410_NAND_SKIP_BAD

		 //if (i & (nand.block_size-1)== 0) {//warning: suggest parentheses around comparison in operand of '&'
		 if ((i & (nand.block_size-1))== 0) {   
			  if (is_bad_block(&nand, i) || is_bad_block(&nand, i + nand.page_size)) {
				    /* Bad block */
				    i += nand.block_size;
				    size += nand.block_size;

				    continue;
			   }
		  }

		#endif

	  j = nand_read_page_ll(&nand, buf, i);

	  i += j;
	  buf += j;

	 }

	 /* chip Disable */
	 NAND_CHIP_DISABLE();
	 return 0;
}

