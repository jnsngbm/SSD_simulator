
#include "const.h"
#include "config.h"
#include "nand.h"
#include "ftl.h"
#include "pagemapping.h"
#include "list.h"
#include "library.h"
#ifndef CONFIG_LINUX
#include <assert.h>
#include <stdio.h>
#endif

#ifdef CONFIG_PAGEMAPPING

static PPAGEMAPPING_MAP			pstMap; // Page mapping할 경우 Map으로 한개 지정.
static void*					pstDataBlocks; //datablock의 모음
static PPAGEMAPPING_DATABLOCK	pstCurrent, pstExtra;  //working block, Extra block

static int						adwInvalidCount[PAGES_IN_BLOCK + 1]; // invalidCount 정적 변수 선언 block내에 존재하는 Page의 개수 + 1 -> 0도 포함
static int						dwInvalidMax = 0; dwInvalidMin = PAGES_IN_BLOCK, dwLPcnt = 0, dwGCCnt, dwFreeBlocks = 0, dwTotalBlocks; // dwinvalid max = 0 -> Min = page에 있는 block이 전부 0
static long double				llAvgInvalid = 0, llInvalidSum = 0; // GCCnt -> 정적변수 선언시 0인가? 지정을 해주는 것인가 dwTotalBlocks Total blocks의 수
// long double invalid Avg가 몇인지 계산해야한다. 

static FILE*					myfile; //입출력

static LISTHEAD		lhFreeBlock; // 구조체 inFreeBlock에 LIST형식
static LISTHEAD		lhUsedList[PAGES_IN_BLOCK + 1]; // 구조체 사용된 List의 개수 -> 

static void		_Erase(PPAGEMAPPING_DATABLOCK	pstBlock);  // Erase 함수 선언 = 구조체 datablock이고 간단히 안에 리스트, nandblocknum dwFreepageidx, invaildCnt;가 있다.
static int		_ReadPage(int	dwNandBlockNum, // Page를 읽을 때 필요한 함수 선언 블락과 Page의 index를 알아야한다.
	int	dwPageIdx);
static void		_WritePage(PPAGEMAPPING_DATABLOCK	pstBlock, //Write시 필요한 함수 선언 dwSector -> 주소값 , 
	int						dwSector);
static void		_InvalidPage(int	dwPage); //invalidPage 아직 모르겠다 사용방법

static PPAGEMAPPING_DATABLOCK	_GetInvaildMaxBlock(void); // 안에 invalidMaxBlock을 구하는 방법 -> return값을 invalid page num이 가장 높은 것을 가져온다.
static void _AddUsedList(PPAGEMAPPING_DATABLOCK  pstBlock); // usedList 추가 block자체의 리스트를 추가한다. 
static void	_RemoveUsedList(PPAGEMAPPING_DATABLOCK	pstBlock); // 제거  block의 List를 제거한다.

static void _Merge(PPAGEMAPPING_DATABLOCK		pstBlock); // 병합하는 함수인데 block의 값을 받는다.
static PPAGEMAPPING_DATABLOCK _GetCurrent(void); // 최근에 쓰인 Working block을 찾는 함수 ?


static void _GarbgeCollection(PPAGEMAPPING_DATABLOCK	pstBlock) //  datablock이고 간단히 안에 리스트, nandblocknum dwFreepageidx, invaildCnt;가 있다. 
{
	pstBlock->dwInvaildCnt = 0; // 지정된 block의 invalidCnt는 0이다. -> invaildCnt가 최대로 없다. 
	pstBlock->dwFreePageIdx = 0; // 현재 여기서 부터 Free하니까 기록을 하면된다. GC를 한 이후이기 때문에 Freeindex는 0번부터 기록가능
	NandErase(pstBlock->dwNandBlockNum);// NandErase PstBlock의 num를 알아야 하며 이 Block의 eraseCnt를 ++한다.
	// 
}

static int _ReadPage(int	dwNandBlockNum,				// page를 읽기 위해서는 Block의 num과 page의 index가 필요하다. --> Read가능
	int	dwPageIdx)
{
	return NandRead(dwNandBlockNum, dwPageIdx); 
}

static void _WritePage(PPAGEMAPPING_DATABLOCK	pstBlock,											// Write의 과정
	int						dwSector)																// 해당 Write를 할 주소값, block의 값 필요
{
	MY_ASSERT(gstNand->astNandBlocks[pstBlock->dwNandBlockNum].													 
		astPages[pstBlock->dwFreePageIdx].dwSector == FREE);										// FREE하다고 나와있으면 Write가 가능하다.

	NandWrite(pstBlock->dwNandBlockNum, pstBlock->dwFreePageIdx, dwSector);							// 해당 Sector의 주소값과 pstBlock의 Pageindex를 주어서 해당 Page에 Write할 수 있게 한다.

	pstMap->astEntries[dwSector / SECTORS_IN_PAGE].pstDataBlock = pstBlock;							// block의 값을 알수있다. datablock에 해당 pstBlock의 값을 넣는다 -> number와 Freepageindex,invaildCnt를 알 수 있음.
	pstMap->astEntries[dwSector / SECTORS_IN_PAGE].dwPageIdx = pstBlock->dwFreePageIdx;				// block을 구한뒤 dw pageinex의 수를 찾아 해당 block의 Entries에 넣는다.

	pstBlock->dwFreePageIdx++;																		//Write를 했으므로 이 Block에 FreePageindex를 ++한다.
}

static void _InvalidPage(int	dwPage)																						// 몇번 Block인지 알 수 있다.
{
	PPAGEMAPPING_DATABLOCK		pstBlock;																					// pstBlock을 선언 해당 Block의 num과 Freepageindex, invalidCnt를 알 수 있다.
	PPAGEMAPPING_ENTRY			pMapEntry;																					// dwPageIdx와 pMapEntry.pstDataBlock 를 알 수 있다.

	pMapEntry = &pstMap->astEntries[dwPage];																				// astEntryies의 입력된 Page의 수의 Entry를 pMapEntry에 넣는다. --> 이것을 사용함

	if (pMapEntry->dwPageIdx == FREE)																						//Entry->dwPageindex가 FREE하다면 
	{
		MY_ASSERT(pMapEntry->pstDataBlock == NULL);																			// pstDataBlock이 NULL(비어있다). 를 뜻하며
		dwLPcnt++;																											// dwLPcnt를 ++한다. -> 한 Block내에서 비어있는 Page의 개수를 알 수 있다.
		return;
	}

	pstBlock = pMapEntry->pstDataBlock;

	MY_ASSERT(gstNand->astNandBlocks[pstBlock->dwNandBlockNum].
		astPages[pMapEntry->dwPageIdx].bValid == VALID);																	//VALID 하다면(사용했다면)
	MY_ASSERT(gstNand->astNandBlocks[pstBlock->dwNandBlockNum].
		astPages[pMapEntry->dwPageIdx].dwSector == dwPage * SECTORS_IN_PAGE); 

	pstBlock->dwInvaildCnt++;																								//Block의 invalidCnt를 ++한다.
	NandInvalidPage(pstBlock->dwNandBlockNum, pMapEntry->dwPageIdx);														// block의 index와 Pageindex를 넘겨주고 invalid한 데이터라고 한다면 invalidCnt ++ ValidCnt --한다. 

	MY_ASSERT(gstNand->astNandBlocks[pstBlock->dwNandBlockNum].
		astPages[pMapEntry->dwPageIdx].bValid == INVALID);

	if (pstBlock != pstCurrent && pstBlock != pstExtra && pstBlock->dwFreePageIdx == PAGES_IN_BLOCK)						// it can be in free list.
	{
		MY_ASSERT(pstBlock->dwInvaildCnt <= PAGES_IN_BLOCK);
		MY_ASSERT(pstBlock->dwFreePageIdx == PAGES_IN_BLOCK);
		MY_ASSERT(pstBlock->dwNandBlockNum < BLOCKS_IN_NAND);
		_RemoveUsedList(pstBlock);
		adwInvalidCount[pstBlock->dwInvaildCnt - 1]--;
		_AddUsedList(pstBlock);
		adwInvalidCount[pstBlock->dwInvaildCnt]++;
	}
}

static void	_RemoveUsedList(PPAGEMAPPING_DATABLOCK	pstBlock)				// list head와 entry사이 리스트를 제거한다.
{
	MY_ASSERT(pstBlock->dwFreePageIdx == PAGES_IN_BLOCK);					//왜 같아야함?
	MY_ASSERT(pstBlock->dwInvaildCnt <= PAGES_IN_BLOCK); 
	MY_ASSERT(pstBlock->dwNandBlockNum < BLOCKS_IN_NAND);
	LIST_DEL(&pstBlock->leList);											// 해당 Block에 삭제한다.
}

static void _AddUsedList(PPAGEMAPPING_DATABLOCK  pstBlock)
{
	MY_ASSERT(pstBlock->dwFreePageIdx == PAGES_IN_BLOCK);
	MY_ASSERT(pstBlock->dwInvaildCnt <= PAGES_IN_BLOCK);
	MY_ASSERT(pstBlock->dwNandBlockNum < BLOCKS_IN_NAND);
	LIST_ADD(&pstBlock->leList, &lhUsedList[pstBlock->dwInvaildCnt]); //dwlnvaildCnt(head), head. next에 leList를 추가한다.
}

static PPAGEMAPPING_DATABLOCK	_GetInvaildMaxBlock(void)
{
	int						i;
	PLISTENTRY				pInvaildMaxBlockEntry;
	PPAGEMAPPING_DATABLOCK	pstBlock;

	for (i = PAGES_IN_BLOCK; i>0; i--)
	{
		pInvaildMaxBlockEntry = LIST_CHOP_LAST_ENTRY(&lhUsedList[i]);

		if (pInvaildMaxBlockEntry != NULL)
		{
			pstBlock = LIST_ENTRY(pInvaildMaxBlockEntry, PPAGEMAPPING_DATABLOCK, leList);
			MY_ASSERT(pstBlock->dwInvaildCnt == i || pstBlock->dwInvaildCnt == 0);
			MY_ASSERT(pstBlock->dwFreePageIdx == PAGES_IN_BLOCK || pstBlock->dwFreePageIdx == 0);
			MY_ASSERT(pstBlock->dwNandBlockNum < BLOCKS_IN_NAND);
			adwInvalidCount[i]--;
			return pstBlock;
		}
	}
	return NULL;
}


static void _Merge(PPAGEMAPPING_DATABLOCK		pstBlock)
{
	int							i, j = 0;
	int							dwSector;

	dwGCCnt++;

	MY_ASSERT(pstExtra->dwNandBlockNum < BLOCKS_IN_NAND);

	MY_ASSERT(pstBlock->dwFreePageIdx == PAGES_IN_BLOCK || pstBlock->dwFreePageIdx == 0);
	MY_ASSERT(pstBlock->dwInvaildCnt <= PAGES_IN_BLOCK);
	MY_ASSERT(pstBlock->dwNandBlockNum < BLOCKS_IN_NAND);

	if (pstBlock->dwFreePageIdx == 0)
	{
		pstCurrent = pstExtra;
		pstExtra = pstBlock;

		return;
	}

	if (pstBlock->dwInvaildCnt != PAGES_IN_BLOCK)
	{
		for (i = 0, j = 0; i<PAGES_IN_BLOCK; i++)
		{
			dwSector = _ReadPage(pstBlock->dwNandBlockNum, i);
			if (dwSector != FREE)
			{
				_WritePage(pstExtra, dwSector);
				j++;
			}
			
		}
	}
	MY_ASSERT(j + pstBlock->dwInvaildCnt == PAGES_IN_BLOCK);

	MY_ASSERT(pstExtra->dwFreePageIdx < PAGES_IN_BLOCK);
	MY_ASSERT(pstExtra->dwInvaildCnt == 0);
	MY_ASSERT(pstExtra->dwNandBlockNum < BLOCKS_IN_NAND);

	_Erase(pstBlock);

	MY_ASSERT(pstBlock->dwFreePageIdx == 0);
	MY_ASSERT(pstBlock->dwInvaildCnt == 0);
	MY_ASSERT(pstBlock->dwNandBlockNum < BLOCKS_IN_NAND);

	pstCurrent = pstExtra;
	pstExtra = pstBlock;
}

static PPAGEMAPPING_DATABLOCK _GetCurrent(void)
{
	PPAGEMAPPING_DATABLOCK		pMerge;
	PLISTENTRY					pCurrentEntry;

	if (pstCurrent)
	{
		MY_ASSERT(pstCurrent->dwFreePageIdx < PAGES_IN_BLOCK);
		MY_ASSERT(pstCurrent->dwInvaildCnt < PAGES_IN_BLOCK);
		MY_ASSERT(pstCurrent->dwNandBlockNum < BLOCKS_IN_NAND);
		return pstCurrent;
	}

	pCurrentEntry = LIST_CHOP_LAST_ENTRY(&lhFreeBlock);
	if (pCurrentEntry)
	{
		dwFreeBlocks--;
		pstCurrent = LIST_ENTRY(pCurrentEntry, PPAGEMAPPING_DATABLOCK, leList);
		MY_ASSERT(pstCurrent->dwFreePageIdx < PAGES_IN_BLOCK);
		//MY_ASSERT(pstCurrent->dwInvaildCnt == 0);
		MY_ASSERT(pstCurrent->dwNandBlockNum < BLOCKS_IN_NAND);
		return pstCurrent;
	}
	
	MY_ASSERT(dwFreeBlocks == 0);
	// we need to merge
	pMerge = _GetInvaildMaxBlock();

	_Merge(pMerge);
	MY_ASSERT(pstCurrent);
	MY_ASSERT(pstExtra);

	MY_ASSERT(pstExtra->dwFreePageIdx == 0);
	MY_ASSERT(pstExtra->dwInvaildCnt == 0);
	MY_ASSERT(pstExtra->dwNandBlockNum < BLOCKS_IN_NAND);

	MY_ASSERT(pstCurrent->dwFreePageIdx < PAGES_IN_BLOCK);
	MY_ASSERT(pstCurrent->dwInvaildCnt < PAGES_IN_BLOCK);
	MY_ASSERT(pstCurrent->dwNandBlockNum < BLOCKS_IN_NAND);
	return pstCurrent;
}

void PageMapping_Init()
{
	int dwNandBlkIdx, dwPage, dwTemp;
	int i;
	PPAGEMAPPING_DATABLOCK		pDataBlock;

	pstMap = malloc(sizeof(PAGEMAPPING_MAP));
	pstDataBlocks = malloc(sizeof(PAGEMAPPING_DATABLOCK)* BLOCKS_IN_NAND);

	pDataBlock = (PPAGEMAPPING_DATABLOCK)pstDataBlocks;

	// init lists
	INIT_LIST_HEAD(&lhFreeBlock);
	for (i = 0; i <= PAGES_IN_BLOCK; i++)
	{
		INIT_LIST_HEAD(&lhUsedList[i]);
		adwInvalidCount[i] = 0;
	}

	// init pstDataBlocks
	dwTotalBlocks = BLOCKS_IN_NAND - 1;
	dwFreeBlocks = BLOCKS_IN_NAND - 2;
	dwTemp = BLOCKS_IN_NAND;
	for (dwNandBlkIdx = 0; dwNandBlkIdx < BLOCKS_IN_NAND; dwNandBlkIdx++)
	{
		pDataBlock->dwNandBlockNum = dwNandBlkIdx;
		pDataBlock->dwFreePageIdx = 0;
		pDataBlock->dwInvaildCnt = 0;

		INIT_LIST_HEAD(&pDataBlock->leList);

		if (dwNandBlkIdx == 0)
		{
			pstCurrent = pDataBlock;
		}
		else if (dwNandBlkIdx == 1)
		{
			pstExtra = pDataBlock;
		}
		else
		{
			LIST_ADD(&pDataBlock->leList, &lhFreeBlock);
		}
		pDataBlock++;
	}

	// init map table
	for (dwPage = 0; dwPage < BLOCKS_IN_NAND*PAGES_IN_BLOCK; dwPage++)
	{
		pstMap->astEntries[dwPage].pstDataBlock = NULL;
		pstMap->astEntries[dwPage].dwPageIdx = FREE;
	}
	return;
}

void PageMapping_ReadSector(int dwSector, int dwPageCnt)
{
	int							dwPage, i;
	PPAGEMAPPING_ENTRY			pMapEntry;

	//gllFtlReadCount += dwPageCnt;

	dwPage = dwSector / SECTORS_IN_PAGE;

	for (i = 0; i<dwPageCnt; i++, dwPage++)
	{
		pMapEntry = &pstMap->astEntries[dwPage];

		if (pMapEntry->dwPageIdx != FREE)
		{
			_ReadPage(pMapEntry->pstDataBlock->dwNandBlockNum, pMapEntry->dwPageIdx);
		}
	}

	return;
}

void PageMapping_WriteSector(int dwSector, int dwPageCnt, int dwSectorCnt)
{
	int							dwPage, i;
	PPAGEMAPPING_DATABLOCK		pstBlock;

	//gllFtlWriteCount += dwPageCnt;
	dwPage = dwSector / SECTORS_IN_PAGE; // 몇번 Block인지 알 수 있다.

	for (i = 0; i<dwPageCnt; i++)
	{
		pstBlock = _GetCurrent();
		MY_ASSERT(pstBlock);
		MY_ASSERT(pstBlock->dwFreePageIdx < PAGES_IN_BLOCK);

		_InvalidPage(dwPage);

		_WritePage(pstBlock, dwSector);

		dwSector = dwSector + SECTORS_IN_PAGE;
		dwPage++;

		if (pstBlock->dwFreePageIdx == PAGES_IN_BLOCK)
		{
			// current log is full
			pstCurrent = NULL;
			_AddUsedList(pstBlock);
			adwInvalidCount[pstBlock->dwInvaildCnt]++;
		}
	}
}


FTL		gstPageMapping = {
	PageMapping_Init,
	PageMapping_WriteSector,
	PageMapping_ReadSector,
};



#endif
