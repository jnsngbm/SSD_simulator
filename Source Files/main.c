#include <stdio.h>
#include <stdlib.h>
#include <string.h>											////헤더파일 인클루드 작업 <> 괄호는 이미 만들어져있는것들

#include "ftl.h"
#include "library.h"
#include "const.h"
#include "config.h"											////헤더파일 인클루드 작업 ""괄호는 커스텀한 헤더파일

FTL		*gpFTL = NULL;										////"ftl.h" 에서 구조체 정의함				//// 일단 초기화 시킴

#ifdef CONFIG_BLOCKMAPPING
extern	FTL		gstBlockMapping;		
#endif

#ifdef CONFIG_BAST
extern	FTL		gstBast;									////#include "config.h" 여기 헤더파일을 들어가보면 BAST와 BLOCKMAPPING은 def가 안되어있음 따라서 효력없는 녀석들
#endif				

#ifdef CONFIG_PAGEMAPPING;									////이 친구만 효력이 있다
extern FTL		gstPageMapping;								////extern은 다른 헤더파일에 있는 전역 변수를 끌고 오는거
#endif														////ifdef란 만약 ifdef뒤의 매크로가 (여기선 CONFIG_PAGEMAPPING) 정의 되어 있다면 endif사이의 코드를 실행


int		gdwLogBlockRatio, gdwLogBlockCnt, gdwDataBlockCnt;				////type 정수 설정

void _ReadSector(int dwSector, int dwSectorCnt);
void _WriteSector(int dwSector, int dwSectorCnt);
void _Stat();															////함수선언

int main(int argc, char *argv[])
{
	char	*pFileName, aTemp[10];					
	FILE	*pTrace;				
	int		dwType, dwSector, dwSectorCnt;
	int		dwTemp, i = 0;
	double	lfTemp;

	pFileName = "ntfs1.txt";														////포인터인데 왜 주소값을 안 받을까?

	gdwLogBlockRatio = 0;
	gdwLogBlockCnt = BLOCKS_IN_NAND * gdwLogBlockRatio / 100;						////nand.h 에서 define BLOCKS_IN_NAND		(NAND_SIZE * 1024 * 1024 / BLOCK_SIZE * 1024)
	gdwDataBlockCnt = BLOCKS_IN_NAND - gdwLogBlockCnt;								////LogBlockCnt은 쓰여진 데이터 DataBlockCnt 남아있는거?

//	gpFTL = &gstBlockMapping;
	//gpFTL = &gstBast;																////의미x
	gpFTL = &gstPageMapping;														//// gpFTL 포인터가 gstPageMapping 의 주소값 가지는 중


 	NandInit();																		////erase count, vaild page들 모두 리셋
	gpFTL->InitFtl();																//// ?

	pTrace = fopen(pFileName, "r");													//// "ntfs1.txt"를 read 모드로 open
	if (pTrace == NULL)
	{
		printf("%s file open error\n", pFileName);
		return -1;
	}																				//// 만약 파일이 없으면 에러로 종료
	while (fscanf(pTrace, "%d %lf %lf %d %s %d %d", &dwTemp, &lfTemp, &lfTemp, 
						&dwType, aTemp, &dwSector, &dwSectorCnt) != EOF)			//// type, sector, sectorcount 입력 받기
	{
		if (strcmp(aTemp, "Read") == 0)												//// read모드
		{
			_ReadSector(dwSector, dwSectorCnt);			
		}																			//// 읽기 시작할 섹터와 몇개 읽을지 입력 받은걸로 읽음
		else if (strcmp(aTemp, "Write") == 0)										//// write 모드
		{
			_WriteSector(dwSector, dwSectorCnt);						
		}
		else
		{
			printf("file error\n");
			exit(1);																////read, write 둘다 아니면 에러
		}
		if (i % 10000 == 0)															//// 10000번 실행
		{
			printf("*");
		}
		i++;
	}

	_Stat();

	return 0;
}

void _ReadSector(int dwSector, int dwSectorCnt)
{
	int      dwPageCnt, dwStartPage, dwEndPage, dwEndSector;

	dwEndSector = dwSector + dwSectorCnt - 1;      // 끝나는 섹터 = 시작 섹터 + 읽을 갯수 -1
	dwStartPage = dwSector / SECTORS_IN_PAGE;      // 섹터 / 1페이지당 섹터수 = (여기선) (2 * 1024) / 512
	dwEndPage = dwEndSector / SECTORS_IN_PAGE;      // 끝나는 페이지 = 끝나는 섹터 / 1페이지당 섹터수

	dwSector = dwSector - (dwSector % SECTORS_IN_PAGE);      // 읽는 섹터 - ( 읽는 섹터 / 페이지당 섹터)의 나머지
	dwPageCnt = dwEndPage - dwStartPage + 1;      // 끝나는 페이지 - 시작 페이지 + 1

	gpFTL->ReadSector(dwSector, dwPageCnt);
	return;
}

void _WriteSector(int dwSector, int dwSectorCnt)      // (입력 시작 섹터, 입력할 섹터 수)
{
	int      dwPageCnt, dwStartPage, dwEndPage, dwEndSector;

	dwEndSector = dwSector + dwSectorCnt - 1;         //끝나는 섹터 = 시작 섹터 + 입력할 섹터 수 - 1
	dwStartPage = dwSector / SECTORS_IN_PAGE;         //시작하는 페이지 = 시작 섹터 / 페이지당 섹터수
	dwEndPage = dwEndSector / SECTORS_IN_PAGE;         //끝나는 페이지 = 끝나는 섹터 / 페이지당 섹터수

	if (dwSector % SECTORS_IN_PAGE)                  //만약 ??
	{
		dwSector -= (dwSector % SECTORS_IN_PAGE);		//숑혼뜩岱돨꼬롸
		_ReadSector(dwSector, SECTORS_IN_PAGE);			/*******************/
	}

	if ((dwEndSector + 1) % SECTORS_IN_PAGE)
	{
		_ReadSector(dwEndSector - (dwEndSector % SECTORS_IN_PAGE), SECTORS_IN_PAGE);
	}
	dwPageCnt = dwEndPage - dwStartPage + 1;			//女셕鑒

	gpFTL->WriteSector(dwSector, dwPageCnt);			//쉿셕炬杰돤鑒앴畇흙변鑒
}

void _Stat()		//봬珂셕炬
{
	MINT64	llReadCnt, llWriteCnt, llEraseCnt, llInvalidCount;
	double	nand_iotime;	

	NandStat(&llReadCnt, &llWriteCnt, &llEraseCnt, &llInvalidCount);		

	nand_iotime = (double) (llReadCnt * READ_LAT) / 1000000 + (double) (llWriteCnt * WRITE_LAT) / 1000000 +			// sec
				(double) (llEraseCnt * ERASE_LAT) / 1000000;			// does not include invalid count

	printf("%lf\n", nand_iotime);

	return;
} 
