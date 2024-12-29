
#ifndef _SYSTEM_H
#define _SYSTEM_H

#define MAX_DRIVES 4

#ifndef MAX_PATH
	#define MAX_PATH 256
#endif

// structures
//-----------------------------------------------------------------------------

typedef struct {
	int  nModel;
	char szIniFolder[256];
	char szDrivePath[MAX_DRIVES][MAX_PATH];
	char szRomPath[MAX_PATH];
	int  nRomAddr;
} SystemType;

// unions
//-----------------------------------------------------------------------------

// variables

extern bool g_bCpuLogOpen;
extern SystemType sysdef;

// function definitions
//-----------------------------------------------------------------------------

void OpenCpuLogFile(void);
void CloseCpuLogFile(void);
void WriteCpuLogFile(char* psz);

uint32_t GetCycDuration(uint32_t dwStart, uint32_t dwEnd);
void  StartStopWatch(void);
void  StopStopWatch(void);
float GetStopWatchDuration(void);

uint64_t CountDown(uint64_t nCount, uint64_t nAdjust);
uint64_t CountUp(uint64_t nCount, uint64_t nAdjust);

////////////////////////////////////////////////////////////////////////////////////

char* SkipBlanks(char* psz);
char* SkipToBlank(char* psz);
void  CopySectionName(char* pszSrc, char* pszDst, int nMaxLen);
char* CopyLabelName(char* pszSrc, char* pszDst, int nMaxLen);
void  CopyString(char* pszSrc, char* pszDst, int nMaxLen);
void  StrToUpper(char* psz);
char* stristr(char* psz, char* pszFind);

void  AddTrailingBackslash(char szFilePath[], int nMaxLen);

#ifndef MFC
  int   stricmp(char* psz1, char* psz2);
  void  strcat_s(char* pszDst, int nDstSize, char* pszSrc);
#endif

uint64_t time_us_64(void);

void LoadIniFile(char* pszFileName);

#endif

/* END OF FILE */
