#include "pch.h"
#include "Z80Emu.h"
#include "fdc.h"
#include "system.h"
#include "keyboard.h"

extern CpuType cpu;
extern byte g_byMemory[];
extern int  g_nKeyboardReadCount;
extern byte g_byVideoMemory[VIDEO_BUFFER_SIZE];
extern word g_wVideoStart;
extern word g_wVideoEnd;
extern byte g_byVideoNumCols;
extern byte g_byVideoNumRows;
extern int  g_nVideoModified;
extern byte g_byKeyboardMemory[0x400];
extern word g_wKeyboardStart;
extern word g_wKeyboardEnd;
extern int  nCharBufferHead;
extern int  nCharBufferTail;
extern int  nCharBufferSize;
extern UINT nCharBuffer[];

int g_nVideoCursor;
int g_nVideoRow;
int g_nVideoCol;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CPM_RtcIntr()
{
}

//-----------------------------------------------------------------------------
byte fdc_read_nmi(void)
{
	return g_FDC.byNmiStatusReg;
}

//-----------------------------------------------------------------------------
// B7 - ENINTRQ (0-disabled disk INTRQ from generating an NMI)
// B6 - ENDRQ   (0-disables disk DRQ form generating an NMIM)
void fdc_write_nmi(byte byData)
{
#ifdef ENABLE_LOGGING
	if (g_bLogOpen)
	{
	  char szBuf[128];
	  sprintf_s(szBuf, sizeof(szBuf), "    FDC  WR NMI %02X\r\n", byData);
	  WriteLogFile(szBuf);
	}
#endif

	g_FDC.byNmiMaskReg = byData;

	if (byData & 0x80)
	{
		g_FDC.byNmiMaskReg = byData;
	}
	else
	{
		g_byIntrRequest = 0;
	}
}

//----------------------------------------------------------------------------
void fdc_process_command_request(byte by)
{
	if (by == CPM_READ_BLOCK_CMD) // read the CPM 512-byte block indicated by HL, into memtory indicated by DE
	{
		if ((g_FDC.byDriveSel < MAX_DRIVES) && (g_dtDives[g_FDC.byDriveSel].f != NULL))
		{
			int nOffset = (cpu.regs.wordregs.hl - CPM_PARTITION_BASE) * CPM_BLOCK_SIZE;
			FileSeek(g_dtDives[g_FDC.byDriveSel].f, nOffset);
			FileRead(g_dtDives[g_FDC.byDriveSel].f, g_byMemory+cpu.regs.wordregs.de, CPM_BLOCK_SIZE);
			cpu.regs.byteregs.a = 0;
		}
	}
	else if (by == CPM_WRITE_BLOCK_CMD)
	{
		if ((g_FDC.byDriveSel < MAX_DRIVES) && (g_dtDives[g_FDC.byDriveSel].f != NULL))
		{
			int nOffset = (cpu.regs.wordregs.hl - CPM_PARTITION_BASE) * CPM_BLOCK_SIZE;
			FileSeek(g_dtDives[g_FDC.byDriveSel].f, nOffset);
			FileWrite(g_dtDives[g_FDC.byDriveSel].f, g_byMemory+cpu.regs.wordregs.de, CPM_BLOCK_SIZE);
			cpu.regs.byteregs.a = 0;
		}
	}
}

void ProcessVideoChar(byte by)
{
  if (by == 8) // BS
  {
    if (g_nVideoCol > 0)
    {
      --g_nVideoCursor;
      --g_nVideoCol;
      g_byVideoMemory[g_nVideoCursor] = ' ';
    }
  }
  else if (by == 13) // CR
  {
    g_nVideoCol = 0;
    g_nVideoCursor = g_nVideoRow * g_byVideoNumCols;
  }
  else if (by == 10) // LF
  {
    g_nVideoCol = 0;
    ++g_nVideoRow;

    if (g_nVideoRow == g_byVideoNumRows)
    {
      memmove(g_byVideoMemory, g_byVideoMemory+g_byVideoNumCols, sizeof(g_byVideoMemory)-g_byVideoNumCols);
      memset(g_byVideoMemory+sizeof(g_byVideoMemory)-g_byVideoNumCols, ' ', g_byVideoNumCols);
      --g_nVideoRow;
    }

    g_nVideoCursor = g_nVideoRow * g_byVideoNumCols;
  }
  else if (sizeof(g_byVideoMemory) > g_nVideoCursor)
  {
    g_byVideoMemory[g_nVideoCursor] = by;
    ++g_nVideoCursor;
    ++g_nVideoCol;
  }

  ++g_nVideoModified;
}

void ProcessVideoCommand(byte by)
{
  switch (by)
  {
    case 255:
      g_nVideoCursor = 0;
      g_nVideoCol = 0;
      g_nVideoRow = 0;
      memset(g_byVideoMemory, ' ', sizeof(g_byVideoMemory));
      break;
  }
}

void CPM_OutPort(byte port, byte by)
{
  switch (port)
  {
    case 0xE4: // WRNMIMASKREG
      fdc_write_nmi(by);
      break;

 		case 0xF0: // Command register
		case 0xF1: // Track register
		case 0xF2: // Sector register
		case 0xF3: // Data register
			fdc_write(port, by);
      break;

    case 0xF4:
			fdc_write_drive_select(by);
      break;

    case 0xF5:
      fdc_process_command_request(by);
      break;

    case 0xFE:
      ProcessVideoChar(by);
      break;

    case 0xFF:
      ProcessVideoCommand(by);
      break;
  }
}

byte CPM_InPort(byte port)
{
  byte by = 0;

  switch (port)
  {
		case 0xF0: // Status register
    case 0xF1: // Track register
		case 0xF2: // Sector register
		case 0xF3: // Data register
    	return fdc_read(port);
      break;

    case 0xFE: // character input
		  if (nCharBufferTail == nCharBufferHead)
      {
        by = 0;
      }
      else
      {
        by = nCharBuffer[nCharBufferTail];

		    ++nCharBufferTail;

		    if (nCharBufferTail >= nCharBufferSize)
		    {
		  	  nCharBufferTail = 0;
		    }
      }

      break;

    case 0xFF: // console input status
		  if (nCharBufferTail != nCharBufferHead)
      {
        by = 0xFF;
      }
      else
      {
        by = 0;
      }

      break;
  }

  return by;
}

byte CPM_InPort16(byte port, byte addr)
{
  return CPM_InPort(port);
}

byte CPM_MemRead(word addr)
{
  byte byRet = g_byMemory[addr];

#ifdef ENABLE_LOGGING
  if (g_bLogOpen)
  {
     char szBuf[128];
     sprintf_s(szBuf, sizeof(szBuf), "     RD %04X => %02X\r\n", addr, byRet);
     WriteLogFile(szBuf);
  }
#endif

  return byRet;
}

void CPM_MemWrite(word addr, byte by)
{
#ifdef ENABLE_LOGGING
  if (g_bLogOpen)
  {
     char szBuf[128];
     sprintf_s(szBuf, sizeof(szBuf), "     WR %04X <= %02X\r\n", addr, by);
     WriteLogFile(szBuf);
  }
#endif

  g_byMemory[addr] = by;
}

void CPM_Init(void)
{
	file* f;
	byte  byBuf[256];
	int   i, j, n, nRomAddr;

	// read the default ini file to load on init
	f = FileOpen(sysdef.szRomPath, FA_READ);
	
	if (f == NULL)
	{
		return;
	}

  nRomAddr = sysdef.nRomAddr;
  j = nRomAddr;

  n = FileRead(f, byBuf, sizeof(byBuf));

  while (n > 0)
  {
    for (i = 0; i < n; ++i)
    {
      g_byMemory[j] = byBuf[i];
      ++j;
    }

    n = FileRead(f, byBuf, sizeof(byBuf));
  }

	FileClose(f);

  g_byMemory[0] = 0xC3; // JP 0xC000
  g_byMemory[1] = nRomAddr & 0xFF;
  g_byMemory[2] = (nRomAddr >> 8) & 0xFF;

	memset(g_byVideoMemory, 0x20, sizeof(g_byVideoMemory));
	g_wVideoStart        = 0x3C00;
	g_wVideoEnd          = 0x3FFF;
	g_byVideoNumCols     = 80;
	g_byVideoNumRows     = VIDEO_BUFFER_SIZE / g_byVideoNumCols;
	g_nVideoModified     = 0;
  g_nVideoCursor       = 0;
  g_nVideoRow          = 0;
  g_nVideoCol          = 0;
  g_wKeyboardStart     = 0x3800;
	g_wKeyboardEnd       = 0x3BFF;
  g_byKeyboardMode     = eKeyPortMapped;
}
