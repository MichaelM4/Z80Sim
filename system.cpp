#include "pch.h"

#include "defines.h"
#include "datetime.h"
#include "system.h"
#include "z80emu.h"
#include "fdc.h"
#include "file.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"

#ifndef MFC
extern "C" {
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/structs/systick.h"
}
#endif

extern CpuType cpu;
extern int g_nModel;
extern FdcDriveType g_dtDives[MAX_DRIVES];

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t SystemCoreClock = 133000000;

// Note: g_ (among other) prefix is used to denote global variables

//-----------------------------------------------------------------------------
// counter for real time clock (RTC)

uint32_t g_dwForegroundRtc = 0;
uint32_t g_dwBackgroundRtc;
uint64_t g_nTimeStart;
uint64_t g_nTimeEnd;
uint64_t g_nTimeDiff;

uint32_t g_dwRotationTime;
//uint32_t g_dwIndexTime;
uint32_t g_dwResetTime;
uint8_t  g_byMonitorReset;

uint8_t  g_byMotorWasOn;
uint8_t  g_byFlushTraceBuffer;

uint64_t g_nTimeNow;
uint64_t g_nPrevTime;

SystemType sysdef;

//////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef MFC

CFile g_fCpuLog;
bool  g_bCpuLogOpen = false;

char  g_szCpuLogBuffer[0x40000];
int   g_nCpuLogHead = 0;

void OpenCpuLogFile(void)
{
	if (g_bCpuLogOpen)
	{
		return;
	}

	if (!g_fCpuLog.Open(_T("D:\\Temp\\Z80.txt"), CFile::modeWrite | CFile::modeCreate | CFile::typeBinary))
	{
		return;
	}

	g_nCpuLogHead = 0;
	g_bCpuLogOpen = true;
	g_szCpuLogBuffer[0] = 0;
}

void CloseCpuLogFile(void)
{
	if (!g_bCpuLogOpen)
	{
		return;
	}

	if (g_nCpuLogHead > 0)
	{
		g_fCpuLog.Write(g_szCpuLogBuffer, g_nCpuLogHead);
	}

	g_fCpuLog.Close();
	g_bCpuLogOpen = false;
}

void WriteCpuLogFile(char* psz)
{
	if (!g_bCpuLogOpen)
	{
		return;
	}

	int nLen = (int)strlen(psz);

	if ((nLen + g_nCpuLogHead) >= (sizeof(g_szCpuLogBuffer) - 20))
	{
		g_fCpuLog.Write(g_szCpuLogBuffer, g_nCpuLogHead);
		g_nCpuLogHead = 0;
		g_szCpuLogBuffer[0] = 0;
	}

	strcpy_s(g_szCpuLogBuffer+g_nCpuLogHead, sizeof(g_szCpuLogBuffer)-g_nCpuLogHead, psz);
	g_nCpuLogHead += nLen;
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
void InitVars(void)
{
	g_dwRotationTime = 200000;	// 200ms
//	g_dwIndexTime    = (g_dwRotationTime * 5) / 360;	// 5 degrees for index
	g_dwResetTime    = 1000;	// 1ms
	g_byMonitorReset = FALSE;

	g_byMotorWasOn = 0;
	g_byFlushTraceBuffer = 0;
	g_nTimeNow  = time_us_64();
	g_nPrevTime = g_nTimeNow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const WORD wNormalYearDaysInMonth[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
const WORD wLeapYearDaysInMonth[12]   = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};

///////////////////////////////////////////////////////////////////////////////////////////////////
// parameters: dwSeconds - number of seconds since midnight on January 1st, 1980
//
//             pdt - pointer to a CodedDateTime structure to receive the coded date/time
//                   - .year  - number of years since 1980.  A value of 43 relates to year 2023
//                   - .month - zero based index of the month (0 - 11)
//                   - .day   - zero based index of the day of the month (0 to one less than the number of days in the month)
//                   - .hour  - zero based index of the hour of the day (0 - 23)
//                   - .min   - zero based index of the minute in the hour (0 - 59)
//                   - .sec   - zero based index of the second in the minute (0 - 59)
//
// Note, works up to February, 28h 2100
//
void CodeDateTime(DWORD dwSeconds, CodedDateTime* pdt)
{
	WORD  wNumDays, wYear, wDayOfYear, wTimeBalance, wNumLeapYears, j;
	DWORD dwDayTime;

	wNumDays     = (WORD)(dwSeconds / SECONDS_IN_DAY);
	dwDayTime    = dwSeconds % SECONDS_IN_DAY;
	pdt->hour    = dwDayTime / SECONDS_IN_HOUR;
	wTimeBalance = dwDayTime % SECONDS_IN_HOUR;
	pdt->min     = wTimeBalance / 60;
	pdt->sec     = wTimeBalance % 60;

	wNumLeapYears = wNumDays / DAYS_IN_FOUR_YEARS;
	wNumDays      = wNumDays % DAYS_IN_FOUR_YEARS;

	wYear      = wNumDays / 365 + wNumLeapYears * 4;
	wDayOfYear = wNumDays % 365;
	pdt->year  = wYear;

	j = 11;
	
	while (wNormalYearDaysInMonth[j] > wDayOfYear)
	{
		j--;
	}

	pdt->month = j;												// coded 0..11
	pdt->day   = wDayOfYear - wNormalYearDaysInMonth[j];		// days start at 0
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// psz - points to the null terminated string in the date/time format (MM/DD/YYYY hh:mm:ss)
void ParseDateTime(char* psz, CodedDateTime* pdt)
{
	pdt->month = atoi(psz) - 1;
	pdt->day   = 0;
	pdt->year  = 0;
	pdt->hour  = 0;
	pdt->min   = 0;
	pdt->sec   = 0;
	
	// get day
	while ((*psz != 0) && (*psz != '/'))
	{
		++psz;
	}
	
	if (*psz != '/')
	{
		return;
	}
	
	++psz;

	pdt->day = atoi(psz) - 1;

	// get year
	while ((*psz != 0) && (*psz != '/'))
	{
		++psz;
	}
	
	if (*psz != '/')
	{
		return;
	}
	
	++psz;

	pdt->year = atoi(psz);
	
	if (pdt->year >= 80)
	{
		pdt->year += 1900;
	}
	else
	{
		pdt->year += 2000;
	}

	pdt->year -= 1980;
	
	// get hour
	while ((*psz != 0) && (*psz != ' '))
	{
		++psz;
	}
	
	if (*psz != ' ')
	{
		return;
	}
	
	++psz;

	pdt->hour = atoi(psz);

	// get minute
	while ((*psz != 0) && (*psz != ':'))
	{
		++psz;
	}
	
	if (*psz != ':')
	{
		return;
	}
	
	++psz;

	pdt->min = atoi(psz);

	// get second
	while ((*psz != 0) && (*psz != ':'))
	{
		++psz;
	}
	
	if (*psz != ':')
	{
		return;
	}
	
	++psz;

	pdt->sec = atoi(psz);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long EncodeDateTime(CodedDateTime* pdt)
{
	unsigned long nTime;

	nTime  = pdt->year * SECONDS_IN_YEAR;			// start with seconds for the specified years
	nTime += ((pdt->year / 4) * SECONDS_IN_DAY);	// add a days seconds for each leap year

	if (((pdt->year + 1980) % 4) == 0) // is this a leap year
	{
		nTime += (wLeapYearDaysInMonth[pdt->month] * SECONDS_IN_DAY); // number of seconds to day 1 of the specified month (for a leap year)
	}
	else
	{
		nTime += (wNormalYearDaysInMonth[pdt->month] * SECONDS_IN_DAY); // number of seconds to day 1 of the specified month (for a non leap year)
	}

	nTime += (pdt->day * SECONDS_IN_DAY);    // number of second to start of this day of the month
	nTime += (pdt->hour * SECONDS_IN_HOUR);  // number of seconds to this hour
	nTime += (pdt->min * 60);                // number of seconds to this minute
	nTime += pdt->sec;                       // number of seconds to this second
	
	return nTime;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
uint64_t CountDown(uint64_t nCount, uint64_t nAdjust)
{
	if (nCount > nAdjust)
	{
		nCount -= nAdjust;
	}
	else
	{
		nCount = 0;
	}

	return nCount;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
uint64_t CountUp(uint64_t nCount, uint64_t nAdjust)
{
	if (nCount < (0xFFFFFFFFFFFFFFFF - nAdjust))
	{
		nCount += nAdjust;
	}

	return nCount;
}

////////////////////////////////////////////////////////////////////////////////////
char* SkipBlanks(char* psz)
{
	if (psz == NULL)
	{
		return NULL;
	}
	
	while ((*psz == ' ') && (*psz != 0))
	{
		++psz;
	}
	
	return psz;
}

////////////////////////////////////////////////////////////////////////////////////
char* SkipToBlank(char* psz)
{
	if (psz == NULL)
	{
		return NULL;
	}
	
	while ((*psz != ' ') && (*psz != 0))
	{
		++psz;
	}
	
	return psz;
}

////////////////////////////////////////////////////////////////////////////////////
void CopySectionName(char* pszSrc, char* pszDst, int nMaxLen)
{
	int i = 0;
	
	if (*pszSrc == '[')
	{
		++pszSrc;
	}
	
	while ((i < nMaxLen) && (*pszSrc != ']') && (*pszSrc != 0))
	{
		*pszDst = toupper(*pszSrc);
		++pszDst;
		++pszSrc;
		++i;
	}
	
	*pszDst = 0;
}

////////////////////////////////////////////////////////////////////////////////////
char* CopyLabelName(char* pszSrc, char* pszDst, int nMaxLen)
{
	int i = 0;
	
	while ((i < nMaxLen) && (*pszSrc != '=') && (*pszSrc != 0))
	{
		*pszDst = toupper(*pszSrc);
		++pszDst;
		++pszSrc;
		++i;
	}

	*pszDst = 0;

	if (*pszSrc == '=')
	{
		++pszSrc;
	}
	
	return pszSrc;
}

////////////////////////////////////////////////////////////////////////////////////
void CopyString(char* pszSrc, char* pszDst, int nMaxLen)
{
	int i;
	
	for (i = 0; i < nMaxLen; ++i)
	{
		if (*pszSrc != 0)
		{
			*pszDst = *pszSrc;
			++pszSrc;
		}
		else
		{
			*pszDst = 0;
		}

		++pszDst;
  	}
}

////////////////////////////////////////////////////////////////////////////////////
void StrToUpper(char* psz)
{
	while (*psz != 0)
	{
		*psz = toupper(*psz);
		++psz;
	}
}

////////////////////////////////////////////////////////////////////////////////////
char* stristr(char* psz, char* pszFind)
{
	char* psz1;
	char* psz2;

	while (*psz != 0)
	{
		if (tolower(*psz) == tolower(*pszFind))
		{
			psz1 = psz + 1;
			psz2 = pszFind + 1;

			while ((*psz1 != 0) && (*psz2 != 0) && (tolower(*psz1) == tolower(*psz2)))
			{
				++psz1;
				++psz2;
			}

			if (*psz2 == 0)
			{
				return psz;
			}
		}

		++psz;
	}

	return NULL;
}

#ifndef MFC

///////////////////////////////////////////////////////////////////////////////////////////////////
// returns   0 if psz1 == psz2
//			    -1 if psz1 < psz2
//           1 if psz1 > psz2
int stricmp(char* psz1, char* psz2)
{
	while ((tolower(*psz1) == tolower(*psz2)) && (*psz1 != 0) && (*psz2 != 0))
	{
		++psz1;
		++psz2;
	}

	if ((*psz1 == 0) && (*psz2 == 0))
	{
		return 0;
	}
	else if (*psz1 == 0)
	{
		return -1;
	}
	else if (*psz2 == 0)
	{
		return 1;
	}
	else if (tolower(*psz1) < tolower(*psz2))
	{
		return -1;
	}
	else if (tolower(*psz1) > tolower(*psz2))
	{
		return 1;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void strcat_s(char* pszDst, int nDstSize, char* pszSrc)
{
	int nLen = strlen(pszDst);

	pszDst += nLen;
	
	while ((nLen < nDstSize) && (*pszSrc != 0))
	{
		*pszDst = *pszSrc;
		++pszDst;
		++pszSrc;
		++nLen;
	}

	*pszDst = 0;
}

void strcpy_s(char* pszDst, int nDstSize, char* pszSrc)
{
	int nLen = 0;

	while ((nLen < nDstSize) && (*pszSrc != 0))
	{
		*pszDst = *pszSrc;
		++pszDst;
		++pszSrc;
		++nLen;
	}

	*pszDst = 0;
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
DWORD GetCycDuration(DWORD dwEndCount, DWORD dwStartCount)
{
	if (dwStartCount >= dwEndCount)
	{
		return dwStartCount - dwEndCount;
	}
	else
	{
		return (0x01000000 - dwEndCount) + dwStartCount;
	}
}

////////////////////////////////////////////////////////////////////////////////////
void ValidateIniPath(char szPath[], int nMaxLen)
{
	if (FileExists(szPath))
	{
		return;
	}

#ifdef MFC
	if (szPath[0] != 0)
	{
		CString strMsg = _T("Unable to locate file ");
		strMsg += szPath;
		AfxMessageBox(strMsg);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////
void BuildFilePath(char* pszSrc, char* pszDst, int nDstSize)
{
	*pszDst = 0;

	if (*pszSrc == 0)
	{
		return;
	}

	strcpy_s(pszDst, nDstSize, sysdef.szIniFolder);
	AddTrailingBackslash(pszDst, nDstSize);
	strcat_s(pszDst, nDstSize, pszSrc);
}

////////////////////////////////////////////////////////////////////////////////////
void ProcessConfigEntry(char szLabel[], char* psz)
{
	if (strcmp(szLabel, "DRIVE0") == 0)
	{
		BuildFilePath(psz, sysdef.szDrivePath[0], sizeof(sysdef.szDrivePath[0])-2);
		ValidateIniPath(sysdef.szDrivePath[0], sizeof(sysdef.szDrivePath[0])-2);
	}
	else if (strcmp(szLabel, "DRIVE1") == 0)
	{
		BuildFilePath(psz, sysdef.szDrivePath[1], sizeof(sysdef.szDrivePath[1])-2);
		ValidateIniPath(sysdef.szDrivePath[1], sizeof(sysdef.szDrivePath[1])-2);
	}
	else if (strcmp(szLabel, "DRIVE2") == 0)
	{
		BuildFilePath(psz, sysdef.szDrivePath[2], sizeof(sysdef.szDrivePath[2])-2);
		ValidateIniPath(sysdef.szDrivePath[2], sizeof(sysdef.szDrivePath[2])-2);
	}
	else if (strcmp(szLabel, "DRIVE3") == 0)
	{
		BuildFilePath(psz, sysdef.szDrivePath[3], sizeof(sysdef.szDrivePath[3])-2);
		ValidateIniPath(sysdef.szDrivePath[3], sizeof(sysdef.szDrivePath[3])-2);
	}
	else if (strcmp(szLabel, "ROM") == 0)
	{
		BuildFilePath(psz, sysdef.szRomPath, sizeof(sysdef.szRomPath)-2);
		ValidateIniPath(sysdef.szRomPath, sizeof(sysdef.szRomPath)-2);
	}
	else if (strcmp(szLabel, "ROMADDR") == 0)
	{
		sysdef.nRomAddr = atoi(psz);
	}
	else if (strcmp(szLabel, "MODEL") == 0)
	{
		if (strcmp(psz, "MODEL1") == 0)
		{
			sysdef.nModel = eModel1;
		}
		else if (strcmp(psz, "MODEL3") == 0)
		{
			sysdef.nModel = eModel3;
		}
		else if (strcmp(psz, "MODEL4") == 0)
		{
			sysdef.nModel = eModel4;
		}
		else if (strcmp(psz, "CPM") == 0)
		{
			sysdef.nModel = eCpm;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
void RemoveFileNameFromPath(char szFileName[])
{
	char* psz = szFileName + strlen(szFileName) - 1;

	while ((psz > szFileName) && (*psz != '\\'))
	{
		--psz;
	}

	if (*psz == '\\')
	{
		*psz = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////////
void AddTrailingBackslash(char szFilePath[], int nMaxLen)
{
	char* psz = szFilePath + strlen(szFilePath) - 1;

	if (*psz == '\\')
	{
		return;
	}

	strcat_s(szFilePath, nMaxLen, (char*)"\\");
}

////////////////////////////////////////////////////////////////////////////////////
void LoadIniFile(char* pszFileName)
{
	file* f;
	char  szLine[256];
	char  szSection[16];
	char  szLabel[128];
	char* psz;
	int   nLen;

	FileCloseAll();

	memset(&sysdef, 0, sizeof(sysdef));

	strcpy_s(sysdef.szIniFolder, sizeof(sysdef.szIniFolder)-2, pszFileName);
	RemoveFileNameFromPath(sysdef.szIniFolder);

	sysdef.nRomAddr = 0;

	// read the default ini file to load on init
	f = FileOpen(pszFileName, FA_READ);
	
	if (f == NULL)
	{
#ifdef MFC
		CString strMsg = _T("Unable to open file ");
		strMsg += pszFileName;
		AfxMessageBox(strMsg);
#endif
		return;
	}

	// open the ini file specified in boot.cfg
	nLen = FileReadLine(f, szLine, sizeof(szLine)-2);
	
	while (nLen >= 0)
	{
		psz = SkipBlanks(szLine);
		
		if (*psz == '[')
		{
			CopySectionName(psz, szSection, sizeof(szSection)-1);
		}
		else if ((*psz != 0) && (*psz != ';')) // blank line or a comment line
		{
			StrToUpper(psz);
			psz = CopyLabelName(psz, szLabel, sizeof(szLabel)-1);
			ProcessConfigEntry(szLabel, psz);
		}

		nLen = FileReadLine(f, szLine, sizeof(szLine)-2);
	}
	
	FileClose(f);

	SetModel(sysdef.nModel);
	InitVars();
	InitSystem();
}
