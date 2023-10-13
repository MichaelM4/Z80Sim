
#ifndef _H_DEFINES_
#define _H_DEFINES_

#include <stdio.h>

#define SizeOfArray(x) (sizeof(x) / sizeof(x[0]))

// set to 1 to enable capture of bus activity logging to sd-card
#define ENABLE_TRACE_LOG 0

///////////////////////////////////////////////////////////////////////////////////////////////////
// Bit - 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14  13  12  11     10      9     8     7      6    5    4    3   2   1  0
// Net -          A1 A0 CD          D7 D6 D5 D4 D3 D2 D1 D0 DIR NMI RST DISKIN DISKOUT WRNMI RDNMI DRVSEL WAIT DAT3 DAT0 CMD CLK OE TXD
//       0  0  0  1  1  1  0  0  0  1  1  1  1  1  1  1  1  0   0   1   1      1       1     1     1      0    0    0    0   0   0  0
//       ----------  ----------  ----------  ----------  ------------   ----------------------     ------------------    ------------
// Hex            1           C           7           F             9                        F                      8               0
#define GPIO_IN_MASK 0x1C7F9F80

#define DRVSEL_MASK   0x00000080
#define RDNMI_MASK    0x00000100
#define WRNMI_MASK    0x00000200
#define DISKOUT_MASK  0x00000400
#define DISKIN_MASK   0x00000800

#define INTR_MASK     (DRVSEL_MASK | RDNMI_MASK | WRNMI_MASK | DISKOUT_MASK | DISKIN_MASK)

#define ADDRESS_MASK  0x18000000
#define DATA_BUS_MASK 0x007F8000

#define NMI_MASK      0x00002000
#define WAIT_MASK     0x00000040

#define RED_LED_PIN 0
#define OE_PIN      1
#define CLK_SCLK    2
#define CMD_MOSI    3
#define DAT0_MISO   4
#define DAT3_CS     5

#define WAIT_PIN    6
#define DRVSEL_PIN  7
#define RDNMI_PIN   8
#define WRNMI_PIN   9
#define DISKOUT_PIN 10
#define DISKIN_PIN  11

#define RESET_PIN   12
#define NMI_PIN     13
#define DIR_PIN     14

#define D0_PIN      15
#define D1_PIN      16
#define D2_PIN      17
#define D3_PIN      18
#define D4_PIN      19
#define D5_PIN      20
#define D6_PIN      21
#define D7_PIN      22

#define CD_PIN      26

#define A0_PIN      27
#define A1_PIN      28

#define DIR_PIN_MASK  0x4000

///////////////////////////////////////////////////////////////////////////////////////////////////

#define BOARD_REVISION 2

extern int g_nRevBoard;

///////////////////////////////////////////////////////////////////////////////////////////////////

// Low Power Timer Overflow Rates in Hz
#define TIMER_OVERFLOW_RATE 2000

///////////////////////////////////////////////////////////////////////////////////////////////////
// generic defines/equates

#ifndef TRUE
	#define FALSE	0
	#define TRUE	1
#endif

#ifndef MAX_PATH
  #define MAX_PATH 256
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// typedefs

typedef struct {
   uint64_t nSectorsTotal;
   uint64_t nSectorsFree;
   uint64_t nSectorSize;

   uint32_t nClustersTotal;
   uint32_t nClustersFree;
   uint32_t nClustersUsed;
   uint32_t nClustersBad;
   uint32_t nClusterSize;
} F_SPACE;

typedef union
{
  struct
  {
    // note that this is valid only if the GPIO all input is rotated down 6 bits
    // in reality WAIT is at GPIO6
    uint32_t WAIT:1;                     /*!< bit:  0      WAIT                               */
    uint32_t DVRSEL:1;                   /*!< bit:  1      DRIVESEL                           */
    uint32_t RDNMI:1;                    /*!< bit:  2      RDNMISTATUS                        */
    uint32_t WRNMI:1;                    /*!< bit:  3      WRNMIMASKREG                       */
    uint32_t DISK_OUT_WE:1;              /*!< bit:  4      DISK_OUT                           */
    uint32_t DISK_IN_RE:1;               /*!< bit:  5      DISK_IN                            */
    uint32_t RESET:1;                    /*!< bit:  6      RESET                              */
    uint32_t NMI:1;                      /*!< bit:  7      NMI                                */
    uint32_t DIR:1;                      /*!< bit:  8      DIR                                */
    uint32_t DATA:8;                     /*!< bit:  9..16  Data Bus                           */

    uint32_t NA2:3;                      /*!< bit:  17..19 NA                                 */
    uint32_t CD:1;                       /*!< bit:  20     CD                                 */
    uint32_t A0:1;                       /*!< bit:  21     A0                                 */
    uint32_t A1:1;                       /*!< bit:  22     A1                                 */

    uint32_t Count:9;                    /*!< bit:  23..31 Count                              */
  } b;                                   /*!< Structure used for bit  access                  */
  uint32_t dw;                           /*!< Type      used for word access                  */
} FDC_BusBitsType;

typedef struct {
	FDC_BusBitsType Bus;
  uint32_t        dwTicks;
} FDC_BusType;

///////////////////////////////////////////////////////////////////////////////////////////////////
// enumerations

void ConfigureCoreClock(int nClockOption);

///////////////////////////////////////////////////////////////////////////////////////////////////
// global variables

//-----------------------------------------------------------------------------
// counter for real time clock (RTC).

extern uint32_t g_dwForegroundRtc;
extern uint32_t g_dwBackgroundRtc;
extern uint16_t  g_wWatchdogRefreshCounter;
extern uint32_t g_dwRTC;

extern uint8_t  g_byCaptureBusActivity;

extern uint32_t g_dwRotationTime;
extern uint32_t g_dwIndexTime;
extern uint32_t g_dwResetTime;

extern uint8_t  g_byMonitorReset;

extern uint64_t g_nWaitTime;
extern uint64_t g_nTimeStart;
extern uint64_t g_nTimeEnd;
extern uint64_t g_nTimeDiff;

extern uint8_t  sd_byCardInialized;

///////////////////////////////////////////////////////////////////////////////////////////////////

void TogglePin(int nCount);
void InitVars(void);
void DecodeBusData(FDC_BusType* pHistory, char* psz, int nType);

///////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t CalcDuration(uint32_t dwEndCount, uint32_t dwStartCount);
void     InitBusTrace(void);
void     FlushTraceBuffer(void);

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
