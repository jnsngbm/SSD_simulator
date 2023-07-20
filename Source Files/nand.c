#include "const.h"
#include "nand.h"
#include "library.h"
#include "stdlib.h"
#include <assert.h>
#include <stdio.h>

PNAND			gstNand;
static MINT64	llReadCnt, llWriteCnt, llEraseCnt, llInvalidCnt;

static void _CleanPages(PNANDBLOCK	pstNandBlk) //dwValidPageCnt dwEraseCnt astPages[Page_In_Block]
{
	int	i;

	for (i=0; i<PAGES_IN_BLOCK; i++)
	{
		pstNandBlk->astPages[i].dwSector = FREE;		//姦렴첼몸혐
		pstNandBlk->astPages[i].bValid = INVALID;		//겉唐槻女긴槨轟槻女
	} 
}
/*typedef struct nandblock {
	int		dwValidPageCnt;
	int		dwEraseCnt;
	PAGE	astPages[PAGES_IN_BLOCK];
} NANDBLOCK, *PNANDBLOCK;
*/

void NandInit()
{
	int i;

	gstNand = malloc(sizeof(NAND));							//gstNand에게  NAND의 size만큼 메모리 할당

	for (i=0; i<BLOCKS_IN_NAND; i++)						//NAND의 block수만큼 반복
	{
		gstNand->astNandBlocks[i].dwValidPageCnt = 0;		//헌쥐唐槻女
		gstNand->astNandBlocks[i].dwEraseCnt = 0;			//헌쥐轟槻女

		_CleanPages(&gstNand->astNandBlocks[i]);			//헌뇜닸女
	}

	return;
}

int NandRead(int	dwNandBlockNum,
			 int	dwNandPageIdx)															//渴흙꽝鑒槨욥뵀뵨女뵀
{
	llReadCnt++;																			//뗍혤셕鑒	
	if (gstNand->astNandBlocks[dwNandBlockNum].astPages[dwNandPageIdx].bValid == VALID)		//흔벎唐槻 橙럿쀼令
	{
		return gstNand->astNandBlocks[dwNandBlockNum].astPages[dwNandPageIdx].dwSector;
	}
	else                              //흔벎轟槻 橙럿쀼왕
	{
		return FREE;
	}
}

void NandWrite(int	dwNandBlockNum,
			   int	dwNandPageIdx,
			   int	dwSector)																				//渴흙꽝鑒槨욥뵀 女뵀뵨코휭
{
	llWriteCnt++; //전체 Write의 cnt를 ++한다.

	MY_ASSERT(gstNand->astNandBlocks[dwNandBlockNum].astPages[dwNandPageIdx].dwSector == FREE);				// FREE하다면 데이터 Write가능 체크를 2번한다 
	MY_ASSERT(gstNand->astNandBlocks[dwNandBlockNum].astPages[dwNandPageIdx].bValid == INVALID);			// INVALID해야지 Write 가능

	gstNand->astNandBlocks[dwNandBlockNum].dwValidPageCnt++;												//Write를 했으니 PageCnt ++를 해준다.
	
	gstNand->astNandBlocks[dwNandBlockNum].astPages[dwNandPageIdx].dwSector = dwSector;						//dwSector에는 값을 Write한 주소값을 넣어주고
	gstNand->astNandBlocks[dwNandBlockNum].astPages[dwNandPageIdx].bValid = VALID;							//VALID로 바꾸어준다.
}

void NandInvalidPage(int	dwNandBlockNum,													//Block num과 pageindex를  숫자로 받고
					int		dwNandPageIdx)
{
	if (gstNand->astNandBlocks[dwNandBlockNum].astPages[dwNandPageIdx].bValid == VALID)		//데이터가 저장되어있다면 
	{
		llInvalidCnt++;																		// invalidCnt를 ++한다. 원래 invalidCnt는 0개인데 데이터가 저장된다면 invalid하게 바뀐 데이터가 1개 추가되기때문이다.
		gstNand->astNandBlocks[dwNandBlockNum].astPages[dwNandPageIdx].bValid = INVALID;	//
		gstNand->astNandBlocks[dwNandBlockNum].dwValidPageCnt--;							//dwValidPageCnt는 invalid로 바뀌었기 때문에 1개 마이너스된다.
	}
}

void NandErase(int	dwNandBlockNum)
{
	llEraseCnt++;			//전체 EraseCnt++를 한다.
	
	gstNand->astNandBlocks[dwNandBlockNum].dwValidPageCnt = 0;			//Valid한 Page의 Cnt는 0이다. -> GC
	gstNand->astNandBlocks[dwNandBlockNum].dwEraseCnt++;				//해당 Block의 EraseCnt는 ++이다.
		
	_CleanPages(&(gstNand->astNandBlocks[dwNandBlockNum]));		//CleanPages -> Page를 Clean하게 해주는 함수이다. 따라서 nandblocknum을 넘겨주면 해당 Page를 Free시킨다.
}

void NandStat(MINT64	*pllReadCnt,			
			  MINT64	*pllWriteCnt,
			  MINT64	*pllEraseCnt,
			  MINT64	*pllInvalidCnt)
{
	*pllReadCnt = llReadCnt;
	*pllWriteCnt = llWriteCnt;
	*pllEraseCnt = llEraseCnt;
	*pllInvalidCnt = llInvalidCnt;
	free(gstNand);
}

