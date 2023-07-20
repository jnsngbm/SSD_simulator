#ifndef _PageMapping_H_
#define _PageMapping_H_

#include "nand.h"
#include "list.h"

typedef struct pagemapping_datablock {
	LISTENTRY		leList;
	int				dwNandBlockNum;
	int				dwFreePageIdx;
	int				dwInvaildCnt;
}  PAGEMAPPING_DATABLOCK, *PPAGEMAPPING_DATABLOCK;				////

typedef struct pagemapping_entry {
	PPAGEMAPPING_DATABLOCK		pstDataBlock;
	int							dwPageIdx;
} PAGEMAPPING_ENTRY, *PPAGEMAPPING_ENTRY;

typedef struct pagemapping_map {
	PAGEMAPPING_ENTRY	astEntries[BLOCKS_IN_NAND*PAGES_IN_BLOCK];
} PAGEMAPPING_MAP, *PPAGEMAPPING_MAP;

void PageMappingInit();
void PageMappingReadSector(int dwSector, int dwPageCnt);
void PageMappingWriteSectror(int dwSector, int dwPageCnt, int dwSectorCnt);


#endif
