#ifndef _NAND_H_
#define _NAND_H_

#include "types.h"

#define NAND_SIZE			(32)			// 32 GB

#define PAGE_SIZE			(2 *1024)		// 2 KB
#define	BLOCK_SIZE			(64*PAGE_SIZE)	// 128 KB
#define SECTOR_SIZE			(512)			// 512 byte

#define BLOCKS_IN_NAND		(NAND_SIZE * 1024 * 1024 / BLOCK_SIZE * 1024)
#define PAGES_IN_BLOCK		(BLOCK_SIZE / PAGE_SIZE)
#define SECTORS_IN_PAGE		(PAGE_SIZE / SECTOR_SIZE)
#define	SECTORS_IN_BLOCK	(BLOCK_SIZE / SECTOR_SIZE)

#define BASIC_READ_LAT			(25)
#define BASIC_WRITE_LAT			(200)
#define BASIC_ERASE_LAT			(2000)
#define CHANNEL_LAT				(70)

#define READ_LAT			(BASIC_READ_LAT + CHANNEL_LAT)
#define WRITE_LAT			(BASIC_WRITE_LAT + CHANNEL_LAT)
#define ERASE_LAT			(BASIC_ERASE_LAT)

typedef struct page {
	int		dwSector;
	int		bValid;
} PAGE, *PPAGE;

typedef struct nandblock {
	int		dwValidPageCnt;
	int		dwEraseCnt;
	PAGE	astPages[PAGES_IN_BLOCK];
} NANDBLOCK, *PNANDBLOCK;

typedef struct nand {
	NANDBLOCK	astNandBlocks[BLOCKS_IN_NAND];
} NAND, *PNAND;

extern PNAND		gstNand;

void NandInit();
int NandRead(int	dwNandBlockNum,
			 int	dwNandPageIdx);
void NandWrite(int	dwNandBlockNum,
			   int	dwNandPageIdx,
			   int	dwSector);
void NandInvalidPage(int	dwNandBlockNum,
					int		dwNandPageIdx);
void NandErase(int	dwNandBlockNum);
void NandStat(MINT64	*pllReadCnt,
			  MINT64	*pllWriteCnt,
			  MINT64	*pllEraseCnt,
			  MINT64	*pllInvalidCnt);

#endif

