#pragma once

typedef unsigned char byte;
typedef unsigned short word;

#define CPU_CLOCK 10000000

#define EXEC_LOOP_COUNT 1000                                        // number of clock cycles/states to run in EmuExecute() before execting
#define EXEC_LOOP_TIME ((EXEC_LOOP_COUNT * 1000000) / CPU_CLOCK) // number of u seconds to run EXEC_LOOP_COUNT states

#define MODEL1_RTC_INTERRUPT_PERIOD (CPU_CLOCK / 40)
#define MODEL3_RTC_INTERRUPT_PERIOD (CPU_CLOCK / 30)
#define MODEL_CPM_RTC_INTERRUPT_PERIOD (CPU_CLOCK / 40)

#define VIDEO_BUFFER_SIZE 0x800

#define BASE_MEMORY_SIZE 0x10000

enum {
	eModel1 = 0,
	eModel3,
	eModel4,
	eCpm,
};

/*
 * Z80REGS/H - Definitions for Z80 register structures, used with call()
 */
union REGS
{
	struct		/* double register variables */
	{
		word af;
		word bc;
		word de;
		word hl;
		word ix;
		word iy;
	} wordregs;

	struct		/* single register variables */
	{
		byte f;
		byte a;
		byte c;
		byte b;
		byte e;
		byte d;
		byte l;
		byte h;
		byte ixl; // low byte of ix
		byte ixh; // high byte of ix
		byte iyl; // low byte of iy
		byte iyh; // high byte of iy
	} byteregs;
};

union two_comp
{
	unsigned char by;
	signed char n;
};

/* simpler definitions */

#define	AF wordregs.af
#define	BC wordregs.bc
#define	DE wordregs.de
#define	HL wordregs.hl
#define	IX wordregs.ix
#define	IY wordregs.iy
#define	F  byteregs.f
#define	A  byteregs.a
#define	C  byteregs.c
#define	B  byteregs.b
#define	E  byteregs.e
#define	D  byteregs.d
#define	L  byteregs.l
#define	H  byteregs.h
#define	LX byteregs.lx
#define	HX byteregs.hx
#define	LY byteregs.ly
#define	HY byteregs.hy

#define S_FLAG_MASK 0x80
#define Z_FLAG_MASK 0x40
#define Y_FLAG_MASK 0x20
#define H_FLAG_MASK 0x10
#define X_FLAG_MASK 0x08
#define P_FLAG_MASK 0x04
#define N_FLAG_MASK 0x02
#define C_FLAG_MASK 0x01

typedef struct {
	union REGS regs;		  // main and alternate sets
	union REGS alt_regs;  // main and alternate sets
	word pc;				// program counter
	word sp;				// stack pointer
	byte i;
	byte r;
	byte halt;
	byte iff1;
	byte iff2;
	byte imode;
	byte wait;
	byte intr;
	byte nmi;
	uint32_t cycles;
	uint64_t nTotalCycles;
	uint64_t nExecTickCount;
} CpuType;

uint64_t time_us_64(void);

void ResetCpu(void);
void InitSystem(void);

#ifdef MFC
	UINT EmuExecute(LPVOID pParm);
#else
	void EmuExecute(void);
#endif

void StopEmuThread(void);

byte Model1_MemRead(word addr);
void Model1_MemWrite(word addr, byte by);
void Model1_Init(void);
void Model1_OutPort(byte port, byte by);
byte Model1_InPort(byte port);
byte Model1_InPort16(byte port, byte addr);
void Model1_RtcIntr();

byte Model3_MemRead(word addr);
void Model3_MemWrite(word addr, byte by);
void Model3_Init(void);
void Model3_OutPort(byte port, byte val);
byte Model3_InPort(byte port);
byte Model3_InPort16(byte port, byte addr);
void Model3_RtcIntr();

byte Model4_MemRead(word addr);
void Model4_MemWrite(word addr, byte by);
void Model4_Init(void);
void Model4_OutPort(byte port, byte val);
byte Model4_InPort(byte port);
byte Model4_InPort16(byte port, byte addr);
void Model4_RtcIntr();

byte CPM_MemRead(word addr);
void CPM_MemWrite(word addr, byte by);
void CPM_Init(void);
void CPM_OutPort(byte port, byte val);
byte CPM_InPort(byte port);
byte CPM_InPort16(byte port, byte addr);
void CPM_RtcIntr();

void SetModel(int nModel);

