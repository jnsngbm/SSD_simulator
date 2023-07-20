#ifndef _FTL_H_
#define _FTL_H_

#include "nand.h"

typedef struct ftl {
	void (*InitFtl) ();
	void (*WriteSector) (int dwSector, int dwPageCnt);
	void (*ReadSector) (int dwSector, int dwPageCnt);
} FTL;

extern int gdwLogBlockCnt, gdwDataBlockCnt;
extern int gdwLogBlockRatio;
#endif

