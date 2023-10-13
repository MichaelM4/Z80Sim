#include "pch.h"
#include "defines.h"
#include "z80emu.h"
#include "z80dis.h"
#include "fdc.h"
#include "system.h"
#include "string.h"
#include "keyboard.h"

//#define ENABLE_LOGGING 1

byte g_byMemory[BASE_MEMORY_SIZE];

int  g_nRtcCounter;
int  g_nRtcIntrPeriod;

uint64_t g_nSystemTickCount = 0;

CpuType cpu;

typedef struct {
  byte bytes;
  byte cycles;
  void (*inst)();
} InstType;

int g_nModel = eModel1;

/////////////////////////////////////////////////
byte byParity[] = {
  4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
  0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
  0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
  4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
  0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
  4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
  4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
  0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
  0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
  4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
  4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
  0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
  4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
  0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
  0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
  4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
};

byte DefaultReadMem(word addr);

byte (*pMemRead)(word) = DefaultReadMem;
void (*pMemWrite)(word, byte);
void (*pModelInit)();
byte (*pPortIn)(byte);
byte (*pPortIn16)(byte, byte);
void (*pPortOut)(byte, byte);
void (*pRtcIntr)();

/////////////////////////////////////////////////

#define GetMem(a)        (*pMemRead)(a)
#define PutMem(a, b)     (*pMemWrite)(a, b)

#define ReadMem(a)       (*pMemRead)(a)
#define WriteMem(a, b)   (*pMemWrite)(a, b)
#define WritePort(a, b)  (*pPortOut)(a, b)
#define ReadPort(a)      (*pPortIn)(a)
#define ReadPort16(a, b) (*pPortIn16)(a, b)

/////////////////////////////////////////////////

#ifndef MFC
static void ASSERT(bool b)
{
}
#endif

void DefaultOutPort(byte port, byte val)
{
}

byte DefaultInPort(byte port)
{
  return 0x00;
}

byte DefaultInPort16(byte port)
{
  return 0x00;
}

byte DefaultReadMem(word addr)
{
  return g_byMemory[addr];
}

/////////////////////////////////////////////////

void Z80_ModelInit(void)
{
  switch (g_nModel)
  {
    case eModel1:
      pMemRead   = Model1_MemRead;
      pMemWrite  = Model1_MemWrite;
      pModelInit = Model1_Init;
      pPortOut   = Model1_OutPort;
      pPortIn    = Model1_InPort;
      pPortIn16  = Model1_InPort16;
      pRtcIntr   = Model1_RtcIntr;
      g_nRtcIntrPeriod = MODEL1_RTC_INTERRUPT_PERIOD;
      break;

    case eModel3:
      pMemRead   = Model3_MemRead;
      pMemWrite  = Model3_MemWrite;
      pModelInit = Model3_Init;
      pPortOut   = Model3_OutPort;
      pPortIn    = Model3_InPort;
      pPortIn16  = Model3_InPort16;
      pRtcIntr   = Model3_RtcIntr;
      g_nRtcIntrPeriod = MODEL3_RTC_INTERRUPT_PERIOD;
      break;

    case eModel4:
      pMemRead   = Model4_MemRead;
      pMemWrite  = Model4_MemWrite;
      pModelInit = Model4_Init;
      pPortOut   = Model4_OutPort;
      pPortIn    = Model4_InPort;
      pPortIn16  = Model4_InPort16;
      pRtcIntr   = Model4_RtcIntr;
      g_nRtcIntrPeriod = MODEL3_RTC_INTERRUPT_PERIOD;
      break;

    case eCpm:
      pMemRead   = CPM_MemRead;
      pMemWrite  = CPM_MemWrite;
      pModelInit = CPM_Init;
      pPortOut   = CPM_OutPort;
      pPortIn    = CPM_InPort;
      pPortIn16  = CPM_InPort16;
      pRtcIntr   = CPM_RtcIntr;
      g_nRtcIntrPeriod = MODEL3_RTC_INTERRUPT_PERIOD;
      break;
  }

  (*pModelInit)();
}

void SetModel(int nModel)
{
  g_nModel = nModel;
}

void InitSystem(void)
{
  ResetCpu();

  memset(g_byMemory, 0xFF, sizeof(g_byMemory));

  g_nRtcCounter = 0;
  g_nSystemTickCount = 0;
  InitKeyboard();
  Z80_ModelInit();
	FdcInit();
}

#ifdef MFC
UINT64 time_us_64(void)
{
  return g_nSystemTickCount;
}
#endif

void ResetCpu(void)
{
  memset(&cpu, 0, sizeof(cpu));
  cpu.halt = 0;
  cpu.iff1 = 0;
  cpu.iff2 = 0;
}

void GenerateNMI(void)
{
  cpu.halt = 0;

  // RST 0x0066
  WriteMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
  WriteMem(cpu.sp-2, cpu.pc & 0xFF);

  cpu.pc = 0x0066;
  cpu.sp -= 2;
  cpu.cycles += 11;
}

void SignalHalt(void)
{
  GenerateNMI(); // for Model 1
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#define getbit(n, val) (((val) >> (n)) & 1)
#define get_de() cpu.regs.wordregs.de
#define set_de(x) cpu.regs.wordregs.de = x
#define get_hl() cpu.regs.wordregs.hl
#define set_hl(x) cpu.regs.wordregs.hl = x
#define get_bc() cpu.regs.wordregs.bc
#define set_bc(x) cpu.regs.wordregs.bc = x
#define get_ix() cpu.regs.wordregs.ix
#define set_ix(x) cpu.regs.wordregs.ix = x
#define get_iy() cpu.regs.wordregs.iy
#define set_iy(x) cpu.regs.wordregs.iy = x

uint8_t subb(uint8_t a, uint8_t b, bool cy);

bool getflag(word mask)
{
  if (cpu.regs.byteregs.f & mask)
  {
    return 1;
  }

  return 0;
}

void setflag(byte mask, bool state)
{
  if (state)
  {
    cpu.regs.byteregs.f |= mask;
  }
  else
  {
    cpu.regs.byteregs.f &= ~mask;
  }
}

void land(uint8_t val)
{
  const uint8_t result = cpu.regs.byteregs.a & val;

  // S, Z, Y, H=1, X, P=Parity, N=0, C=0
  cpu.regs.byteregs.f = H_FLAG_MASK | byParity[result];

  // setflag(S_FLAG_MASK, result >> 7);
  if (result >> 7)
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }

  // setflag(Z_FLAG_MASK, result == 0);
  if (result == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }

  // setflag(X_FLAG_MASK, getbit(3, result));
  if (result & 0x08)
  {
    cpu.regs.byteregs.f |= X_FLAG_MASK;
  }

  // setflag(Y_FLAG_MASK, getbit(5, result));
  if (result & 0x20)
  {
    cpu.regs.byteregs.f |= Y_FLAG_MASK;
  }

  cpu.regs.byteregs.a = result;
}

void lor(const uint8_t val)
{
  const uint8_t result = cpu.regs.byteregs.a | val;

  // S, Z, Y, H=0, X, P=Parity, N=0, C=0
  cpu.regs.byteregs.f = byParity[result];

  // setflag(S_FLAG_MASK, result >> 7);
  if (result >> 7)
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }

  // setflag(Z_FLAG_MASK, result == 0);
  if (result == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }

  // setflag(X_FLAG_MASK, getbit(3, result));
  if (result & 0x08)
  {
    cpu.regs.byteregs.f |= X_FLAG_MASK;
  }

  // setflag(Y_FLAG_MASK, getbit(5, result));
  if (result & 0x20)
  {
    cpu.regs.byteregs.f |= Y_FLAG_MASK;
  }

  cpu.regs.byteregs.a = result;
}

void lxor(const uint8_t val)
{
  const uint8_t result = cpu.regs.byteregs.a ^ val;

  // S, Z, Y, H=0, X, P=Parity, N=0, C=0
  cpu.regs.byteregs.f = byParity[result];

  // setflag(S_FLAG_MASK, result >> 7);
  if (result >> 7)
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }

  // setflag(Z_FLAG_MASK, result == 0);
  if (result == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }

  // setflag(X_FLAG_MASK, getbit(3, result));
  if (result & 0x08)
  {
    cpu.regs.byteregs.f |= X_FLAG_MASK;
  }

  // setflag(Y_FLAG_MASK, getbit(5, result));
  if (result & 0x20)
  {
    cpu.regs.byteregs.f |= Y_FLAG_MASK;
  }


  cpu.regs.byteregs.a = result;
}

uint8_t cb_bit(uint8_t val, uint8_t n)
{
  const uint8_t result = val & (1 << n);

  // S, Z, Y, H=1, X, P N=0, C=C
  cpu.regs.byteregs.f = (cpu.regs.byteregs.f & C_FLAG_MASK) | H_FLAG_MASK;

  setflag(S_FLAG_MASK, result >> 7);
  setflag(Z_FLAG_MASK, result == 0);
  setflag(Y_FLAG_MASK, getbit(5, val));
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(P_FLAG_MASK, result == 0);
  return result;
}

void bit(byte val, byte n)
{
  const uint8_t result = val & n;

  // S, Z, Y, H=1, X, P, N=0, C
  cpu.regs.byteregs.f = (cpu.regs.byteregs.f & C_FLAG_MASK) | H_FLAG_MASK;

  setflag(S_FLAG_MASK, result >> 7);
  setflag(Z_FLAG_MASK, result == 0);
  setflag(Y_FLAG_MASK, getbit(5, val));
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(P_FLAG_MASK, result == 0);
}

void cp(const uint8_t val)
{
  subb(cpu.regs.byteregs.a, val, 0);

  // the only difference between cp and sub is that
  // the xf/yf are taken from the value to be substracted,
  // not the result
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(Y_FLAG_MASK, getbit(5, val));
}

void cpi()
{
  byte    cf = getflag(C_FLAG_MASK);
  uint8_t result = subb(cpu.regs.byteregs.a, ReadMem(get_hl()), 0);
  set_hl(get_hl() + 1);
  set_bc(get_bc() - 1);
  setflag(X_FLAG_MASK, getbit(3, result - getflag(H_FLAG_MASK)));
  setflag(Y_FLAG_MASK, getbit(1, result - getflag(H_FLAG_MASK)));
  setflag(P_FLAG_MASK, get_bc() != 0);
  setflag(C_FLAG_MASK, cf);
}

void cpd()
{
  cpi();

  // same as cpi but HL is decremented instead of incremented
  set_hl(get_hl() - 2);
}

void ldi()
{
  const uint16_t de = get_de();
  const uint16_t hl = get_hl();
  const uint8_t val = GetMem(hl);

  PutMem(de, val);

  set_hl(get_hl() + 1);
  set_de(get_de() + 1);
  set_bc(get_bc() - 1);

  // see https://wikiti.brandonw.net/index.php?title=Z80_Instruction_Set
  // for the calculation of xf/yf on LDI
  const uint8_t result = val + cpu.regs.byteregs.a;
  
  setflag(X_FLAG_MASK, getbit(3, result));
  setflag(Y_FLAG_MASK, getbit(1, result));
  setflag(N_FLAG_MASK, 0);
  setflag(H_FLAG_MASK, 0);
  setflag(P_FLAG_MASK, get_bc() > 0);
}

void ldd()
{
  const uint16_t de = get_de();
  const uint16_t hl = get_hl();
  const uint8_t val = GetMem(hl);

  PutMem(de, val);

  set_hl(get_hl() - 1);
  set_de(get_de() - 1);
  set_bc(get_bc() - 1);

  // see https://wikiti.brandonw.net/index.php?title=Z80_Instruction_Set
  // for the calculation of xf/yf on LDI
  const uint8_t result = val + cpu.regs.byteregs.a;
  
  setflag(X_FLAG_MASK, getbit(3, result));
  setflag(Y_FLAG_MASK, getbit(1, result));
  setflag(N_FLAG_MASK, 0);
  setflag(H_FLAG_MASK, 0);
  setflag(P_FLAG_MASK, get_bc() > 0);
}

uint8_t cb_rlc(uint8_t val)
{
  uint8_t old = val >> 7;
  val = (val << 1) | old;

  // S, Z, Y, H=0, X, P, N=0, C=C
  cpu.regs.byteregs.f = 0;

  setflag(S_FLAG_MASK, val >> 7);
  setflag(Z_FLAG_MASK, val == 0);
  cpu.regs.byteregs.f |= byParity[val];
  setflag(C_FLAG_MASK, old);
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(Y_FLAG_MASK, getbit(5, val));
  return val;
}

uint8_t cb_rrc(uint8_t val)
{
  uint8_t old = val & 1;
  val = (val >> 1) | (old << 7);

  // S, Z, Y, H=0, X, P, N=0, C=C
  cpu.regs.byteregs.f = 0;

  setflag(S_FLAG_MASK, val >> 7);
  setflag(Z_FLAG_MASK, val == 0);
  setflag(C_FLAG_MASK, old);
  cpu.regs.byteregs.f |= byParity[val];
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(Y_FLAG_MASK, getbit(5, val));
  return val;
}

uint8_t cb_rl(uint8_t val)
{
  uint8_t cf = getflag(C_FLAG_MASK);
  setflag(C_FLAG_MASK, val >> 7);
  val = (val << 1) | cf;

  // S, Z, Y, H=0, X, P, N=0, C=C
  cpu.regs.byteregs.f = cpu.regs.byteregs.f & C_FLAG_MASK;

  setflag(S_FLAG_MASK, val >> 7);
  setflag(Z_FLAG_MASK, val == 0);
  cpu.regs.byteregs.f |= byParity[val];
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(Y_FLAG_MASK, getbit(5, val));
  return val;
}

uint8_t cb_rr(uint8_t val)
{
  const bool c = getflag(C_FLAG_MASK);
  setflag(C_FLAG_MASK,  val & 1);
  val = (val >> 1) | (c << 7);

  // S, Z, Y, H=0, X, P N=0, C
  cpu.regs.byteregs.f = cpu.regs.byteregs.f & C_FLAG_MASK;

  setflag(S_FLAG_MASK, val >> 7);
  setflag(Z_FLAG_MASK, val == 0);
  cpu.regs.byteregs.f |= byParity[val];
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(Y_FLAG_MASK, getbit(5, val));
  return val;
}

uint8_t cb_sla(uint8_t val)
{
  setflag(C_FLAG_MASK, val >> 7);
  val <<= 1;

  // S, Z, Y, H=0, X, P N=0, C
  cpu.regs.byteregs.f = cpu.regs.byteregs.f & C_FLAG_MASK;

  setflag(S_FLAG_MASK, val >> 7);
  setflag(Z_FLAG_MASK, val == 0);
  cpu.regs.byteregs.f |= byParity[val];
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(Y_FLAG_MASK, getbit(5, val));
  return val;
}

uint8_t cb_sll(uint8_t val)
{
  setflag(C_FLAG_MASK, val >> 7);

  val <<= 1;
  val |= 1;
  
  // S, Z, Y, H=0, X, P N=0, C
  cpu.regs.byteregs.f = cpu.regs.byteregs.f & C_FLAG_MASK;

  setflag(S_FLAG_MASK, val >> 7);
  setflag(Z_FLAG_MASK, val == 0);
  cpu.regs.byteregs.f |= byParity[val];
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(Y_FLAG_MASK, getbit(5, val));
  return val;
}

uint8_t cb_sra(uint8_t val)
{
  setflag(C_FLAG_MASK, val & 1);
  val = (val >> 1) | (val & 0x80);

  // S, Z, Y, H=0, X, P N=0, C
  cpu.regs.byteregs.f = cpu.regs.byteregs.f & C_FLAG_MASK;

  setflag(S_FLAG_MASK, val >> 7);
  setflag(Z_FLAG_MASK, val == 0);
  cpu.regs.byteregs.f |= byParity[val];
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(Y_FLAG_MASK, getbit(5, val));
  return val;
}

uint8_t cb_srl(uint8_t val)
{
  setflag(C_FLAG_MASK, val & 1);
  val >>= 1;

  // S, Z, Y, H=0, X, P N=0, C
  cpu.regs.byteregs.f = cpu.regs.byteregs.f & C_FLAG_MASK;

  setflag(S_FLAG_MASK, val >> 7);
  setflag(Z_FLAG_MASK, val == 0);
  cpu.regs.byteregs.f |= byParity[val];
  setflag(X_FLAG_MASK, getbit(3, val));
  setflag(Y_FLAG_MASK, getbit(5, val));
  return val;
}

/////////////////////////////////////////////////

uint8_t addb(uint8_t a, uint8_t b, bool cy)
{
  uint32_t result32 = a + b + cy;
  int32_t  carry32  = result32 ^ a ^ b;
  uint8_t  result   = result32;

  cpu.regs.byteregs.f = 0;

  if (result >> 7)
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }

  if (result == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }

  if (carry32 & (1 << 4))
  {
    cpu.regs.byteregs.f |= H_FLAG_MASK;
  }

  if ((bool)(carry32 & (1 << 7)) != (bool)(carry32 & (1 << 8)))
  {
    cpu.regs.byteregs.f |= P_FLAG_MASK;
  }

  if (carry32 & (1 << 8))
  {
    cpu.regs.byteregs.f |= C_FLAG_MASK;
  }

  // N = 0;

  if (getbit(3, result))
  {
    cpu.regs.byteregs.f |= X_FLAG_MASK;
  }

  if (getbit(5, result))
  {
    cpu.regs.byteregs.f |= Y_FLAG_MASK;
  }

  return result;
}

uint16_t addw(uint16_t a, uint16_t b, bool cy)
{
  uint8_t  lsb = addb((byte)a, (byte)b, cy);
  uint8_t  msb = addb(a >> 8, b >> 8, getflag(C_FLAG_MASK));
  uint16_t result = (msb << 8) | lsb;

  setflag(Z_FLAG_MASK, result == 0);

  return result;
}

uint8_t subb(uint8_t a, uint8_t b, bool cy)
{
  uint8_t val = addb(a, ~b, !cy);

  // C is set if borrow; otherwise, it is reset; (c = !c)
  if (cpu.regs.byteregs.f & C_FLAG_MASK)
  {
    cpu.regs.byteregs.f &= ~C_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f |= C_FLAG_MASK;
  }

  // H is set if borrow from bit 4; otherwise, it is reset (h = !h)
  if (cpu.regs.byteregs.f & H_FLAG_MASK)
  {
    cpu.regs.byteregs.f &= ~H_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f |= H_FLAG_MASK;
  }
 
  // N is set;
  cpu.regs.byteregs.f |= N_FLAG_MASK;

  return val;
}

uint16_t subw(uint16_t a, uint16_t b, bool cy)
{
  uint8_t  lsb = subb((byte)a, (byte)b, cy);
  uint8_t  msb = subb(a >> 8, b >> 8, getflag(C_FLAG_MASK));
  uint16_t result = (msb << 8) | lsb;

  // Z is set if result is 0; otherwise, it is reset.
  if (result == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~Z_FLAG_MASK;
  }

  return result;
}

void sbchl(uint16_t val)
{
  uint16_t result;

  if (cpu.regs.byteregs.f & C_FLAG_MASK)
  {
    result = subw(cpu.regs.wordregs.hl, val, 1);
  }
  else
  {
    result = subw(cpu.regs.wordregs.hl, val, 0);
  }

  // S is set if result is negative; otherwise, it is reset.
  if (result & 0x8000)
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~S_FLAG_MASK;
  }

  // Z is set if result is 0; otherwise, it is reset.
  if (result == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~Z_FLAG_MASK;
  }

  cpu.regs.wordregs.hl = result;
}

void addhl(uint16_t val)
{
  bool sf = getflag(S_FLAG_MASK);
  bool zf = getflag(Z_FLAG_MASK);
  bool pf = getflag(P_FLAG_MASK);
  uint16_t result = addw(get_hl(), val, 0);
  set_hl(result);
  setflag(S_FLAG_MASK, sf);
  setflag(Z_FLAG_MASK, zf);
  setflag(P_FLAG_MASK, pf);
}

void addix(uint16_t val)
{
  bool sf = getflag(S_FLAG_MASK);
  bool zf = getflag(Z_FLAG_MASK);
  bool pf = getflag(P_FLAG_MASK);
  uint16_t result = addw(get_ix(), val, 0);
  set_ix(result);
  setflag(S_FLAG_MASK, sf);
  setflag(Z_FLAG_MASK, zf);
  setflag(P_FLAG_MASK, pf);
}

void addiy(uint16_t val)
{
  bool sf = getflag(S_FLAG_MASK);
  bool zf = getflag(Z_FLAG_MASK);
  bool pf = getflag(P_FLAG_MASK);
  uint16_t result = addw(get_iy(), val, 0);
  set_iy(result);
  setflag(S_FLAG_MASK, sf);
  setflag(Z_FLAG_MASK, zf);
  setflag(P_FLAG_MASK, pf);
}

uint8_t inc(uint8_t a)
{
  bool cf = getflag(C_FLAG_MASK);
  uint8_t result = addb(a, 1, 0);
  setflag(C_FLAG_MASK, cf);
  return result;
}

uint8_t dec(uint8_t a)
{
  bool cf = getflag(C_FLAG_MASK);
  uint8_t result = subb(a, 1, 0);
  setflag(C_FLAG_MASK, cf);
  return result;
}

/////////////////////////////////////////////////

void _UNDEF()
{
}

void _NOP()
{
}

void _LD_B_B()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.b;
}

void _LD_B_C()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.c;
}

void _LD_B_D()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.d;
}

void _LD_B_E()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.e;
}

void _LD_B_H()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.h;
}

void _LD_B_L()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.l;
}

void _LD_B_A()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.a;
}

void _LD_C_B()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.b;
}

void _LD_C_C()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.c;
}

void _LD_C_D()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.d;
}

void _LD_C_E()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.e;
}

void _LD_C_H()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.h;
}

void _LD_C_L()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.l;
}

void _LD_C_A()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.a;
}

void _LD_D_B()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.b;
}

void _LD_D_C()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.c;
}

void _LD_D_D()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.d;
}

void _LD_D_E()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.e;
}

void _LD_D_H()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.h;
}

void _LD_D_L()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.l;
}

void _LD_D_A()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.a;
}

void _LD_E_B()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.b;
}

void _LD_E_C()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.c;
}

void _LD_E_D()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.d;
}

void _LD_E_E()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.e;
}

void _LD_E_H()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.h;
}

void _LD_E_L()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.l;
}

void _LD_E_A()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.a;
}

void _LD_H_B()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.b;
}

void _LD_H_C()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.c;
}

void _LD_H_D()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.d;
}

void _LD_H_E()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.e;
}

void _LD_H_H()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.h;
}

void _LD_H_L()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.l;
}

void _LD_H_A()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.a;
}

void _LD_L_B()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.b;
}

void _LD_L_C()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.c;
}

void _LD_L_D()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.d;
}

void _LD_L_E()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.e;
}

void _LD_L_H()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.h;
}

void _LD_L_L()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.l;
}

void _LD_L_A()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.a;
}

void _LD_A_B()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.b;
}

void _LD_A_C()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.c;
}

void _LD_A_D()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.d;
}

void _LD_A_E()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.e;
}

void _LD_A_H()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.h;
}

void _LD_A_L()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.l;
}

void _LD_A_A()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.a;
}

void _LD_B_n()
{
  cpu.regs.byteregs.b = GetMem(cpu.pc+1);
}

void _LD_C_n()
{
  cpu.regs.byteregs.c = GetMem(cpu.pc+1);
}

void _LD_D_n()
{
  cpu.regs.byteregs.d = GetMem(cpu.pc+1);
}

void _LD_E_n()
{
  cpu.regs.byteregs.e = GetMem(cpu.pc+1);
}

void _LD_H_n()
{
  cpu.regs.byteregs.h = GetMem(cpu.pc+1);
}

void _LD_L_n()
{
  cpu.regs.byteregs.l = GetMem(cpu.pc+1);
}

void _LD_A_n()
{
  cpu.regs.byteregs.a = GetMem(cpu.pc+1);
}

void _LD_B_At_HL()
{
  cpu.regs.byteregs.b = ReadMem(cpu.regs.wordregs.hl);
}

void _LD_C_At_HL()
{
  cpu.regs.byteregs.c = ReadMem(cpu.regs.wordregs.hl);
}

void _LD_D_At_HL()
{
  cpu.regs.byteregs.d = ReadMem(cpu.regs.wordregs.hl);
}

void _LD_E_At_HL()
{
  cpu.regs.byteregs.e = ReadMem(cpu.regs.wordregs.hl);
}

void _LD_H_At_HL()
{
  cpu.regs.byteregs.h = ReadMem(cpu.regs.wordregs.hl);
}

void _LD_L_At_HL()
{
  cpu.regs.byteregs.l = ReadMem(cpu.regs.wordregs.hl);
}

void _LD_A_At_HL()
{
  cpu.regs.byteregs.a = ReadMem(cpu.regs.wordregs.hl);
}

void _LD_At_HL_B()
{
  WriteMem(cpu.regs.wordregs.hl, cpu.regs.byteregs.b);
}

void _LD_At_HL_C()
{
  WriteMem(cpu.regs.wordregs.hl, cpu.regs.byteregs.c);
}

void _LD_At_HL_D()
{
  WriteMem(cpu.regs.wordregs.hl, cpu.regs.byteregs.d);
}

void _LD_At_HL_E()
{
  WriteMem(cpu.regs.wordregs.hl, cpu.regs.byteregs.e);
}

void _LD_At_HL_H()
{
  WriteMem(cpu.regs.wordregs.hl, cpu.regs.byteregs.h);
}

void _LD_At_HL_L()
{
  WriteMem(cpu.regs.wordregs.hl, cpu.regs.byteregs.l);
}

void _LD_At_HL_A()
{
  WriteMem(cpu.regs.wordregs.hl, cpu.regs.byteregs.a);
}

void _LD_At_HL_n()
{
  WriteMem(cpu.regs.wordregs.hl, GetMem(cpu.pc+1));
}

void _LD_A_At_BC()
{
  cpu.regs.byteregs.a = ReadMem(cpu.regs.wordregs.bc);
}

void _LD_A_At_DE()
{
  cpu.regs.byteregs.a = ReadMem(cpu.regs.wordregs.de);
}

void _LD_A_At_nn()
{
  int n = GetMem(cpu.pc+1) + (GetMem(cpu.pc+2) << 8);

  cpu.regs.byteregs.a = ReadMem(n);
}

void _LD_At_BC_A()
{
  WriteMem(cpu.regs.wordregs.bc, cpu.regs.byteregs.a);
}

void _LD_At_DE_A()
{
  WriteMem(cpu.regs.wordregs.de, cpu.regs.byteregs.a);
}

void _LD_At_nn_A()
{
  int n = GetMem(cpu.pc+1) + (GetMem(cpu.pc+2) << 8);

  WriteMem(n, cpu.regs.byteregs.a);
}

void _LD_BC_nn()
{
  cpu.regs.byteregs.b = GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = GetMem(cpu.pc+1);
}

void _LD_DE_nn()
{
  cpu.regs.byteregs.d = GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = GetMem(cpu.pc+1);
}

void _LD_HL_nn()
{
  cpu.regs.byteregs.h = GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = GetMem(cpu.pc+1);
}

void _LD_SP_nn()
{
  cpu.sp = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
}

void _LD_HL_At_nn()
{
  int n = GetMem(cpu.pc+1) + (GetMem(cpu.pc+2) << 8);
  cpu.regs.byteregs.h = ReadMem(n+1);
  cpu.regs.byteregs.l = ReadMem(n);
}

void _LD_At_nn_HL()
{
  int n = GetMem(cpu.pc+1) + (GetMem(cpu.pc+2) << 8);
  WriteMem(n+1, cpu.regs.byteregs.h);
  WriteMem(n, cpu.regs.byteregs.l);
}

void _LD_SP_HL()
{
  cpu.sp = cpu.regs.wordregs.hl;
}

void _PUSH_BC()
{
  PutMem(cpu.sp-1, cpu.regs.byteregs.b);
  PutMem(cpu.sp-2, cpu.regs.byteregs.c);
  cpu.sp -= 2;
}

void _PUSH_DE()
{
  PutMem(cpu.sp-1, cpu.regs.byteregs.d);
  PutMem(cpu.sp-2, cpu.regs.byteregs.e);
  cpu.sp -= 2;
}

void _PUSH_HL()
{
  PutMem(cpu.sp-1, cpu.regs.byteregs.h);
  PutMem(cpu.sp-2, cpu.regs.byteregs.l);
  cpu.sp -= 2;
}

void _PUSH_AF()
{
  PutMem(cpu.sp-1, cpu.regs.byteregs.a);
  PutMem(cpu.sp-2, cpu.regs.byteregs.f);
  cpu.sp -= 2;
}

void _POP_BC()
{
  cpu.regs.byteregs.b = GetMem(cpu.sp+1);
  cpu.regs.byteregs.c = GetMem(cpu.sp);
  cpu.sp += 2;
}

void _POP_DE()
{
  cpu.regs.byteregs.d = GetMem(cpu.sp+1);
  cpu.regs.byteregs.e = GetMem(cpu.sp);
  cpu.sp += 2;
}

void _POP_HL()
{
  cpu.regs.byteregs.h = GetMem(cpu.sp+1);
  cpu.regs.byteregs.l = GetMem(cpu.sp);
  cpu.sp += 2;
}

void _POP_AF()
{
  cpu.regs.byteregs.a = GetMem(cpu.sp+1);
  cpu.regs.byteregs.f = GetMem(cpu.sp);
  cpu.sp += 2;
}

void _EX_DE_HL()
{
  unsigned short x = cpu.regs.wordregs.de;
  cpu.regs.wordregs.de = cpu.regs.wordregs.hl;
  cpu.regs.wordregs.hl = x;
}

void _EX_AF_Alt_AF()
{
  unsigned short x = cpu.regs.wordregs.af;
  cpu.regs.wordregs.af = cpu.alt_regs.wordregs.af;
  cpu.alt_regs.wordregs.af = x;
}

void _EXX()
{
  unsigned short x = cpu.regs.wordregs.bc;
  cpu.regs.wordregs.bc = cpu.alt_regs.wordregs.bc;
  cpu.alt_regs.wordregs.bc = x;

  x = cpu.regs.wordregs.de;
  cpu.regs.wordregs.de = cpu.alt_regs.wordregs.de;
  cpu.alt_regs.wordregs.de = x;

  x = cpu.regs.wordregs.hl;
  cpu.regs.wordregs.hl = cpu.alt_regs.wordregs.hl;
  cpu.alt_regs.wordregs.hl = x;
}

void _EX_At_SP_HL()
{
  byte x = cpu.regs.byteregs.h;
  cpu.regs.byteregs.h = GetMem(cpu.sp+1);
  PutMem(cpu.sp+1, x);

  x = cpu.regs.byteregs.l;
  cpu.regs.byteregs.l = GetMem(cpu.sp);
  PutMem(cpu.sp, x);
}

void _ADD_A_B()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.b, 0);
}

void _ADD_A_C()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.c, 0);
}

void _ADD_A_D()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.d, 0);
}

void _ADD_A_E()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.e, 0);
}

void _ADD_A_H()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.h, 0);
}

void _ADD_A_L()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.l, 0);
}

void _ADD_A_At_HL()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, ReadMem(cpu.regs.wordregs.hl), 0);
}

void _ADD_A_A()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.a, 0);
}

void _ADD_A_n()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, GetMem(cpu.pc+1), 0);
}

void _ADC_A_B()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.b, getflag(C_FLAG_MASK));
}

void _ADC_A_C()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.c, getflag(C_FLAG_MASK));
}

void _ADC_A_D()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.d, getflag(C_FLAG_MASK));
}

void _ADC_A_E()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.e, getflag(C_FLAG_MASK));
}

void _ADC_A_H()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.h, getflag(C_FLAG_MASK));
}

void _ADC_A_L()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.l, getflag(C_FLAG_MASK));
}

void _ADC_A_A()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.a, getflag(C_FLAG_MASK));
}

void _ADC_A_n()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, GetMem(cpu.pc+1), getflag(C_FLAG_MASK));
}

void _ADC_A_At_HL()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, ReadMem(cpu.regs.wordregs.hl), getflag(C_FLAG_MASK));
}

void _SUB_A_B()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.b, 0);
}

void _SUB_A_C()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.c, 0);
}

void _SUB_A_D()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.d, 0);
}

void _SUB_A_E()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.e, 0);
}

void _SUB_A_H()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.h, 0);
}

void _SUB_A_L()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.l, 0);
}

void _SUB_A_A()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.a, 0);
}

void _SUB_A_n()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, GetMem(cpu.pc+1), 0);
}

void _SUB_A_At_HL()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, ReadMem(cpu.regs.wordregs.hl), 0);
}

void _SBC_A_B()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.b, getflag(C_FLAG_MASK));
}

void _SBC_A_C()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.c, getflag(C_FLAG_MASK));
}

void _SBC_A_D()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.d, getflag(C_FLAG_MASK));
}

void _SBC_A_E()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.e, getflag(C_FLAG_MASK));
}

void _SBC_A_H()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.h, getflag(C_FLAG_MASK));
}

void _SBC_A_L()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.l, getflag(C_FLAG_MASK));
}

void _SBC_A_A()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.a, getflag(C_FLAG_MASK));
}

void _SBC_A_n()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, GetMem(cpu.pc+1), getflag(C_FLAG_MASK));
}

void _SBC_A_At_HL()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, ReadMem(cpu.regs.wordregs.hl), getflag(C_FLAG_MASK));
}

void _AND_A_B()
{
  land(cpu.regs.byteregs.b);
}

void _AND_A_C()
{
  land(cpu.regs.byteregs.c);
}

void _AND_A_D()
{
  land(cpu.regs.byteregs.d);
}

void _AND_A_E()
{
  land(cpu.regs.byteregs.e);
}

void _AND_A_H()
{
  land(cpu.regs.byteregs.h);
}

void _AND_A_L()
{
  land(cpu.regs.byteregs.l);
}

void _AND_A_A()
{
  land(cpu.regs.byteregs.a);
}

void _AND_A_n()
{
  land(GetMem(cpu.pc+1));
}

void _AND_A_At_HL()
{
  land(ReadMem(cpu.regs.wordregs.hl));
}

void _OR_A_B()
{
  lor(cpu.regs.byteregs.b);
}

void _OR_A_C()
{
  lor(cpu.regs.byteregs.c);
}

void _OR_A_D()
{
  lor(cpu.regs.byteregs.d);
}

void _OR_A_E()
{
  lor(cpu.regs.byteregs.e);
}

void _OR_A_H()
{
  lor(cpu.regs.byteregs.h);
}

void _OR_A_L()
{
  lor(cpu.regs.byteregs.l);
}

void _OR_A_A()
{
  lor(cpu.regs.byteregs.a);
}

void _OR_A_n()
{
  lor(GetMem(cpu.pc+1));
}

void _OR_A_At_HL()
{
  lor(ReadMem(cpu.regs.wordregs.hl));
}

void _XOR_A_B()
{
  lxor(cpu.regs.byteregs.b);
}

void _XOR_A_C()
{
  lxor(cpu.regs.byteregs.c);
}

void _XOR_A_D()
{
  lxor(cpu.regs.byteregs.d);
}

void _XOR_A_E()
{
  lxor(cpu.regs.byteregs.e);
}

void _XOR_A_H()
{
  lxor(cpu.regs.byteregs.h);
}

void _XOR_A_L()
{
  lxor(cpu.regs.byteregs.l);
}

void _XOR_A_A()
{
  lxor(cpu.regs.byteregs.a);
}

void _XOR_A_n()
{
  lxor(GetMem(cpu.pc+1));
}

void _XOR_A_At_HL()
{
  lxor(ReadMem(cpu.regs.wordregs.hl));
}

void _CP_B()
{
  cp(cpu.regs.byteregs.b);
}

void _CP_C()
{
  cp(cpu.regs.byteregs.c);
}

void _CP_D()
{
  cp(cpu.regs.byteregs.d);
}

void _CP_E()
{
  cp(cpu.regs.byteregs.e);
}

void _CP_H()
{
  cp(cpu.regs.byteregs.h);
}

void _CP_L()
{
  cp(cpu.regs.byteregs.l);
}

void _CP_A()
{
  cp(cpu.regs.byteregs.a);
}

void _CP_n()
{
  cp(GetMem(cpu.pc+1));
}

void _CP_At_HL()
{
  cp(ReadMem(cpu.regs.wordregs.hl));
}

void _INC_B()
{
  cpu.regs.byteregs.b = inc(cpu.regs.byteregs.b);
}

void _INC_C()
{
  cpu.regs.byteregs.c = inc(cpu.regs.byteregs.c);
}

void _INC_D()
{
  cpu.regs.byteregs.d = inc(cpu.regs.byteregs.d);
}

void _INC_E()
{
  cpu.regs.byteregs.e = inc(cpu.regs.byteregs.e);
}

void _INC_H()
{
  cpu.regs.byteregs.h = inc(cpu.regs.byteregs.h);
}

void _INC_L()
{
  cpu.regs.byteregs.l = inc(cpu.regs.byteregs.l);
}

void _INC_A()
{
  cpu.regs.byteregs.a = inc(cpu.regs.byteregs.a);
}

void _INC_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = inc(p);
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _DEC_B()
{
  cpu.regs.byteregs.b = dec(cpu.regs.byteregs.b);
}

void _DEC_C()
{
  cpu.regs.byteregs.c = dec(cpu.regs.byteregs.c);
}

void _DEC_D()
{
  cpu.regs.byteregs.d = dec(cpu.regs.byteregs.d);
}

void _DEC_E()
{
  cpu.regs.byteregs.e = dec(cpu.regs.byteregs.e);
}

void _DEC_H()
{
  cpu.regs.byteregs.h = dec(cpu.regs.byteregs.h);
}

void _DEC_L()
{
  cpu.regs.byteregs.l = dec(cpu.regs.byteregs.l);
}

void _DEC_A()
{
  cpu.regs.byteregs.a = dec(cpu.regs.byteregs.a);
}

void _DEC_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = dec(p);
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _DAA()
{
  // "When this instruction is executed, the A register is BCD corrected
  // using the  contents of the flags. The exact process is the following:
  // if the least significant four bits of A contain a non-BCD digit
  // (i. e. it is greater than 9) or the H flag is set, then $06 is
  // added to the register. Then the four most significant bits are
  // checked. If this more significant digit also happens to be greater
  // than 9 or the C flag is set, then $60 is added."
  // > http://z80-heaven.wikidot.com/instructions-set:daa
  uint8_t correction = 0;

  if ((cpu.regs.byteregs.a & 0x0F) > 0x09 || getflag(H_FLAG_MASK))
  {
    correction += 0x06;
  }

  if (cpu.regs.byteregs.a > 0x99 || getflag(C_FLAG_MASK))
  {
    correction += 0x60;
    setflag(C_FLAG_MASK, 1);
  }

  if (getflag(N_FLAG_MASK))
  {
    setflag(H_FLAG_MASK, getflag(H_FLAG_MASK) && (cpu.regs.byteregs.a & 0x0F) < 0x06);
    cpu.regs.byteregs.a -= correction;
  }
  else
  {
    setflag(H_FLAG_MASK, (cpu.regs.byteregs.a & 0x0F) > 0x09);
    cpu.regs.byteregs.a += correction;
  }

  setflag(S_FLAG_MASK, cpu.regs.byteregs.a >> 7);
  setflag(Z_FLAG_MASK, cpu.regs.byteregs.a == 0);
  setflag(P_FLAG_MASK, byParity[cpu.regs.byteregs.a]);
  setflag(X_FLAG_MASK, getbit(3, cpu.regs.byteregs.a));
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.byteregs.a));
}

void _CPL()
{
  cpu.regs.byteregs.a = ~cpu.regs.byteregs.a;

  // S is not affected. 
  // Z is not affected.
  // H is set.
  cpu.regs.byteregs.f |= H_FLAG_MASK;

  // P/V is not affected. 
  // N is set. 
  cpu.regs.byteregs.f |= N_FLAG_MASK;

  // C is not affected.

  setflag(X_FLAG_MASK, getbit(3, cpu.regs.byteregs.a));
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.byteregs.a));
}

void _CCF()
{
  // S is not affected.
  // Z is not affected.
  // P/V is not affected.
  
  // N is reset.
  cpu.regs.byteregs.f &= ~N_FLAG_MASK;

  // H, previous carry is copied.
  // C is set if CY was 0 before operation; otherwise, it is reset.
  if ((cpu.regs.byteregs.f & C_FLAG_MASK) == 0)
  {
    cpu.regs.byteregs.f |= C_FLAG_MASK;
    cpu.regs.byteregs.f &= ~H_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~C_FLAG_MASK;
    cpu.regs.byteregs.f |= H_FLAG_MASK;
  }

  setflag(X_FLAG_MASK, getbit(3, cpu.regs.byteregs.a));
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.byteregs.a));
}

void _SCF()
{
  // S is not affected.
  // Z is not affected.
  // H is reset.
  cpu.regs.byteregs.f &= ~H_FLAG_MASK;

  // P/V is not affected.
  // N is reset.
  cpu.regs.byteregs.f &= ~N_FLAG_MASK;

  // C is set.
  cpu.regs.byteregs.f |= C_FLAG_MASK;

  // S is not affected.
  // Z is not affected.
  // P/V is not affected.

  setflag(X_FLAG_MASK, getbit(3, cpu.regs.byteregs.a));
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.byteregs.a));
}

void _HALT()
{
  cpu.halt = 1;
  cpu.cycles += 4;
  cpu.pc += 1;
  SignalHalt();
}

void _DI()
{
  cpu.iff1 = 0;
  cpu.iff2 = 0;
}

void _EI()
{
  cpu.iff1 = 1;
  cpu.iff2 = 1;
}

void _ADD_HL_BC()
{
  addhl(cpu.regs.wordregs.bc);
}

void _ADD_HL_DE()
{
  addhl(cpu.regs.wordregs.de);
}

void _ADD_HL_HL()
{
  addhl(cpu.regs.wordregs.hl);
}

void _ADD_HL_SP()
{
  addhl(cpu.sp);
}

void _INC_BC()
{
  ++cpu.regs.wordregs.bc;
}

void _INC_DE()
{
  ++cpu.regs.wordregs.de;
}

void _INC_HL()
{
  ++cpu.regs.wordregs.hl;
}

void _INC_SP()
{
  ++cpu.sp;
}

void _DEC_BC()
{
  --cpu.regs.wordregs.bc;
}

void _DEC_DE()
{
  --cpu.regs.wordregs.de;
}

void _DEC_HL()
{
  --cpu.regs.wordregs.hl;
}

void _DEC_SP()
{
  --cpu.sp;
}

void _RLCA()
{
  setflag(C_FLAG_MASK, cpu.regs.byteregs.a >> 7);

  cpu.regs.byteregs.a = (cpu.regs.byteregs.a << 1) | (byte)getflag(C_FLAG_MASK);

  // setflag(N_FLAG_MASK, 0);
  cpu.regs.byteregs.f &= ~N_FLAG_MASK;

  // setflag(H_FLAG_MASK, 0);
  cpu.regs.byteregs.f &= ~H_FLAG_MASK;

  setflag(X_FLAG_MASK, getbit(3, cpu.regs.byteregs.a));
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.byteregs.a));
}

void _RLA()
{
  byte cy = getflag(C_FLAG_MASK);
  setflag(C_FLAG_MASK, cpu.regs.byteregs.a >> 7);

  cpu.regs.byteregs.a = (cpu.regs.byteregs.a << 1) | cy;

  // setflag(N_FLAG_MASK, 0);
  cpu.regs.byteregs.f &= ~N_FLAG_MASK;

  // setflag(H_FLAG_MASK, 0);
  cpu.regs.byteregs.f &= ~H_FLAG_MASK;

  setflag(X_FLAG_MASK, getbit(3, cpu.regs.byteregs.a));
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.byteregs.a));
}

void _RRCA()
{
  setflag(C_FLAG_MASK, cpu.regs.byteregs.a & 1);

  cpu.regs.byteregs.a = ( cpu.regs.byteregs.a >> 1) | (getflag(C_FLAG_MASK) << 7);

  // setflag(N_FLAG_MASK, 0);
  cpu.regs.byteregs.f &= ~N_FLAG_MASK;

  // setflag(H_FLAG_MASK, 0);
  cpu.regs.byteregs.f &= ~H_FLAG_MASK;

  setflag(X_FLAG_MASK, getbit(3, cpu.regs.byteregs.a));
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.byteregs.a));
}

void _RRA()
{
  byte cy = getflag(C_FLAG_MASK);
  setflag(C_FLAG_MASK, cpu.regs.byteregs.a & 1);

  cpu.regs.byteregs.a = (cpu.regs.byteregs.a >> 1) | (cy << 7);

  // setflag(N_FLAG_MASK, 0);
  cpu.regs.byteregs.f &= ~N_FLAG_MASK;

  // setflag(H_FLAG_MASK, 0);
  cpu.regs.byteregs.f &= ~H_FLAG_MASK;

  setflag(X_FLAG_MASK, getbit(3, cpu.regs.byteregs.a));
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.byteregs.a));
}

void _JP_nn()
{
  cpu.pc = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
  cpu.cycles += 10;
}

void _JP_nz_nn()
{
  if (cpu.regs.byteregs.f & Z_FLAG_MASK)
  {
    cpu.pc += 3;
  }
  else
  {
    cpu.pc = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
  }

  cpu.cycles += 10;
}

void _JP_z_nn()
{
  if (cpu.regs.byteregs.f & Z_FLAG_MASK)
  {
    cpu.pc = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
  }
  else
  {
    cpu.pc += 3;
  }

  cpu.cycles += 10;
}

void _JP_nc_nn()
{
  if (cpu.regs.byteregs.f & C_FLAG_MASK)
  {
    cpu.pc += 3;
  }
  else
  {
    cpu.pc = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
  }

  cpu.cycles += 10;
}

void _JP_c_nn()
{
  if (cpu.regs.byteregs.f & C_FLAG_MASK)
  {
    cpu.pc = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
  }
  else
  {
    cpu.pc += 3;
  }

  cpu.cycles += 10;
}

void _JP_npv_nn()
{
  if (cpu.regs.byteregs.f & P_FLAG_MASK)
  {
    cpu.pc += 3;
  }
  else
  {
    cpu.pc = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
  }

  cpu.cycles += 10;
}

void _JP_pv_nn()
{
  if (cpu.regs.byteregs.f & P_FLAG_MASK)
  {
    cpu.pc = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
  }
  else
  {
    cpu.pc += 3;
  }

  cpu.cycles += 10;
}

void _JP_ns_nn()
{
  if (cpu.regs.byteregs.f & S_FLAG_MASK)
  {
    cpu.pc += 3;
  }
  else
  {
    cpu.pc = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
  }

  cpu.cycles += 10;
}

void _JP_s_nn()
{
  if (cpu.regs.byteregs.f & S_FLAG_MASK)
  {
    cpu.pc = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
  }
  else
  {
    cpu.pc += 3;
  }

  cpu.cycles += 10;
}

void _JR_e()
{
  cpu.pc = cpu.pc + 2 + (char)GetMem(cpu.pc+1);
  cpu.cycles += 12;
}

void _JR_C_e()
{
  if (cpu.regs.byteregs.f & C_FLAG_MASK)
  {
    cpu.pc = cpu.pc + (char)GetMem(cpu.pc+1);
    cpu.cycles += 12;
  }
  else
  {
    cpu.cycles += 7;
  }

  cpu.pc += 2;
}

void _JR_NC_e()
{
  if (cpu.regs.byteregs.f & C_FLAG_MASK)
  {
    cpu.cycles += 7;
  }
  else
  {
    cpu.pc = cpu.pc + (char)GetMem(cpu.pc+1);
    cpu.cycles += 12;
  }

  cpu.pc += 2;
}

void _JR_Z_e()
{
  if (cpu.regs.byteregs.f & Z_FLAG_MASK)
  {
    cpu.pc = cpu.pc + (char)GetMem(cpu.pc+1);
    cpu.cycles += 12;
  }
  else
  {
    cpu.cycles += 7;
  }

  cpu.pc += 2;
}

void _JR_NZ_e()
{
  if (cpu.regs.byteregs.f & Z_FLAG_MASK)
  {
    cpu.cycles += 7;
  }
  else
  {
    cpu.pc = cpu.pc + (char)GetMem(cpu.pc+1);
    cpu.cycles += 12;
  }

  cpu.pc += 2;
}

void _JP_At_HL()
{
  cpu.pc = cpu.regs.wordregs.hl;
  cpu.cycles += 4;
}

void _DJNZ_e()
{
  --cpu.regs.byteregs.b;

  if (cpu.regs.byteregs.b != 0)
  {
    cpu.pc = cpu.pc + (char)GetMem(cpu.pc+1);
    cpu.cycles += 13;
  }
  else
  {
    cpu.cycles += 8;
  }

  cpu.pc += 2;
}

void _CALL_nn()
{
  word pc = cpu.pc + 3;

  PutMem(cpu.sp-1, (pc >> 8) & 0xFF);
  PutMem(cpu.sp-2, pc & 0xFF);

  cpu.pc = (GetMem(cpu.pc+2) << 8) + GetMem(cpu.pc+1);
  cpu.sp -= 2;
  cpu.cycles += 17;
}

void _CALL_nz_nn()
{
  cpu.pc += 3;

  if (cpu.regs.byteregs.f & Z_FLAG_MASK)
  {
    cpu.cycles += 10;
  }
  else
  {
    PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
    PutMem(cpu.sp-2, cpu.pc & 0xFF);
    cpu.pc = (GetMem(cpu.pc-1) << 8) + GetMem(cpu.pc-2);
    cpu.sp -= 2;
    cpu.cycles += 17;
  }
}

void _CALL_z_nn()
{
  cpu.pc += 3;

  if (cpu.regs.byteregs.f & Z_FLAG_MASK)
  {
    PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
    PutMem(cpu.sp-2, cpu.pc & 0xFF);
    cpu.pc = (GetMem(cpu.pc-1) << 8) + GetMem(cpu.pc-2);
    cpu.sp -= 2;
    cpu.cycles += 17;
  }
  else
  {
    cpu.cycles += 10;
  }
}

void _CALL_nc_nn()
{
  cpu.pc += 3;

  if (cpu.regs.byteregs.f & C_FLAG_MASK)
  {
    cpu.cycles += 10;
  }
  else
  {
    PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
    PutMem(cpu.sp-2, cpu.pc & 0xFF);
    cpu.pc = (GetMem(cpu.pc-1) << 8) + GetMem(cpu.pc-2);
    cpu.sp -= 2;
    cpu.cycles += 17;
  }
}

void _CALL_c_nn()
{
  cpu.pc += 3;

  if (cpu.regs.byteregs.f & C_FLAG_MASK)
  {
    PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
    PutMem(cpu.sp-2, cpu.pc & 0xFF);
    cpu.pc = (GetMem(cpu.pc-1) << 8) + GetMem(cpu.pc-2);
    cpu.sp -= 2;
    cpu.cycles += 17;
  }
  else
  {
    cpu.cycles += 10;
  }
}

void _CALL_npv_nn()
{
  cpu.pc += 3;

  if (cpu.regs.byteregs.f & P_FLAG_MASK)
  {
    cpu.cycles += 10;
  }
  else
  {
    PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
    PutMem(cpu.sp-2, cpu.pc & 0xFF);
    cpu.pc = (GetMem(cpu.pc-1) << 8) + GetMem(cpu.pc-2);
    cpu.sp -= 2;
    cpu.cycles += 17;
  }
}

void _CALL_pv_nn()
{
  cpu.pc += 3;

  if (cpu.regs.byteregs.f & P_FLAG_MASK)
  {
    PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
    PutMem(cpu.sp-2, cpu.pc & 0xFF);
    cpu.pc = (GetMem(cpu.pc-1) << 8) + GetMem(cpu.pc-2);
    cpu.sp -= 2;
    cpu.cycles += 17;
  }
  else
  {
    cpu.cycles += 10;
  }
}

void _CALL_ns_nn()
{
  cpu.pc += 3;

  if (cpu.regs.byteregs.f & S_FLAG_MASK)
  {
    cpu.cycles += 10;
  }
  else
  {
    PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
    PutMem(cpu.sp-2, cpu.pc & 0xFF);
    cpu.pc = (GetMem(cpu.pc-1) << 8) + GetMem(cpu.pc-2);
    cpu.sp -= 2;
    cpu.cycles += 17;
  }
}

void _CALL_s_nn()
{
  cpu.pc += 3;

  if (cpu.regs.byteregs.f & S_FLAG_MASK)
  {
    PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
    PutMem(cpu.sp-2, cpu.pc & 0xFF);
    cpu.pc = (GetMem(cpu.pc-1) << 8) + GetMem(cpu.pc-2);
    cpu.sp -= 2;
    cpu.cycles += 17;
  }
  else
  {
    cpu.cycles += 10;
  }
}

void _RET()
{
  cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
  cpu.sp += 2;
  cpu.cycles += 10;
}

void _RET_nz()
{
  if ((cpu.regs.byteregs.f & Z_FLAG_MASK))
  {
    ++cpu.pc;
    cpu.cycles += 5;
  }
  else
  {
    cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
    cpu.sp += 2;
    cpu.cycles += 11;
  }
}

void _RET_z()
{
  if (cpu.regs.byteregs.f & Z_FLAG_MASK)
  {
    cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
    cpu.sp += 2;
    cpu.cycles += 11;
  }
  else
  {
    ++cpu.pc;
    cpu.cycles += 5;
  }
}

void _RET_nc()
{
  if (cpu.regs.byteregs.f & C_FLAG_MASK)
  {
    ++cpu.pc;
    cpu.cycles += 5;
  }
  else
  {
    cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
    cpu.sp += 2;
    cpu.cycles += 11;
  }
}

void _RET_c()
{
  if (cpu.regs.byteregs.f & C_FLAG_MASK)
  {
    cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
    cpu.sp += 2;
    cpu.cycles += 11;
  }
  else
  {
    ++cpu.pc;
    cpu.cycles += 5;
  }
}

void _RET_npv()
{
  if (cpu.regs.byteregs.f & P_FLAG_MASK)
  {
    ++cpu.pc;
    cpu.cycles += 5;
  }
  else
  {
    cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
    cpu.sp += 2;
    cpu.cycles += 11;
  }
}

void _RET_pv()
{
  if (cpu.regs.byteregs.f & P_FLAG_MASK)
  {
    cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
    cpu.sp += 2;
    cpu.cycles += 11;
  }
  else
  {
    ++cpu.pc;
    cpu.cycles += 5;
  }
}

void _RET_ns()
{
  if (cpu.regs.byteregs.f & S_FLAG_MASK)
  {
    ++cpu.pc;
    cpu.cycles += 5;
  }
  else
  {
    cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
    cpu.sp += 2;
    cpu.cycles += 11;
  }
}

void _RET_s()
{
  if (cpu.regs.byteregs.f & S_FLAG_MASK)
  {
    cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
    cpu.sp += 2;
    cpu.cycles += 11;
  }
  else
  {
    ++cpu.pc;
    cpu.cycles += 5;
  }
}

void _RST_00()
{
  ++cpu.pc;

  PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
  PutMem(cpu.sp-2, cpu.pc & 0xFF);

  cpu.pc = 0x0000;
  cpu.sp -= 2;
  cpu.cycles += 11;
}

void _RST_08()
{
  ++cpu.pc;

  PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
  PutMem(cpu.sp-2, cpu.pc & 0xFF);

  cpu.pc = 0x0008;
  cpu.sp -= 2;
  cpu.cycles += 11;
}

void _RST_10()
{
  ++cpu.pc;

  PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
  PutMem(cpu.sp-2, cpu.pc & 0xFF);

  cpu.pc = 0x0010;
  cpu.sp -= 2;
  cpu.cycles += 11;
}

void _RST_18()
{
  ++cpu.pc;

  PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
  PutMem(cpu.sp-2, cpu.pc & 0xFF);

  cpu.pc = 0x0018;
  cpu.sp -= 2;
  cpu.cycles += 11;
}

void _RST_20()
{
  ++cpu.pc;

  PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
  PutMem(cpu.sp-2, cpu.pc & 0xFF);

  cpu.pc = 0x0020;
  cpu.sp -= 2;
  cpu.cycles += 11;
}

void _RST_28()
{
  ++cpu.pc;

  PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
  PutMem(cpu.sp-2, cpu.pc & 0xFF);

  cpu.pc = 0x0028;
  cpu.sp -= 2;
  cpu.cycles += 11;
}

void _RST_30()
{
  ++cpu.pc;

  PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
  PutMem(cpu.sp-2, cpu.pc & 0xFF);

  cpu.pc = 0x0030;
  cpu.sp -= 2;
  cpu.cycles += 11;
}

void _RST_38()
{
  ++cpu.pc;

  PutMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
  PutMem(cpu.sp-2, cpu.pc & 0xFF);

  cpu.pc = 0x0038;
  cpu.sp -= 2;
  cpu.cycles += 11;
}

void _IN_A()
{
  int n = GetMem(cpu.pc+1);
  cpu.regs.byteregs.a = ReadPort(n);
}

void _OUT_n_A()
{
  WritePort(GetMem(cpu.pc+1), cpu.regs.byteregs.a);
}

void _RLC_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = cb_rlc(p);
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RL_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = cb_rl(p);
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RRC_B()
{
  cpu.regs.byteregs.b = cb_rrc(cpu.regs.byteregs.b);
}

void _RRC_C()
{
  cpu.regs.byteregs.c = cb_rrc(cpu.regs.byteregs.c);
}

void _RRC_D()
{
  cpu.regs.byteregs.d = cb_rrc(cpu.regs.byteregs.d);
}

void _RRC_E()
{
  cpu.regs.byteregs.e = cb_rrc(cpu.regs.byteregs.e);
}

void _RRC_H()
{
  cpu.regs.byteregs.h = cb_rrc(cpu.regs.byteregs.h);
}

void _RRC_L()
{
  cpu.regs.byteregs.l = cb_rrc(cpu.regs.byteregs.l);
}

void _RRC_A()
{
  cpu.regs.byteregs.a = cb_rrc(cpu.regs.byteregs.a);
}

void _RL_B()
{
  cpu.regs.byteregs.b = cb_rl(cpu.regs.byteregs.b);
}

void _RL_C()
{
  cpu.regs.byteregs.c = cb_rl(cpu.regs.byteregs.c);
}

void _RL_D()
{
  cpu.regs.byteregs.d = cb_rl(cpu.regs.byteregs.d);
}

void _RL_E()
{
  cpu.regs.byteregs.e = cb_rl(cpu.regs.byteregs.e);
}

void _RL_H()
{
  cpu.regs.byteregs.h = cb_rl(cpu.regs.byteregs.h);
}

void _RL_L()
{
  cpu.regs.byteregs.l = cb_rl(cpu.regs.byteregs.l);
}

void _RL_A()
{
  cpu.regs.byteregs.a = cb_rl(cpu.regs.byteregs.a);
}

void _RRC_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = cb_rrc(p);
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RR_B()
{
  cpu.regs.byteregs.b = cb_rr(cpu.regs.byteregs.b);
}

void _RR_C()
{
  cpu.regs.byteregs.c = cb_rr(cpu.regs.byteregs.c);
}

void _RR_D()
{
  cpu.regs.byteregs.d = cb_rr(cpu.regs.byteregs.d);
}

void _RR_E()
{
  cpu.regs.byteregs.e = cb_rr(cpu.regs.byteregs.e);
}

void _RR_H()
{
  cpu.regs.byteregs.h = cb_rr(cpu.regs.byteregs.h);
}

void _RR_L()
{
  cpu.regs.byteregs.l = cb_rr(cpu.regs.byteregs.l);
}

void _RR_A()
{
  cpu.regs.byteregs.a = cb_rr(cpu.regs.byteregs.a);
}

void _RR_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = cb_rr(p);
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SLA_B()
{
  cpu.regs.byteregs.b = cb_sla(cpu.regs.byteregs.b);
}

void _SLA_C()
{
  cpu.regs.byteregs.c = cb_sla(cpu.regs.byteregs.c);
}

void _SLA_D()
{
  cpu.regs.byteregs.d = cb_sla(cpu.regs.byteregs.d);
}

void _SLA_E()
{
  cpu.regs.byteregs.e = cb_sla(cpu.regs.byteregs.e);
}

void _SLA_H()
{
  cpu.regs.byteregs.h = cb_sla(cpu.regs.byteregs.h);
}

void _SLA_L()
{
  cpu.regs.byteregs.l = cb_sla(cpu.regs.byteregs.l);
}

void _SLA_A()
{
  cpu.regs.byteregs.a = cb_sla(cpu.regs.byteregs.a);
}

void _SLA_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = cb_sla(p);
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SLL_B()
{
  cpu.regs.byteregs.b = cb_sll(cpu.regs.byteregs.b);
}

void _SLL_C()
{
  cpu.regs.byteregs.c = cb_sll(cpu.regs.byteregs.c);
}

void _SLL_D()
{
  cpu.regs.byteregs.d = cb_sll(cpu.regs.byteregs.d);
}

void _SLL_E()
{
  cpu.regs.byteregs.e = cb_sll(cpu.regs.byteregs.e);
}

void _SLL_H()
{
  cpu.regs.byteregs.h = cb_sll(cpu.regs.byteregs.h);
}

void _SLL_L()
{
  cpu.regs.byteregs.l = cb_sll(cpu.regs.byteregs.l);
}

void _SLL_A()
{
  cpu.regs.byteregs.a = cb_sll(cpu.regs.byteregs.a);
}

void _SLL_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = cb_sll(p);
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SRA_B()
{
  cpu.regs.byteregs.b = cb_sra(cpu.regs.byteregs.b);
}

void _SRA_C()
{
  cpu.regs.byteregs.c = cb_sra(cpu.regs.byteregs.c);
}

void _SRA_D()
{
  cpu.regs.byteregs.d = cb_sra(cpu.regs.byteregs.d);
}

void _SRA_E()
{
  cpu.regs.byteregs.e = cb_sra(cpu.regs.byteregs.e);
}

void _SRA_H()
{
  cpu.regs.byteregs.h = cb_sra(cpu.regs.byteregs.h);
}

void _SRA_L()
{
  cpu.regs.byteregs.l = cb_sra(cpu.regs.byteregs.l);
}

void _SRA_A()
{
  cpu.regs.byteregs.a = cb_sra(cpu.regs.byteregs.a);
}

void _SRA_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = cb_sra(p);
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SRL_B()
{
  cpu.regs.byteregs.b = cb_srl(cpu.regs.byteregs.b);
}

void _SRL_C()
{
  cpu.regs.byteregs.c = cb_srl(cpu.regs.byteregs.c);
}

void _SRL_D()
{
  cpu.regs.byteregs.d = cb_srl(cpu.regs.byteregs.d);
}

void _SRL_E()
{
  cpu.regs.byteregs.e = cb_srl(cpu.regs.byteregs.e);
}

void _SRL_H()
{
  cpu.regs.byteregs.h = cb_srl(cpu.regs.byteregs.h);
}

void _SRL_L()
{
  cpu.regs.byteregs.l = cb_srl(cpu.regs.byteregs.l);
}

void _SRL_A()
{
  cpu.regs.byteregs.a = cb_srl(cpu.regs.byteregs.a);
}

void _SRL_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = cb_srl(p);
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _BIT_0_B()
{
  bit(cpu.regs.byteregs.b, 0x01);
}

void _BIT_0_C()
{
  bit(cpu.regs.byteregs.c, 0x01);
}

void _BIT_0_D()
{
  bit(cpu.regs.byteregs.d, 0x01);
}

void _BIT_0_E()
{
  bit(cpu.regs.byteregs.e, 0x01);
}

void _BIT_0_H()
{
  bit(cpu.regs.byteregs.h, 0x01);
}

void _BIT_0_L()
{
  bit(cpu.regs.byteregs.l, 0x01);
}

void _BIT_0_A()
{
  bit(cpu.regs.byteregs.a, 0x01);
}

void _BIT_1_B()
{
  bit(cpu.regs.byteregs.b, 0x02);
}

void _BIT_1_C()
{
  bit(cpu.regs.byteregs.c, 0x02);
}

void _BIT_1_D()
{
  bit(cpu.regs.byteregs.d, 0x02);
}

void _BIT_1_E()
{
  bit(cpu.regs.byteregs.e, 0x02);
}

void _BIT_1_H()
{
  bit(cpu.regs.byteregs.h, 0x02);
}

void _BIT_1_L()
{
  bit(cpu.regs.byteregs.l, 0x02);
}

void _BIT_1_A()
{
  bit(cpu.regs.byteregs.a, 0x02);
}

void _BIT_2_B()
{
  bit(cpu.regs.byteregs.b, 0x04);
}

void _BIT_2_C()
{
  bit(cpu.regs.byteregs.c, 0x04);
}

void _BIT_2_D()
{
  bit(cpu.regs.byteregs.d, 0x04);
}

void _BIT_2_E()
{
  bit(cpu.regs.byteregs.e, 0x04);
}

void _BIT_2_H()
{
  bit(cpu.regs.byteregs.h, 0x04);
}

void _BIT_2_L()
{
  bit(cpu.regs.byteregs.l, 0x04);
}

void _BIT_2_A()
{
  bit(cpu.regs.byteregs.a, 0x04);
}

void _BIT_3_B()
{
  bit(cpu.regs.byteregs.b, 0x08);
}

void _BIT_3_C()
{
  bit(cpu.regs.byteregs.c, 0x08);
}

void _BIT_3_D()
{
  bit(cpu.regs.byteregs.d, 0x08);
}

void _BIT_3_E()
{
  bit(cpu.regs.byteregs.e, 0x08);
}

void _BIT_3_H()
{
  bit(cpu.regs.byteregs.h, 0x08);
}

void _BIT_3_L()
{
  bit(cpu.regs.byteregs.l, 0x08);
}

void _BIT_3_A()
{
  bit(cpu.regs.byteregs.a, 0x08);
}

void _BIT_4_B()
{
  bit(cpu.regs.byteregs.b, 0x10);
}

void _BIT_4_C()
{
  bit(cpu.regs.byteregs.c, 0x10);
}

void _BIT_4_D()
{
  bit(cpu.regs.byteregs.d, 0x10);
}

void _BIT_4_E()
{
  bit(cpu.regs.byteregs.e, 0x10);
}

void _BIT_4_H()
{
  bit(cpu.regs.byteregs.h, 0x10);
}

void _BIT_4_L()
{
  bit(cpu.regs.byteregs.l, 0x10);
}

void _BIT_4_A()
{
  bit(cpu.regs.byteregs.a, 0x10);
}

void _BIT_5_B()
{
  bit(cpu.regs.byteregs.b, 0x20);
}

void _BIT_5_C()
{
  bit(cpu.regs.byteregs.c, 0x20);
}

void _BIT_5_D()
{
  bit(cpu.regs.byteregs.d, 0x20);
}

void _BIT_5_E()
{
  bit(cpu.regs.byteregs.e, 0x20);
}

void _BIT_5_H()
{
  bit(cpu.regs.byteregs.h, 0x20);
}

void _BIT_5_L()
{
  bit(cpu.regs.byteregs.l, 0x20);
}

void _BIT_5_A()
{
  bit(cpu.regs.byteregs.a, 0x20);
}

void _BIT_6_B()
{
  bit(cpu.regs.byteregs.b, 0x40);
}

void _BIT_6_C()
{
  bit(cpu.regs.byteregs.c, 0x40);
}

void _BIT_6_D()
{
  bit(cpu.regs.byteregs.d, 0x40);
}

void _BIT_6_E()
{
  bit(cpu.regs.byteregs.e, 0x40);
}

void _BIT_6_H()
{
  bit(cpu.regs.byteregs.h, 0x40);
}

void _BIT_6_L()
{
  bit(cpu.regs.byteregs.l, 0x40);
}

void _BIT_6_A()
{
  bit(cpu.regs.byteregs.a, 0x40);
}

void _BIT_7_B()
{
  bit(cpu.regs.byteregs.b, 0x80);
}

void _BIT_7_C()
{
  bit(cpu.regs.byteregs.c, 0x80);
}

void _BIT_7_D()
{
  bit(cpu.regs.byteregs.d, 0x80);
}

void _BIT_7_E()
{
  bit(cpu.regs.byteregs.e, 0x80);
}

void _BIT_7_H()
{
  bit(cpu.regs.byteregs.h, 0x80);
}

void _BIT_7_L()
{
  bit(cpu.regs.byteregs.l, 0x80);
}

void _BIT_7_A()
{
  bit(cpu.regs.byteregs.a, 0x80);
}

void _BIT_0_At_HL()
{
  bit(ReadMem(cpu.regs.wordregs.hl), 0x01);
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.wordregs.hl >> 8));
  setflag(X_FLAG_MASK, getbit(3, cpu.regs.wordregs.hl >> 8));
}

void _BIT_1_At_HL()
{
  bit(ReadMem(cpu.regs.wordregs.hl), 0x02);
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.wordregs.hl >> 8));
  setflag(X_FLAG_MASK, getbit(3, cpu.regs.wordregs.hl >> 8));
}

void _BIT_2_At_HL()
{
  bit(ReadMem(cpu.regs.wordregs.hl), 0x04);
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.wordregs.hl >> 8));
  setflag(X_FLAG_MASK, getbit(3, cpu.regs.wordregs.hl >> 8));
}

void _BIT_3_At_HL()
{
  bit(ReadMem(cpu.regs.wordregs.hl), 0x08);
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.wordregs.hl >> 8));
  setflag(X_FLAG_MASK, getbit(3, cpu.regs.wordregs.hl >> 8));
}

void _BIT_4_At_HL()
{
  bit(ReadMem(cpu.regs.wordregs.hl), 0x10);
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.wordregs.hl >> 8));
  setflag(X_FLAG_MASK, getbit(3, cpu.regs.wordregs.hl >> 8));
}

void _BIT_5_At_HL()
{
  bit(ReadMem(cpu.regs.wordregs.hl), 0x20);
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.wordregs.hl >> 8));
  setflag(X_FLAG_MASK, getbit(3, cpu.regs.wordregs.hl >> 8));
}

void _BIT_6_At_HL()
{
  bit(ReadMem(cpu.regs.wordregs.hl), 0x40);
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.wordregs.hl >> 8));
  setflag(X_FLAG_MASK, getbit(3, cpu.regs.wordregs.hl >> 8));
}

void _BIT_7_At_HL()
{
  bit(ReadMem(cpu.regs.wordregs.hl), 0x80);
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.wordregs.hl >> 8));
  setflag(X_FLAG_MASK, getbit(3, cpu.regs.wordregs.hl >> 8));
}

void _SET_0_B()
{
  cpu.regs.byteregs.b |= 0x01;
}

void _SET_0_C()
{
  cpu.regs.byteregs.c |= 0x01;
}

void _SET_0_D()
{
  cpu.regs.byteregs.d |= 0x01;
}

void _SET_0_E()
{
  cpu.regs.byteregs.e |= 0x01;
}

void _SET_0_H()
{
  cpu.regs.byteregs.h |= 0x01;
}

void _SET_0_L()
{
  cpu.regs.byteregs.l |= 0x01;
}

void _SET_0_A()
{
  cpu.regs.byteregs.a |= 0x01;
}

void _SET_1_B()
{
  cpu.regs.byteregs.b |= 0x02;
}

void _SET_1_C()
{
  cpu.regs.byteregs.c |= 0x02;
}

void _SET_1_D()
{
  cpu.regs.byteregs.d |= 0x02;
}

void _SET_1_E()
{
  cpu.regs.byteregs.e |= 0x02;
}

void _SET_1_H()
{
  cpu.regs.byteregs.h |= 0x02;
}

void _SET_1_L()
{
  cpu.regs.byteregs.l |= 0x02;
}

void _SET_1_A()
{
  cpu.regs.byteregs.a |= 0x02;
}

void _SET_2_B()
{
  cpu.regs.byteregs.b |= 0x04;
}

void _SET_2_C()
{
  cpu.regs.byteregs.c |= 0x04;
}

void _SET_2_D()
{
  cpu.regs.byteregs.d |= 0x04;
}

void _SET_2_E()
{
  cpu.regs.byteregs.e |= 0x04;
}

void _SET_2_H()
{
  cpu.regs.byteregs.h |= 0x04;
}

void _SET_2_L()
{
  cpu.regs.byteregs.l |= 0x04;
}

void _SET_2_A()
{
  cpu.regs.byteregs.a |= 0x04;
}

void _SET_3_B()
{
  cpu.regs.byteregs.b |= 0x08;
}

void _SET_3_C()
{
  cpu.regs.byteregs.c |= 0x08;
}

void _SET_3_D()
{
  cpu.regs.byteregs.d |= 0x08;
}

void _SET_3_E()
{
  cpu.regs.byteregs.e |= 0x08;
}

void _SET_3_H()
{
  cpu.regs.byteregs.h |= 0x08;
}

void _SET_3_L()
{
  cpu.regs.byteregs.l |= 0x08;
}

void _SET_3_A()
{
  cpu.regs.byteregs.a |= 0x08;
}

void _SET_4_B()
{
  cpu.regs.byteregs.b |= 0x10;
}

void _SET_4_C()
{
  cpu.regs.byteregs.c |= 0x10;
}

void _SET_4_D()
{
  cpu.regs.byteregs.d |= 0x10;
}

void _SET_4_E()
{
  cpu.regs.byteregs.e |= 0x10;
}

void _SET_4_H()
{
  cpu.regs.byteregs.h |= 0x10;
}

void _SET_4_L()
{
  cpu.regs.byteregs.l |= 0x10;
}

void _SET_4_A()
{
  cpu.regs.byteregs.a |= 0x10;
}

void _SET_5_B()
{
  cpu.regs.byteregs.b |= 0x20;
}

void _SET_5_C()
{
  cpu.regs.byteregs.c |= 0x20;
}

void _SET_5_D()
{
  cpu.regs.byteregs.d |= 0x20;
}

void _SET_5_E()
{
  cpu.regs.byteregs.e |= 0x20;
}

void _SET_5_H()
{
  cpu.regs.byteregs.h |= 0x20;
}

void _SET_5_L()
{
  cpu.regs.byteregs.l |= 0x20;
}

void _SET_5_A()
{
  cpu.regs.byteregs.a |= 0x20;
}

void _SET_6_B()
{
  cpu.regs.byteregs.b |= 0x40;
}

void _SET_6_C()
{
  cpu.regs.byteregs.c |= 0x40;
}

void _SET_6_D()
{
  cpu.regs.byteregs.d |= 0x40;
}

void _SET_6_E()
{
  cpu.regs.byteregs.e |= 0x40;
}

void _SET_6_H()
{
  cpu.regs.byteregs.h |= 0x40;
}

void _SET_6_L()
{
  cpu.regs.byteregs.l |= 0x40;
}

void _SET_6_A()
{
  cpu.regs.byteregs.a |= 0x40;
}

void _SET_7_B()
{
  cpu.regs.byteregs.b |= 0x80;
}

void _SET_7_C()
{
  cpu.regs.byteregs.c |= 0x80;
}

void _SET_7_D()
{
  cpu.regs.byteregs.d |= 0x80;
}

void _SET_7_E()
{
  cpu.regs.byteregs.e |= 0x80;
}

void _SET_7_H()
{
  cpu.regs.byteregs.h |= 0x80;
}

void _SET_7_L()
{
  cpu.regs.byteregs.l |= 0x80;
}

void _SET_7_A()
{
  cpu.regs.byteregs.a |= 0x80;
}

void _SET_0_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p | 0x01;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SET_1_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p | 0x02;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SET_2_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p | 0x04;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SET_3_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p | 0x08;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SET_4_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p | 0x10;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SET_5_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p | 0x20;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SET_6_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p | 0x40;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _SET_7_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p | 0x80;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RES_0_B()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.b & ~0x01;
}

void _RES_0_C()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.c & ~0x01;
}

void _RES_0_D()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.d & ~0x01;
}

void _RES_0_E()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.e & ~0x01;
}

void _RES_0_H()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.h & ~0x01;
}

void _RES_0_L()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.l & ~0x01;
}

void _RES_0_A()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.a & ~0x01;
}

void _RES_1_B()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.b & ~0x02;
}

void _RES_1_C()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.c & ~0x02;
}

void _RES_1_D()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.d & ~0x02;
}

void _RES_1_E()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.e & ~0x02;
}

void _RES_1_H()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.h & ~0x02;
}

void _RES_1_L()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.l & ~0x02;
}

void _RES_1_A()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.a & ~0x02;
}

void _RES_2_B()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.b & ~0x04;
}

void _RES_2_C()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.c & ~0x04;
}

void _RES_2_D()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.d & ~0x04;
}

void _RES_2_E()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.e & ~0x04;
}

void _RES_2_H()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.h & ~0x04;
}

void _RES_2_L()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.l & ~0x04;
}

void _RES_2_A()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.a & ~0x04;
}

void _RES_3_B()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.b & ~0x08;
}

void _RES_3_C()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.c & ~0x08;
}

void _RES_3_D()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.d & ~0x08;
}

void _RES_3_E()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.e & ~0x08;
}

void _RES_3_H()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.h & ~0x08;
}

void _RES_3_L()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.l & ~0x08;
}

void _RES_3_A()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.a & ~0x08;
}

void _RES_4_B()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.b & ~0x10;
}

void _RES_4_C()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.c & ~0x10;
}

void _RES_4_D()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.d & ~0x10;
}

void _RES_4_E()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.e & ~0x10;
}

void _RES_4_H()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.h & ~0x10;
}

void _RES_4_L()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.l & ~0x10;
}

void _RES_4_A()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.a & ~0x10;
}

void _RES_5_B()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.b & ~0x20;
}

void _RES_5_C()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.c & ~0x20;
}

void _RES_5_D()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.d & ~0x20;
}

void _RES_5_E()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.e & ~0x20;
}

void _RES_5_H()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.h & ~0x20;
}

void _RES_5_L()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.l & ~0x20;
}

void _RES_5_A()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.a & ~0x20;
}

void _RES_6_B()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.b & ~0x40;
}

void _RES_6_C()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.c & ~0x40;
}

void _RES_6_D()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.d & ~0x40;
}

void _RES_6_E()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.e & ~0x40;
}

void _RES_6_H()
{
  cpu.regs.byteregs.h = cpu.regs.byteregs.h & ~0x40;
}

void _RES_6_L()
{
  cpu.regs.byteregs.l = cpu.regs.byteregs.l & ~0x40;
}

void _RES_6_A()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.a & ~0x40;
}

void _RES_7_B()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.b & ~0x80;
}

void _RES_7_C()
{
  cpu.regs.byteregs.c &= ~0x80;
}

void _RES_7_D()
{
  cpu.regs.byteregs.d &= ~0x80;
}

void _RES_7_E()
{
  cpu.regs.byteregs.e &= ~0x80;
}

void _RES_7_H()
{
  cpu.regs.byteregs.h &= ~0x80;
}

void _RES_7_L()
{
  cpu.regs.byteregs.l &= ~0x80;
}

void _RES_7_A()
{
  cpu.regs.byteregs.a &= ~0x80;
}

void _RES_0_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p & ~0x01;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RES_1_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p & ~0x02;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RES_2_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p & ~0x04;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RES_3_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p & ~0x08;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RES_4_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p & ~0x10;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RES_5_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p & ~0x20;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RES_6_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p & ~0x40;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RES_7_At_HL()
{
  byte p = ReadMem(cpu.regs.wordregs.hl);
  p = p & ~0x80;
  WriteMem(cpu.regs.wordregs.hl, p);
}

void _RLC_B()
{
  cpu.regs.byteregs.b = cb_rlc(cpu.regs.byteregs.b);
}

void _RLC_C()
{
  cpu.regs.byteregs.c = cb_rlc(cpu.regs.byteregs.c);
}

void _RLC_D()
{
  cpu.regs.byteregs.d = cb_rlc(cpu.regs.byteregs.d);
}

void _RLC_E()
{
  cpu.regs.byteregs.e = cb_rlc(cpu.regs.byteregs.e);
}

void _RLC_H()
{
  cpu.regs.byteregs.h = cb_rlc(cpu.regs.byteregs.h);
}

void _RLC_L()
{
  cpu.regs.byteregs.l = cb_rlc(cpu.regs.byteregs.l);
}

void _RLC_A()
{
  cpu.regs.byteregs.a = cb_rlc(cpu.regs.byteregs.a);
}

/////////////////////////////////////////////////
InstType inst_CB[] = {
  {2, 8,  _RLC_B},       // 0x00
  {2, 8,  _RLC_C},       // 0x01
  {2, 8,  _RLC_D},       // 0x02
  {2, 8,  _RLC_E},       // 0x03
  {2, 8,  _RLC_H},       // 0x04
  {2, 8,  _RLC_L},       // 0x05
  {2, 15, _RLC_At_HL},   // 0x06
  {2, 8,  _RLC_A},       // 0x07
  {2, 8,  _RRC_B},       // 0x08
  {2, 8,  _RRC_C},       // 0x09
  {2, 8,  _RRC_D},       // 0x0A
  {2, 8,  _RRC_E},       // 0x0B
  {2, 8,  _RRC_H},       // 0x0C
  {2, 8,  _RRC_L},       // 0x0D
  {2, 15, _RRC_At_HL},   // 0x0E
  {2, 8,  _RRC_A},       // 0x0F
  {2, 8,  _RL_B},        // 0x10
  {2, 8,  _RL_C},        // 0x11
  {2, 8,  _RL_D},        // 0x12
  {2, 8,  _RL_E},        // 0x13
  {2, 8,  _RL_H},        // 0x14
  {2, 8,  _RL_L},        // 0x15
  {2, 15, _RL_At_HL},    // 0x16
  {2, 8,  _RL_A},        // 0x17
  {2, 8,  _RR_B},        // 0x18
  {2, 8,  _RR_C},        // 0x19
  {2, 8,  _RR_D},        // 0x1A
  {2, 8,  _RR_E},        // 0x1B
  {2, 8,  _RR_H},        // 0x1C
  {2, 8,  _RR_L},        // 0x1D
  {2, 15, _RR_At_HL},    // 0x1E
  {2, 8,  _RR_A},        // 0x1F
  {2, 8,  _SLA_B},       // 0x20
  {2, 8,  _SLA_C},       // 0x21
  {2, 8,  _SLA_D},       // 0x22
  {2, 8,  _SLA_E},       // 0x23
  {2, 8,  _SLA_H},       // 0x24
  {2, 8,  _SLA_L},       // 0x25
  {2, 15, _SLA_At_HL},   // 0x26
  {2, 8,  _SLA_A},       // 0x27
  {2, 8,  _SRA_B},       // 0x28
  {2, 8,  _SRA_C},       // 0x29
  {2, 8,  _SRA_D},       // 0x2A
  {2, 8,  _SRA_E},       // 0x2B
  {2, 8,  _SRA_H},       // 0x2C
  {2, 8,  _SRA_L},       // 0x2D
  {2, 15, _SRA_At_HL},   // 0x2E
  {2, 8,  _SRA_A},       // 0x2F
  {2, 8,  _SLL_B},       // 0x30 *
  {2, 8,  _SLL_C},       // 0x31 *
  {2, 8,  _SLL_D},       // 0x32 *
  {2, 8,  _SLL_E},       // 0x33 *
  {2, 8,  _SLL_H},       // 0x34 *
  {2, 8,  _SLL_L},       // 0x35 *
  {2, 15, _SLL_At_HL},   // 0x36 *
  {2, 8,  _SLL_A},       // 0x37 *
  {2, 8,  _SRL_B},       // 0x38
  {2, 8,  _SRL_C},       // 0x39
  {2, 8,  _SRL_D},       // 0x3A
  {2, 8,  _SRL_E},       // 0x3B
  {2, 8,  _SRL_H},       // 0x3C
  {2, 8,  _SRL_L},       // 0x3D
  {2, 15, _SRL_At_HL},   // 0x3E
  {2, 8,  _SRL_A},       // 0x3F
  {2, 8,  _BIT_0_B},     // 0x40
  {2, 8,  _BIT_0_C},     // 0x41
  {2, 8,  _BIT_0_D},     // 0x42
  {2, 8,  _BIT_0_E},     // 0x43
  {2, 8,  _BIT_0_H},     // 0x44
  {2, 8,  _BIT_0_L},     // 0x45
  {2, 15, _BIT_0_At_HL}, // 0x46
  {2, 8,  _BIT_0_A},     // 0x47
  {2, 8,  _BIT_1_B},     // 0x48
  {2, 8,  _BIT_1_C},     // 0x49
  {2, 8,  _BIT_1_D},     // 0x4A
  {2, 8,  _BIT_1_E},     // 0x4B
  {2, 8,  _BIT_1_H},     // 0x4C
  {2, 8,  _BIT_1_L},     // 0x4D
  {2, 15, _BIT_1_At_HL}, // 0x4E
  {2, 8,  _BIT_1_A},     // 0x4F
  {2, 8,  _BIT_2_B},     // 0x50
  {2, 8,  _BIT_2_C},     // 0x51
  {2, 8,  _BIT_2_D},     // 0x52
  {2, 8,  _BIT_2_E},     // 0x53
  {2, 8,  _BIT_2_H},     // 0x54
  {2, 8,  _BIT_2_L},     // 0x55
  {2, 15, _BIT_2_At_HL}, // 0x56
  {2, 8,  _BIT_2_A},     // 0x57
  {2, 8,  _BIT_3_B},     // 0x58
  {2, 8,  _BIT_3_C},     // 0x59
  {2, 8,  _BIT_3_D},     // 0x5A
  {2, 8,  _BIT_3_E},     // 0x5B
  {2, 8,  _BIT_3_H},     // 0x5C
  {2, 8,  _BIT_3_L},     // 0x5D
  {2, 15, _BIT_3_At_HL}, // 0x5E
  {2, 8,  _BIT_3_A},     // 0x5F
  {2, 8,  _BIT_4_B},     // 0x60
  {2, 8,  _BIT_4_C},     // 0x61
  {2, 8,  _BIT_4_D},     // 0x62
  {2, 8,  _BIT_4_E},     // 0x63
  {2, 8,  _BIT_4_H},     // 0x64
  {2, 8,  _BIT_4_L},     // 0x65
  {2, 15, _BIT_4_At_HL}, // 0x66
  {2, 8,  _BIT_4_A},     // 0x67
  {2, 8,  _BIT_5_B},     // 0x68
  {2, 8,  _BIT_5_C},     // 0x69
  {2, 8,  _BIT_5_D},     // 0x6A
  {2, 8,  _BIT_5_E},     // 0x6B
  {2, 8,  _BIT_5_H},     // 0x6C
  {2, 8,  _BIT_5_L},     // 0x6D
  {2, 15, _BIT_5_At_HL}, // 0x6E
  {2, 8,  _BIT_5_A},     // 0x6F
  {2, 8,  _BIT_6_B},     // 0x70
  {2, 8,  _BIT_6_C},     // 0x71
  {2, 8,  _BIT_6_D},     // 0x72
  {2, 8,  _BIT_6_E},     // 0x73
  {2, 8,  _BIT_6_H},     // 0x74
  {2, 8,  _BIT_6_L},     // 0x75
  {2, 15, _BIT_6_At_HL}, // 0x76
  {2, 8,  _BIT_6_A},     // 0x77
  {2, 8,  _BIT_7_B},     // 0x78
  {2, 8,  _BIT_7_C},     // 0x79
  {2, 8,  _BIT_7_D},     // 0x7A
  {2, 8,  _BIT_7_E},     // 0x7B
  {2, 8,  _BIT_7_H},     // 0x7C
  {2, 8,  _BIT_7_L},     // 0x7D
  {2, 15, _BIT_7_At_HL}, // 0x7E
  {2, 8,  _BIT_7_A},     // 0x7F
  {2, 8,  _RES_0_B},     // 0x80
  {2, 8,  _RES_0_C},     // 0x81
  {2, 8,  _RES_0_D},     // 0x82
  {2, 8,  _RES_0_E},     // 0x83
  {2, 8,  _RES_0_H},     // 0x84
  {2, 8,  _RES_0_L},     // 0x85
  {2, 15, _RES_0_At_HL}, // 0x86
  {2, 8,  _RES_0_A},     // 0x87
  {2, 8,  _RES_1_B},     // 0x88
  {2, 8,  _RES_1_C},     // 0x89
  {2, 8,  _RES_1_D},     // 0x8A
  {2, 8,  _RES_1_E},     // 0x8B
  {2, 8,  _RES_1_H},     // 0x8C
  {2, 8,  _RES_1_L},     // 0x8D
  {2, 15, _RES_1_At_HL}, // 0x8E
  {2, 8,  _RES_1_A},     // 0x8F
  {2, 8,  _RES_2_B},     // 0x90
  {2, 8,  _RES_2_C},     // 0x91
  {2, 8,  _RES_2_D},     // 0x92
  {2, 8,  _RES_2_E},     // 0x93
  {2, 8,  _RES_2_H},     // 0x94
  {2, 8,  _RES_2_L},     // 0x95
  {2, 15, _RES_2_At_HL}, // 0x96
  {2, 8,  _RES_2_A},     // 0x97
  {2, 8,  _RES_3_B},     // 0x98
  {2, 8,  _RES_3_C},     // 0x99
  {2, 8,  _RES_3_D},     // 0x9A
  {2, 8,  _RES_3_E},     // 0x9B
  {2, 8,  _RES_3_H},     // 0x9C
  {2, 8,  _RES_3_L},     // 0x9D
  {2, 15, _RES_3_At_HL}, // 0x9E
  {2, 8,  _RES_3_A},     // 0x9F
  {2, 8,  _RES_4_B},     // 0xA0
  {2, 8,  _RES_4_C},     // 0xA1
  {2, 8,  _RES_4_D},     // 0xA2
  {2, 8,  _RES_4_E},     // 0xA3
  {2, 8,  _RES_4_H},     // 0xA4
  {2, 8,  _RES_4_L},     // 0xA5
  {2, 15, _RES_4_At_HL}, // 0xA6
  {2, 8,  _RES_4_A},     // 0xA7
  {2, 8,  _RES_5_B},     // 0xA8
  {2, 8,  _RES_5_C},     // 0xA9
  {2, 8,  _RES_5_D},     // 0xAA
  {2, 8,  _RES_5_E},     // 0xAB
  {2, 8,  _RES_5_H},     // 0xAC
  {2, 8,  _RES_5_L},     // 0xAD
  {2, 15, _RES_5_At_HL}, // 0xAE
  {2, 8,  _RES_5_A},     // 0xAF
  {2, 8,  _RES_6_B},     // 0xB0
  {2, 8,  _RES_6_C},     // 0xB1
  {2, 8,  _RES_6_D},     // 0xB2
  {2, 8,  _RES_6_E},     // 0xB3
  {2, 8,  _RES_6_H},     // 0xB4
  {2, 8,  _RES_6_L},     // 0xB5
  {2, 15, _RES_6_At_HL}, // 0xB6
  {2, 8,  _RES_6_A},     // 0xB7
  {2, 8,  _RES_7_B},     // 0xB8
  {2, 8,  _RES_7_C},     // 0xB9
  {2, 8,  _RES_7_D},     // 0xBA
  {2, 8,  _RES_7_E},     // 0xBB
  {2, 8,  _RES_7_H},     // 0xBC
  {2, 8,  _RES_7_L},     // 0xBD
  {2, 15, _RES_7_At_HL}, // 0xBE
  {2, 8,  _RES_7_A},     // 0xBF
  {2, 8,  _SET_0_B},     // 0xC0
  {2, 8,  _SET_0_C},     // 0xC1
  {2, 8,  _SET_0_D},     // 0xC2
  {2, 8,  _SET_0_E},     // 0xC3
  {2, 8,  _SET_0_H},     // 0xC4
  {2, 8,  _SET_0_L},     // 0xC5
  {2, 15, _SET_0_At_HL}, // 0xC6
  {2, 8,  _SET_0_A},     // 0xC7
  {2, 8,  _SET_1_B},     // 0xC8
  {2, 8,  _SET_1_C},     // 0xC9
  {2, 8,  _SET_1_D},     // 0xCA
  {2, 8,  _SET_1_E},     // 0xCB
  {2, 8,  _SET_1_H},     // 0xCC
  {2, 8,  _SET_1_L},     // 0xCD
  {2, 15, _SET_1_At_HL}, // 0xCE
  {2, 8,  _SET_1_A},     // 0xCF
  {2, 8,  _SET_2_B},     // 0xD0
  {2, 8,  _SET_2_C},     // 0xD1
  {2, 8,  _SET_2_D},     // 0xD2
  {2, 8,  _SET_2_E},     // 0xD3
  {2, 8,  _SET_2_H},     // 0xD4
  {2, 8,  _SET_2_L},     // 0xD5
  {2, 15, _SET_2_At_HL}, // 0xD6
  {2, 8,  _SET_2_A},     // 0xD7
  {2, 8,  _SET_3_B},     // 0xD8
  {2, 8,  _SET_3_C},     // 0xD9
  {2, 8,  _SET_3_D},     // 0xDA
  {2, 8,  _SET_3_E},     // 0xDB
  {2, 8,  _SET_3_H},     // 0xDC
  {2, 8,  _SET_3_L},     // 0xDD
  {2, 15, _SET_3_At_HL}, // 0xDE
  {2, 8,  _SET_3_A},     // 0xDF
  {2, 8,  _SET_4_B},     // 0xE0
  {2, 8,  _SET_4_C},     // 0xE1
  {2, 8,  _SET_4_D},     // 0xE2
  {2, 8,  _SET_4_E},     // 0xE3
  {2, 8,  _SET_4_H},     // 0xE4
  {2, 8,  _SET_4_L},     // 0xE5
  {2, 15, _SET_4_At_HL}, // 0xE6
  {2, 8,  _SET_4_A},     // 0xE7
  {2, 8,  _SET_5_B},     // 0xE8
  {2, 8,  _SET_5_C},     // 0xE9
  {2, 8,  _SET_5_D},     // 0xEA
  {2, 8,  _SET_5_E},     // 0xEB
  {2, 8,  _SET_5_H},     // 0xEC
  {2, 8,  _SET_5_L},     // 0xED
  {2, 15, _SET_5_At_HL}, // 0xEE
  {2, 8,  _SET_5_A},     // 0xEF
  {2, 8,  _SET_6_B},     // 0xF0
  {2, 8,  _SET_6_C},     // 0xF1
  {2, 8,  _SET_6_D},     // 0xF2
  {2, 8,  _SET_6_E},     // 0xF3
  {2, 8,  _SET_6_H},     // 0xF4
  {2, 8,  _SET_6_L},     // 0xF5
  {2, 15, _SET_6_At_HL}, // 0xF6
  {2, 8,  _SET_6_A},     // 0xF7
  {2, 8,  _SET_7_B},     // 0xF8
  {2, 8,  _SET_7_C},     // 0xF9
  {2, 8,  _SET_7_D},     // 0xFA
  {2, 8,  _SET_7_E},     // 0xFB
  {2, 8,  _SET_7_H},     // 0xFC
  {2, 8,  _SET_7_L},     // 0xFD
  {2, 15, _SET_7_At_HL}, // 0xFE
  {2, 8,  _SET_7_A},     // 0xFF
};

void _OPCODE_CB()
{
  byte by = GetMem(cpu.pc+1);

  if (inst_CB[by].bytes == 0)
  {
    inst_CB[by].inst();
  }
  else
  {
    word pc = cpu.pc;
    inst_CB[by].inst();
    cpu.pc = pc + inst_CB[by].bytes;
    cpu.cycles += inst_CB[by].cycles;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void _LD_B_At_IX_Plus_d()
{
  cpu.regs.byteregs.b = ReadMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2));
}

void _LD_C_At_IX_Plus_d()
{
  cpu.regs.byteregs.c = ReadMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2));
}

void _LD_D_At_IX_Plus_d()
{
  cpu.regs.byteregs.d = ReadMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2));
}

void _LD_E_At_IX_Plus_d()
{
  cpu.regs.byteregs.e = ReadMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2));
}

void _LD_H_At_IX_Plus_d()
{
  cpu.regs.byteregs.h = ReadMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2));
}

void _LD_L_At_IX_Plus_d()
{
  cpu.regs.byteregs.l = ReadMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2));
}

void _LD_A_At_IX_Plus_d()
{
  cpu.regs.byteregs.a = ReadMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2));
}

void _LD_At_IX_Plus_d_B()
{
  WriteMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2), cpu.regs.byteregs.b);
}

void _LD_At_IX_Plus_d_C()
{
  WriteMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2), cpu.regs.byteregs.c);
}

void _LD_At_IX_Plus_d_D()
{
  WriteMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2), cpu.regs.byteregs.d);
}

void _LD_At_IX_Plus_d_E()
{
  WriteMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2), cpu.regs.byteregs.e);
}

void _LD_At_IX_Plus_d_H()
{
  WriteMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2), cpu.regs.byteregs.h);
}

void _LD_At_IX_Plus_d_L()
{
  WriteMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2), cpu.regs.byteregs.l);
}

void _LD_At_IX_Plus_d_A()
{
  WriteMem(cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2), cpu.regs.byteregs.a);
}

void _LD_At_IX_Plus_d_n()
{
  int a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  WriteMem(a, GetMem(cpu.pc+3));
}

void _LD_IX_nn()
{
  cpu.regs.byteregs.ixh = GetMem(cpu.pc+3);
  cpu.regs.byteregs.ixl = GetMem(cpu.pc+2);
}

void _LD_IX_At_nn()
{
  int n = GetMem(cpu.pc+2) + (GetMem(cpu.pc+3) << 8);
  cpu.regs.byteregs.ixh = ReadMem(n+1);
  cpu.regs.byteregs.ixl = ReadMem(n);
}

void _LD_At_nn_IX()
{
  int n = GetMem(cpu.pc+2) + (GetMem(cpu.pc+3) << 8);
  WriteMem(n+1, cpu.regs.byteregs.ixh);
  WriteMem(n, cpu.regs.byteregs.ixl);
}

void _LD_SP_IX()
{
  cpu.sp = cpu.regs.wordregs.ix;
}

void _PUSH_IX()
{
  PutMem(cpu.sp-1, cpu.regs.byteregs.ixh);
  PutMem(cpu.sp-2, cpu.regs.byteregs.ixl);
  cpu.sp -= 2;
}

void _POP_IX()
{
  cpu.regs.byteregs.ixh = GetMem(cpu.sp+1);
  cpu.regs.byteregs.ixl = GetMem(cpu.sp);
  cpu.sp += 2;
}

void _EX_At_SP_IX()
{
  byte x;

  x = cpu.regs.byteregs.ixh;
  cpu.regs.byteregs.ixh = GetMem(cpu.sp+1);
  PutMem(cpu.sp+1, x);

  x = cpu.regs.byteregs.ixl;
  cpu.regs.byteregs.ixl = GetMem(cpu.sp);
  PutMem(cpu.sp, x);
}

void _ADD_A_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, GetMem(a), 0);
}

void _ADC_A_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, GetMem(a), getflag(C_FLAG_MASK));
}

void _SUB_A_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, GetMem(a), 0);
}

void _SBC_A_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, GetMem(a), getflag(C_FLAG_MASK));
}

void _AND_A_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  land(GetMem(a));
}

void _OR_A_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  lor(GetMem(a));
}

void _XOR_A_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  lxor(GetMem(a));
}

void _CP_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cp(GetMem(a));
}

void _INC_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = inc(p);
  WriteMem(a, p);
}

void _DEC_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = dec(p);
  WriteMem(a, p);
}

void _ADD_IX_BC()
{
  addix(cpu.regs.wordregs.bc);
}

void _ADD_IX_DE()
{
  addix(cpu.regs.wordregs.de);
}

void _ADD_IX_IX()
{
  addix(cpu.regs.wordregs.ix);
}

void _ADD_IX_SP()
{
  addix(cpu.sp);
}

void _INC_IX()
{
  ++cpu.regs.wordregs.ix;
}

void _DEC_IX()
{
  --cpu.regs.wordregs.ix;
}

void _RLC_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_rlc(p);
  WriteMem(a, p);
}

void _RL_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_rl(p);
  WriteMem(a, p);
}

void _RRC_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_rrc(p);
  WriteMem(a, p);
}

void _RR_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_rr(p);
  WriteMem(a, p);
}

void _SLA_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_sla(p);
  WriteMem(a, p);
}

void _SLL_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_sll(p);
  WriteMem(a, p);
}

void _SRA_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_sra(p);
  WriteMem(a, p);
}

void _SRL_At_IX_Plus_d()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_srl(p);
  WriteMem(a, p);
}

void _BIT_b_At_IX_Plus_d()
{
  byte b = (GetMem(cpu.pc+3) >> 3) & 0x7;
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);

  cb_bit(p, b);
  setflag(X_FLAG_MASK, getbit(3, a >> 8));
  setflag(Y_FLAG_MASK, getbit(5, a >> 8));
}

void _SET_b_At_IX_Plus_d()
{
  byte mask[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
  byte b = (GetMem(cpu.pc+3) >> 3) & 0x7;
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = p | mask[b];
  WriteMem(a, p);
}

void _RES_b_At_IX_Plus_d()
{
  byte mask[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
  byte b = (GetMem(cpu.pc+3) >> 3) & 0x7;
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = p & ~mask[b];
  WriteMem(a, p);
}

void _JP_At_IX()
{
  cpu.pc = cpu.regs.wordregs.ix;
  cpu.cycles += 8;
}

void _RLC_At_IX_Plus_d_B()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rlc(GetMem(a));
}

void _RLC_At_IX_Plus_d_C()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_rlc(GetMem(a));
}

void _RLC_At_IX_Plus_d_D()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_rlc(GetMem(a));
}

void _RLC_At_IX_Plus_d_E()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_rlc(GetMem(a));
}

void _RLC_At_IX_Plus_d_H()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_rlc(GetMem(a));
}

void _RLC_At_IX_Plus_d_L()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_rlc(GetMem(a));
}

void _RLC_At_IX_Plus_d_A()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rlc(GetMem(a));
}

void _RRC_At_IX_Plus_d_B()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rrc(GetMem(a));
}

void _RRC_At_IX_Plus_d_C()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_rrc(GetMem(a));
}

void _RRC_At_IX_Plus_d_D()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_rrc(GetMem(a));
}

void _RRC_At_IX_Plus_d_E()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_rrc(GetMem(a));
}

void _RRC_At_IX_Plus_d_H()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_rrc(GetMem(a));
}

void _RRC_At_IX_Plus_d_L()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_rrc(GetMem(a));
}

void _RRC_At_IX_Plus_d_A()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rrc(GetMem(a));
}

void _RL_At_IX_Plus_d_B()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rl(GetMem(a));
}

void _RL_At_IX_Plus_d_C()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_rl(GetMem(a));
}

void _RL_At_IX_Plus_d_D()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_rl(GetMem(a));
}

void _RL_At_IX_Plus_d_E()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_rl(GetMem(a));
}

void _RL_At_IX_Plus_d_H()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_rl(GetMem(a));
}

void _RL_At_IX_Plus_d_L()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_rl(GetMem(a));
}

void _RL_At_IX_Plus_d_A()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rl(GetMem(a));
}

void _RR_At_IX_Plus_d_B()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rr(GetMem(a));
}

void _RR_At_IX_Plus_d_C()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_rr(GetMem(a));
}

void _RR_At_IX_Plus_d_D()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_rr(GetMem(a));
}

void _RR_At_IX_Plus_d_E()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_rr(GetMem(a));
}

void _RR_At_IX_Plus_d_H()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_rr(GetMem(a));
}

void _RR_At_IX_Plus_d_L()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_rr(GetMem(a));
}

void _RR_At_IX_Plus_d_A()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rr(GetMem(a));
}

void _SLA_At_IX_Plus_d_B()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sla(GetMem(a));
}

void _SLA_At_IX_Plus_d_C()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_sla(GetMem(a));
}

void _SLA_At_IX_Plus_d_D()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_sla(GetMem(a));
}

void _SLA_At_IX_Plus_d_E()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_sla(GetMem(a));
}

void _SLA_At_IX_Plus_d_H()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_sla(GetMem(a));
}

void _SLA_At_IX_Plus_d_L()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_sla(GetMem(a));
}

void _SLA_At_IX_Plus_d_A()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sla(GetMem(a));
}

void _SRA_At_IX_Plus_d_B()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sra(GetMem(a));
}

void _SRA_At_IX_Plus_d_C()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_sra(GetMem(a));
}

void _SRA_At_IX_Plus_d_D()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_sra(GetMem(a));
}

void _SRA_At_IX_Plus_d_E()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_sra(GetMem(a));
}

void _SRA_At_IX_Plus_d_H()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_sra(GetMem(a));
}

void _SRA_At_IX_Plus_d_L()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_sra(GetMem(a));
}

void _SRA_At_IX_Plus_d_A()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sra(GetMem(a));
}

void _SLL_At_IX_Plus_d_B()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sll(GetMem(a));
}

void _SLL_At_IX_Plus_d_C()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_sll(GetMem(a));
}

void _SLL_At_IX_Plus_d_D()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_sll(GetMem(a));
}

void _SLL_At_IX_Plus_d_E()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_sll(GetMem(a));
}

void _SLL_At_IX_Plus_d_H()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_sll(GetMem(a));
}

void _SLL_At_IX_Plus_d_L()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_sll(GetMem(a));
}

void _SLL_At_IX_Plus_d_A()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sll(GetMem(a));
}

void _SRL_At_IX_Plus_d_B()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_srl(GetMem(a));
}

void _SRL_At_IX_Plus_d_C()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_srl(GetMem(a));
}

void _SRL_At_IX_Plus_d_D()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_srl(GetMem(a));
}

void _SRL_At_IX_Plus_d_E()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_srl(GetMem(a));
}

void _SRL_At_IX_Plus_d_H()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_srl(GetMem(a));
}

void _SRL_At_IX_Plus_d_L()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_srl(GetMem(a));
}

void _SRL_At_IX_Plus_d_A()
{
  word a = cpu.regs.wordregs.ix + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_srl(GetMem(a));
}

/////////////////////////////////////////////////
InstType inst_DDCB[] = {
  {4, 23, _RLC_At_IX_Plus_d_B}, // 0x00 *
  {4, 23, _RLC_At_IX_Plus_d_C}, // 0x01 *
  {4, 23, _RLC_At_IX_Plus_d_D}, // 0x02 *
  {4, 23, _RLC_At_IX_Plus_d_E}, // 0x03 *
  {4, 23, _RLC_At_IX_Plus_d_H}, // 0x04 *
  {4, 23, _RLC_At_IX_Plus_d_L}, // 0x05 *
  {4, 23, _RLC_At_IX_Plus_d},   // 0x06
  {4, 23, _RLC_At_IX_Plus_d_A}, // 0x07 *
  {4, 23, _RRC_At_IX_Plus_d_B}, // 0x08 *
  {4, 23, _RRC_At_IX_Plus_d_C}, // 0x09 *
  {4, 23, _RRC_At_IX_Plus_d_D}, // 0x0A *
  {4, 23, _RRC_At_IX_Plus_d_E}, // 0x0B *
  {4, 23, _RRC_At_IX_Plus_d_H}, // 0x0C *
  {4, 23, _RRC_At_IX_Plus_d_L}, // 0x0D *
  {4, 23, _RRC_At_IX_Plus_d},   // 0x0E
  {4, 23, _RRC_At_IX_Plus_d_A}, // 0x0F *
  {4, 23, _RL_At_IX_Plus_d_B},  // 0x10 *
  {4, 23, _RL_At_IX_Plus_d_C},  // 0x11 *
  {4, 23, _RL_At_IX_Plus_d_D},  // 0x12 *
  {4, 23, _RL_At_IX_Plus_d_E},  // 0x13 *
  {4, 23, _RL_At_IX_Plus_d_H},  // 0x14 *
  {4, 23, _RL_At_IX_Plus_d_L},  // 0x15 *
  {4, 23, _RL_At_IX_Plus_d},    // 0x16
  {4, 23, _RL_At_IX_Plus_d_A},  // 0x17 *
  {4, 23, _RR_At_IX_Plus_d_B},  // 0x18 *
  {4, 23, _RR_At_IX_Plus_d_C},  // 0x19 *
  {4, 23, _RR_At_IX_Plus_d_D},  // 0x1A *
  {4, 23, _RR_At_IX_Plus_d_E},  // 0x1B *
  {4, 23, _RR_At_IX_Plus_d_H},  // 0x1C *
  {4, 23, _RR_At_IX_Plus_d_L},  // 0x1D *
  {4, 23, _RR_At_IX_Plus_d},    // 0x1E
  {4, 23, _RR_At_IX_Plus_d_A},  // 0x1F *
  {4, 23, _SLA_At_IX_Plus_d_B}, // 0x20 *
  {4, 23, _SLA_At_IX_Plus_d_C}, // 0x21 *
  {4, 23, _SLA_At_IX_Plus_d_D}, // 0x22 *
  {4, 23, _SLA_At_IX_Plus_d_E}, // 0x23 *
  {4, 23, _SLA_At_IX_Plus_d_H}, // 0x24 *
  {4, 23, _SLA_At_IX_Plus_d_L}, // 0x25 *
  {4, 23, _SLA_At_IX_Plus_d},   // 0x26
  {4, 23, _SLA_At_IX_Plus_d_A}, // 0x27 *
  {4, 23, _SRA_At_IX_Plus_d_B}, // 0x28 *
  {4, 23, _SRA_At_IX_Plus_d_C}, // 0x29 *
  {4, 23, _SRA_At_IX_Plus_d_D}, // 0x2A *
  {4, 23, _SRA_At_IX_Plus_d_E}, // 0x2B *
  {4, 23, _SRA_At_IX_Plus_d_H}, // 0x2C *
  {4, 23, _SRA_At_IX_Plus_d_L}, // 0x2D *
  {4, 23, _SRA_At_IX_Plus_d},   // 0x2E
  {4, 23, _SRA_At_IX_Plus_d_A}, // 0x2F *
  {4, 23, _SLL_At_IX_Plus_d_B}, // 0x30 *
  {4, 23, _SLL_At_IX_Plus_d_C}, // 0x31 *
  {4, 23, _SLL_At_IX_Plus_d_D}, // 0x32 *
  {4, 23, _SLL_At_IX_Plus_d_E}, // 0x33 *
  {4, 23, _SLL_At_IX_Plus_d_H}, // 0x34 *
  {4, 23, _SLL_At_IX_Plus_d_L}, // 0x35 *
  {4, 23, _SLL_At_IX_Plus_d},   // 0x36 *
  {4, 23, _SLL_At_IX_Plus_d_A}, // 0x37 *
  {4, 23, _SRL_At_IX_Plus_d_B}, // 0x38 *
  {4, 23, _SRL_At_IX_Plus_d_C}, // 0x39 *
  {4, 23, _SRL_At_IX_Plus_d_D}, // 0x3A *
  {4, 23, _SRL_At_IX_Plus_d_E}, // 0x3B *
  {4, 23, _SRL_At_IX_Plus_d_H}, // 0x3C *
  {4, 23, _SRL_At_IX_Plus_d_L}, // 0x3D *
  {4, 23, _SRL_At_IX_Plus_d},   // 0x3E
  {4, 23, _SRL_At_IX_Plus_d_A}, // 0x3F *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x40 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x41 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x42 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x43 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x44 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x45 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x46
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x47 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x48 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x49 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x4A *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x4B *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x4C *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x4D *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x4E
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x4F *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x50 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x51 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x52 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x53 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x54 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x55 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x56
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x57 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x58 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x59 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x5A *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x5B *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x5C *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x5D *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x5E
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x5F *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x60 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x61 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x62 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x63 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x64 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x65 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x66
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x67 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x68 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x69 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x6A *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x6B *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x6C *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x6D *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x6E
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x6F *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x70 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x71 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x72 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x73 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x74 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x75 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x76
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x77 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x78 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x79 *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x7A *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x7B *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x7C *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x7D *
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x7E
  {4, 20, _BIT_b_At_IX_Plus_d}, // 0x7F *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x80 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x81 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x82 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x83 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x84 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x85 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x86
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x87 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x88 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x89 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x8A *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x8B *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x8C *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x8D *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x8E
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x8F *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x90 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x91 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x92 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x93 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x94 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x95 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x96
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x97 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x98 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x99 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x9A *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x9B *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x9C *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x9D *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x9E
  {4, 23, _RES_b_At_IX_Plus_d}, // 0x9F *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xA0 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xA1 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xA2 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xA3 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xA4 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xA5 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xA6
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xA7 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xA8 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xA9 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xAA *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xAB *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xAC *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xAD *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xAE
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xAF *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xB0 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xB1 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xB2 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xB3 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xB4 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xB5 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xB6
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xB7 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xB8 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xB9 *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xBA *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xBB *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xBC *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xBD *
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xBE
  {4, 23, _RES_b_At_IX_Plus_d}, // 0xBF *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xC0 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xC1 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xC2 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xC3 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xC4 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xC5 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xC6
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xC7 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xC8 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xC9 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xCA *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xCB *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xCC *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xCD *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xCE
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xCF *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xD0 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xD1 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xD2 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xD3 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xD4 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xD5 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xD6
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xD7 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xD8 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xD9 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xDA *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xDB *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xDC *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xDD *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xDE
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xDF *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xE0 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xE1 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xE2 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xE3 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xE4 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xE5 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xE6
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xE7 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xE8 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xE9 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xEA *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xEB *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xEC *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xED *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xEE
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xEF *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xF0 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xF1 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xF2 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xF3 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xF4 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xF5 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xF6
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xF7 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xF8 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xF9 *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xFA *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xFB *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xFC *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xFD *
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xFE
  {4, 23, _SET_b_At_IX_Plus_d}, // 0xFF *
};

void _OPCODE_DD_CB()
{
  byte by = GetMem(cpu.pc+3);

  if (inst_DDCB[by].bytes == 0)
  {
    inst_DDCB[by].inst();
  }
  else
  {
    word pc = cpu.pc;
    inst_DDCB[by].inst();
    cpu.pc = pc + inst_DDCB[by].bytes;
    cpu.cycles += inst_DDCB[by].cycles;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void _ADD_A_XL()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.ixl, 0);
}

void _ADD_A_XH()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.ixh, 0);
}

void _ADC_A_XL()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.ixl, getflag(C_FLAG_MASK));
}

void _ADC_A_XH()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.ixh, getflag(C_FLAG_MASK));
}

void _SUB_A_XL()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.ixl, 0);
}

void _SUB_A_XH()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.ixh, 0);
}

void _SBC_A_XL()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.ixl, getflag(C_FLAG_MASK));
}

void _SBC_A_XH()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.ixh, getflag(C_FLAG_MASK));
}

void _AND_A_XL()
{
  land(cpu.regs.byteregs.ixl);
}

void _AND_A_XH()
{
  land(cpu.regs.byteregs.ixh);
}

void _XOR_A_XL()
{
  lxor(cpu.regs.byteregs.ixl);
}

void _XOR_A_XH()
{
  lxor(cpu.regs.byteregs.ixh);
}

void _OR_A_XL()
{
  lor(cpu.regs.byteregs.ixl);
}

void _OR_A_XH()
{
  lor(cpu.regs.byteregs.ixh);
}

void _CP_A_XL()
{
  cp(cpu.regs.byteregs.ixl);
}

void _CP_A_XH()
{
  cp(cpu.regs.byteregs.ixh);
}

void _LD_XL_B()
{
  cpu.regs.byteregs.ixl = cpu.regs.byteregs.b;
}

void _LD_XL_C()
{
  cpu.regs.byteregs.ixl = cpu.regs.byteregs.c;
}

void _LD_XL_D()
{
  cpu.regs.byteregs.ixl = cpu.regs.byteregs.d;
}

void _LD_XL_E()
{
  cpu.regs.byteregs.ixl = cpu.regs.byteregs.e;
}

void _LD_XL_H()
{
  cpu.regs.byteregs.ixl = cpu.regs.byteregs.h;
}

void _LD_XL_L()
{
  cpu.regs.byteregs.ixl = cpu.regs.byteregs.l;
}

void _LD_XL_A()
{
  cpu.regs.byteregs.ixl = cpu.regs.byteregs.a;
}

void _LD_XH_B()
{
  cpu.regs.byteregs.ixh = cpu.regs.byteregs.b;
}

void _LD_XH_C()
{
  cpu.regs.byteregs.ixh = cpu.regs.byteregs.c;
}

void _LD_XH_D()
{
  cpu.regs.byteregs.ixh = cpu.regs.byteregs.d;
}

void _LD_XH_E()
{
  cpu.regs.byteregs.ixh = cpu.regs.byteregs.e;
}

void _LD_XH_H()
{
  cpu.regs.byteregs.ixh = cpu.regs.byteregs.h;
}

void _LD_XH_L()
{
  cpu.regs.byteregs.ixh = cpu.regs.byteregs.l;
}

void _LD_XH_A()
{
  cpu.regs.byteregs.ixh = cpu.regs.byteregs.a;
}

void _INC_XH()
{
  cpu.regs.byteregs.ixh = inc(cpu.regs.byteregs.ixh);
}

void _INC_XL()
{
  cpu.regs.byteregs.ixl = inc(cpu.regs.byteregs.ixl);
}

void _DEC_XH()
{
  cpu.regs.byteregs.ixh = dec(cpu.regs.byteregs.ixh);
}

void _DEC_XL()
{
  cpu.regs.byteregs.ixl = dec(cpu.regs.byteregs.ixl);
}

void _LD_XH_n()
{
  cpu.regs.byteregs.ixh = GetMem(cpu.pc+2);
}

void _LD_XL_n()
{
  cpu.regs.byteregs.ixl = GetMem(cpu.pc+2);
}

void _LD_B_XH()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.ixh;
}

void _LD_B_XL()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.ixl;
}

void _LD_C_XH()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.ixh;
}

void _LD_C_XL()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.ixl;
}
void _LD_D_XH()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.ixh;
}

void _LD_D_XL()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.ixl;
}

void _LD_E_XH()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.ixh;
}

void _LD_E_XL()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.ixl;
}

void _LD_A_XH()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.ixh;
}

void _LD_A_XL()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.ixl;
}

/////////////////////////////////////////////////
InstType inst_DD[] = {
  {1, 4,  _UNDEF},              // 0x00
  {1, 4,  _UNDEF},              // 0x01
  {1, 4,  _UNDEF},              // 0x02
  {1, 4,  _UNDEF},              // 0x03
  {2, 8,  _INC_B},              // 0x04 *
  {2, 8,  _DEC_B},              // 0x05 *
  {1, 3,  _LD_B_n},             // 0x06 *
  {3, 11, _UNDEF},              // 0x07
  {1, 4,  _UNDEF},              // 0x08
  {2, 15, _ADD_IX_BC},          // 0x09
  {1, 4,  _UNDEF},              // 0x0A
  {1, 4,  _UNDEF},              // 0x0B
  {2, 8,  _INC_C},              // 0x0C *
  {2, 8,  _DEC_C},              // 0x0D *
  {3, 15, _LD_C_n},             // 0x0E *
  {1, 4,  _UNDEF},              // 0x0F
  {1, 4,  _UNDEF},              // 0x10
  {1, 4,  _UNDEF},              // 0x11
  {1, 4,  _UNDEF},              // 0x12
  {1, 4,  _UNDEF},              // 0x13
  {2, 8,  _INC_D},              // 0x14 *
  {2, 8,  _DEC_D},              // 0x15 *
  {3, 11, _LD_D_n},             // 0x16 *
  {1, 4,  _UNDEF},              // 0x17
  {1, 4,  _UNDEF},              // 0x18
  {2, 15, _ADD_IX_DE},          // 0x19
  {1, 4,  _UNDEF},              // 0x1A
  {1, 4,  _UNDEF},              // 0x1B
  {2, 8,  _INC_E},              // 0x1C *
  {2, 8,  _DEC_E},              // 0x1D *
  {3, 15, _LD_E_n},             // 0x1E *
  {1, 4,  _UNDEF},              // 0x1F
  {1, 4,  _UNDEF},              // 0x20
  {4, 14, _LD_IX_nn},           // 0x21
  {4, 20, _LD_At_nn_IX},        // 0x22
  {2, 10, _INC_IX},             // 0x23
  {2, 8,  _INC_XH},             // 0x24 *
  {2, 8,  _DEC_XH},             // 0x25 *
  {2, 11, _LD_XH_n},            // 0x26 *
  {1, 4,  _UNDEF},              // 0x27
  {1, 4,  _UNDEF},              // 0x28
  {2, 15, _ADD_IX_IX},          // 0x29
  {4, 20, _LD_IX_At_nn},        // 0x2A
  {2, 10, _DEC_IX},             // 0x2B
  {2, 8,  _INC_XL},             // 0x2C *
  {2, 8,  _DEC_XL},             // 0x2D *
  {3, 11, _LD_XL_n},            // 0x2E *
  {1, 4,  _UNDEF},              // 0x2F
  {1, 4,  _UNDEF},              // 0x30
  {1, 4,  _UNDEF},              // 0x31
  {1, 4,  _UNDEF},              // 0x32
  {1, 4,  _UNDEF},              // 0x33
  {3, 23, _INC_At_IX_Plus_d},   // 0x34
  {3, 23, _DEC_At_IX_Plus_d},   // 0x35
  {4, 19, _LD_At_IX_Plus_d_n},  // 0x36
  {1, 4,  _UNDEF},              // 0x37
  {1, 4,  _UNDEF},              // 0x38
  {2, 15, _ADD_IX_SP},          // 0x39
  {1, 4,  _UNDEF},              // 0x3A
  {1, 4,  _UNDEF},              // 0x3B
  {2, 8,  _INC_A},              // 0x3C *
  {2, 8,  _DEC_A},              // 0x3D *
  {2, 11, _LD_A_n},             // 0x3E *
  {1, 4,  _UNDEF},              // 0x3F
  {2, 8,  _LD_B_B},             // 0x40 *
  {2, 8,  _LD_B_C},             // 0x41 *
  {2, 8,  _LD_B_D},             // 0x42 *
  {2, 8,  _LD_B_E},             // 0x43 *
  {2, 8,  _LD_B_XH},            // 0x44 *
  {2, 8,  _LD_B_XL},            // 0x45 *
  {3, 19, _LD_B_At_IX_Plus_d},  // 0x46
  {2, 8,  _LD_B_A},             // 0x47 *
  {2, 8,  _LD_C_B},             // 0x48 *
  {2, 8,  _LD_C_C},             // 0x49 *
  {2, 8,  _LD_C_D},             // 0x4A *
  {2, 8,  _LD_C_E},             // 0x4B *
  {2, 8,  _LD_C_XH},            // 0x4C *
  {2, 8,  _LD_C_XL},            // 0x4D *
  {3, 19, _LD_C_At_IX_Plus_d},  // 0x4E
  {2, 8,  _LD_C_A},             // 0x4F *
  {2, 8,  _LD_D_B},             // 0x50 *
  {2, 8,  _LD_D_C},             // 0x51 *
  {2, 8,  _LD_D_D},             // 0x52 *
  {2, 8,  _LD_D_E},             // 0x53 *
  {2, 8,  _LD_D_XH},            // 0x54 *
  {2, 8,  _LD_D_XL},            // 0x55 *
  {3, 19, _LD_D_At_IX_Plus_d},  // 0x56
  {2, 8,  _LD_D_A},             // 0x57 *
  {2, 8,  _LD_E_B},             // 0x58 *
  {2, 8,  _LD_E_C},             // 0x59 *
  {2, 8,  _LD_E_D},             // 0x5A *
  {2, 8,  _LD_E_E},             // 0x5B *
  {2, 8,  _LD_E_XH},            // 0x5C *
  {2, 8,  _LD_E_XL},            // 0x5D *
  {3, 19, _LD_E_At_IX_Plus_d},  // 0x5E
  {2, 8,  _LD_E_A},             // 0x5F *
  {2, 8,  _LD_XH_B},            // 0x60 *
  {2, 8,  _LD_XH_C},            // 0x61 *
  {2, 8,  _LD_XH_D},            // 0x62 *
  {2, 8,  _LD_XH_E},            // 0x63 *
  {2, 8,  _LD_XH_H},            // 0x64 *
  {2, 8,  _LD_XH_L},            // 0x65 *
  {3, 19, _LD_H_At_IX_Plus_d},  // 0x66
  {2, 8,  _LD_XH_A},            // 0x67 *
  {2, 8,  _LD_XL_B},            // 0x68 *
  {2, 8,  _LD_XL_C},            // 0x69 *
  {2, 8,  _LD_XL_D},            // 0x6A *
  {2, 8,  _LD_XL_E},            // 0x6B *
  {2, 8,  _LD_XL_H},            // 0x6C *
  {2, 8,  _LD_XL_L},            // 0x6D *
  {3, 19, _LD_L_At_IX_Plus_d},  // 0x6E
  {2, 8,  _LD_XL_A},            // 0x6F *
  {3, 19, _LD_At_IX_Plus_d_B},  // 0x70
  {3, 19, _LD_At_IX_Plus_d_C},  // 0x71
  {3, 19, _LD_At_IX_Plus_d_D},  // 0x72
  {3, 19, _LD_At_IX_Plus_d_E},  // 0x73
  {3, 19, _LD_At_IX_Plus_d_H},  // 0x74
  {3, 19, _LD_At_IX_Plus_d_L},  // 0x75
  {1, 4,  _UNDEF},              // 0x76
  {3, 19, _LD_At_IX_Plus_d_A},  // 0x77
  {2, 8,  _LD_A_B},             // 0x78 *
  {2, 8,  _LD_A_C},             // 0x79 *
  {2, 8,  _LD_A_D},             // 0x7A *
  {2, 8,  _LD_A_E},             // 0x7B *
  {2, 8,  _LD_A_XH},            // 0x7C *
  {2, 8,  _LD_A_XL},            // 0x7D *
  {3, 19, _LD_A_At_IX_Plus_d},  // 0x7E
  {2, 8,  _LD_A_A},             // 0x7F *
  {2, 8,  _ADD_A_B},            // 0x80 *
  {2, 8,  _ADD_A_C},            // 0x81 *
  {2, 8,  _ADD_A_D},            // 0x82 *
  {2, 8,  _ADD_A_E},            // 0x83 *
  {2, 8,  _ADD_A_XH},           // 0x84 *
  {2, 8,  _ADD_A_XL},           // 0x85 *
  {3, 19, _ADD_A_At_IX_Plus_d}, // 0x86
  {2, 8,  _ADD_A_A},            // 0x87 *
  {2, 8,  _ADC_A_B},            // 0x88 *
  {2, 8,  _ADC_A_C},            // 0x89 *
  {2, 8,  _ADC_A_D},            // 0x8A *
  {2, 8,  _ADC_A_E},            // 0x8B *
  {2, 8,  _ADC_A_XH},           // 0x8C *
  {2, 8,  _ADC_A_XL},           // 0x8D *
  {3, 19, _ADC_A_At_IX_Plus_d}, // 0x8E
  {2, 8,  _ADC_A_A},            // 0x8F *
  {2, 8,  _SUB_A_B},            // 0x90 *
  {2, 8,  _SUB_A_C},            // 0x91 *
  {2, 8,  _SUB_A_D},            // 0x92 *
  {2, 8,  _SUB_A_E},            // 0x93 *
  {2, 8,  _SUB_A_XH},           // 0x94 *
  {2, 8,  _SUB_A_XL},           // 0x95 *
  {3, 19, _SUB_A_At_IX_Plus_d}, // 0x96
  {2, 8,  _SUB_A_A},            // 0x97 *
  {2, 8,  _SBC_A_B},            // 0x98 *
  {2, 8,  _SBC_A_C},            // 0x99 *
  {2, 8,  _SBC_A_D},            // 0x9A *
  {2, 8,  _SBC_A_E},            // 0x9B *
  {2, 8,  _SBC_A_XH},           // 0x9C *
  {2, 8,  _SBC_A_XL},           // 0x9D *
  {3, 19, _SBC_A_At_IX_Plus_d}, // 0x9E
  {2, 8,  _SBC_A_A},            // 0x9F *
  {2, 8,  _AND_A_B},            // 0xA0 *
  {2, 8,  _AND_A_C},            // 0xA1 *
  {2, 8,  _AND_A_D},            // 0xA2 *
  {2, 8,  _AND_A_E},            // 0xA3 *
  {2, 8,  _AND_A_XH},           // 0xA4 *
  {2, 8,  _AND_A_XL},           // 0xA5 *
  {3, 19, _AND_A_At_IX_Plus_d}, // 0xA6
  {2, 8,  _AND_A_A},            // 0xA7 *
  {2, 8,  _XOR_A_B},            // 0xA8 *
  {2, 8,  _XOR_A_C},            // 0xA9 *
  {2, 8,  _XOR_A_D},            // 0xAA *
  {2, 8,  _XOR_A_E},            // 0xAB *
  {2, 8,  _XOR_A_XH},           // 0xAC *
  {2, 8,  _XOR_A_XL},           // 0xAD *
  {3, 19, _XOR_A_At_IX_Plus_d}, // 0xAE
  {2, 8,  _XOR_A_A},            // 0xAF *
  {2, 8,  _OR_A_B},             // 0xB0 *
  {2, 8,  _OR_A_C},             // 0xB1 *
  {2, 8,  _OR_A_D},             // 0xB2 *
  {2, 8,  _OR_A_E},             // 0xB3 *
  {2, 8,  _OR_A_XH},            // 0xB4 *
  {2, 8,  _OR_A_XL},            // 0xB5 *
  {3, 19, _OR_A_At_IX_Plus_d},  // 0xB6
  {2, 8,  _OR_A_A},             // 0xB7 *
  {2, 8,  _CP_B},               // 0xB8 *
  {2, 8,  _CP_C},               // 0xB9 *
  {2, 8,  _CP_D},               // 0xBA *
  {2, 8,  _CP_E},               // 0xBB *
  {2, 8,  _CP_A_XH},            // 0xBC *
  {2, 8,  _CP_A_XL},            // 0xBD *
  {3, 19, _CP_At_IX_Plus_d},    // 0xBE
  {2, 8,  _CP_A},               // 0xBF *
  {1, 4,  _UNDEF},              // 0xC0
  {1, 4,  _UNDEF},              // 0xC1
  {1, 4,  _UNDEF},              // 0xC2
  {1, 4,  _UNDEF},              // 0xC3
  {1, 4,  _UNDEF},              // 0xC4
  {1, 4,  _UNDEF},              // 0xC5
  {1, 4,  _UNDEF},              // 0xC6
  {1, 4,  _UNDEF},              // 0xC7
  {1, 4,  _UNDEF},              // 0xC8
  {1, 4,  _UNDEF},              // 0xC9
  {1, 4,  _UNDEF},              // 0xCA
  {0, 0,  _OPCODE_DD_CB},       // 0xCB
  {1, 4,  _UNDEF},              // 0xCC
  {1, 4,  _UNDEF},              // 0xCD
  {1, 4,  _UNDEF},              // 0xCE
  {1, 4,  _UNDEF},              // 0xCF
  {1, 4,  _UNDEF},              // 0xD0
  {1, 4,  _UNDEF},              // 0xD1
  {1, 4,  _UNDEF},              // 0xD2
  {1, 4,  _UNDEF},              // 0xD3
  {1, 4,  _UNDEF},              // 0xD4
  {1, 4,  _UNDEF},              // 0xD5
  {1, 4,  _UNDEF},              // 0xD6
  {1, 4,  _UNDEF},              // 0xD7
  {1, 4,  _UNDEF},              // 0xD8
  {1, 4,  _UNDEF},              // 0xD9
  {1, 4,  _UNDEF},              // 0xDA
  {1, 4,  _UNDEF},              // 0xDB
  {1, 4,  _UNDEF},              // 0xDC
  {1, 4,  _UNDEF},              // 0xDD
  {1, 4,  _UNDEF},              // 0xDE
  {1, 4,  _UNDEF},              // 0xDF
  {1, 4,  _UNDEF},              // 0xE0
  {2, 14, _POP_IX},             // 0xE1
  {1, 4,  _UNDEF},              // 0xE2
  {2, 23, _EX_At_SP_IX},        // 0xE3
  {1, 4,  _UNDEF},              // 0xE4
  {2, 15, _PUSH_IX},            // 0xE5
  {1, 4,  _UNDEF},              // 0xE6
  {1, 4,  _UNDEF},              // 0xE7
  {1, 4,  _UNDEF},              // 0xE8
  {0, 0,  _JP_At_IX},           // 0xE9
  {1, 4,  _UNDEF},              // 0xEA
  {1, 4,  _UNDEF},              // 0xEB
  {1, 4,  _UNDEF},              // 0xEC
  {1, 4,  _UNDEF},              // 0xED
  {1, 4,  _UNDEF},              // 0xEE
  {1, 4,  _UNDEF},              // 0xEF
  {1, 4,  _UNDEF},              // 0xF0
  {1, 4,  _UNDEF},              // 0xF1
  {1, 4,  _UNDEF},              // 0xF2
  {1, 4,  _UNDEF},              // 0xF3
  {1, 4,  _UNDEF},              // 0xF4
  {1, 4,  _UNDEF},              // 0xF5
  {1, 4,  _UNDEF},              // 0xF6
  {1, 4,  _UNDEF},              // 0xF7
  {1, 4,  _UNDEF},              // 0xF8
  {2, 10, _LD_SP_IX},           // 0xF9
  {1, 4,  _UNDEF},              // 0xFA
  {1, 4,  _UNDEF},              // 0xFB
  {1, 4,  _UNDEF},              // 0xFC
  {1, 4,  _UNDEF},              // 0xFD
  {1, 4,  _UNDEF},              // 0xFE
  {1, 4,  _UNDEF},              // 0xFF
};

void _OPCODE_DD()
{
  BYTE by = GetMem(cpu.pc+1);

  if (inst_DD[by].bytes == 0)
  {
    inst_DD[by].inst();
  }
  else
  {
    word pc = cpu.pc;
    inst_DD[by].inst();
    cpu.pc = pc + inst_DD[by].bytes;
    cpu.cycles += inst_DD[by].cycles;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void _OUT_C_B()
{
  WritePort(cpu.regs.byteregs.c, cpu.regs.byteregs.b);
}

void _OUT_C_C()
{
  WritePort(cpu.regs.byteregs.c, cpu.regs.byteregs.c);
}

void _OUT_C_D()
{
  WritePort(cpu.regs.byteregs.c, cpu.regs.byteregs.d);
}

void _OUT_C_E()
{
  WritePort(cpu.regs.byteregs.c, cpu.regs.byteregs.e);
}

void _OUT_C_H()
{
  WritePort(cpu.regs.byteregs.c, cpu.regs.byteregs.h);
}

void _OUT_C_L()
{
  WritePort(cpu.regs.byteregs.c, cpu.regs.byteregs.l);
}

void _OUT_C_A()
{
  WritePort(cpu.regs.byteregs.c, cpu.regs.byteregs.a);
}

void _OUTI()
{
  WritePort(cpu.regs.byteregs.c, GetMem(cpu.regs.wordregs.hl));

  ++cpu.regs.wordregs.hl;
  --cpu.regs.byteregs.b;

  // Z is set if b-1 is 0; otherwise, it is reset.
  if ((cpu.regs.byteregs.b & 0xFF) == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~Z_FLAG_MASK;
  }

  // N is set.
  cpu.regs.byteregs.f |= N_FLAG_MASK;
}

void _OTIR()
{
  do {
    WritePort(cpu.regs.byteregs.c, GetMem(cpu.regs.wordregs.hl));

    ++cpu.regs.wordregs.hl;
    --cpu.regs.byteregs.b;

    if (cpu.regs.byteregs.b != 0)
    {
      cpu.cycles += 21;
    }
    else
    {
      cpu.cycles += 16;
    }
  } while (cpu.regs.byteregs.b != 0);

  // Z is set
  cpu.regs.byteregs.f |= Z_FLAG_MASK;

  // N is set.
  cpu.regs.byteregs.f |= N_FLAG_MASK;

  cpu.pc += 2;
}

void _OUTD()
{
  WritePort(cpu.regs.byteregs.c, GetMem(cpu.regs.wordregs.hl));

  --cpu.regs.wordregs.hl;
  --cpu.regs.byteregs.b;

  // Z is set if b-1 is 0; otherwise, it is reset.
  if ((cpu.regs.byteregs.b & 0xFF) == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~Z_FLAG_MASK;
  }

  // N is set.
  cpu.regs.byteregs.f |= N_FLAG_MASK;
}

void _OTDR()
{
  do {
    WritePort(cpu.regs.byteregs.c, GetMem(cpu.regs.wordregs.hl));

    --cpu.regs.wordregs.hl;
    --cpu.regs.byteregs.b;

    if (cpu.regs.byteregs.b != 0)
    {
      cpu.cycles += 21;
    }
    else
    {
      cpu.cycles += 16;
    }
  } while (cpu.regs.byteregs.b != 0);

  // Z is set
   cpu.regs.byteregs.f |= Z_FLAG_MASK;

  // N is set.
  cpu.regs.byteregs.f |= N_FLAG_MASK;

  cpu.pc += 2;
}

void _IN_r_C()
{
  byte* p;
  int   r = (GetMem(cpu.pc+1) >> 3) & 0x07;

  switch (r)
  {
    case 0:
      p = &cpu.regs.byteregs.b;
      break;

    case 1:
      p = &cpu.regs.byteregs.c;
      break;

    case 2:
      p = &cpu.regs.byteregs.d;
      break;

    case 3:
      p = &cpu.regs.byteregs.e;
      break;

    case 4:
      p = &cpu.regs.byteregs.h;
      break;

    case 5:
      p = &cpu.regs.byteregs.l;
      break;

    case 6:
      p = &cpu.regs.byteregs.f;
      break;

    default:
      p = &cpu.regs.byteregs.a;
      break;
  }

  *p = ReadPort16(cpu.regs.byteregs.c, cpu.regs.byteregs.b);

  cpu.regs.byteregs.f = 0;

  // S is set if result is negative; otherwise, it is reset.
  if ((*p & 0x80) != 0)
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }

  // Z is set if result is 0; otherwise, it is reset.
  if ((*p & 0xFF) == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }

  // H is reset.

  // P/V is set if parity even; otherwise, it is reset.
  cpu.regs.byteregs.f |= byParity[*p & 0xFF];

  // N is reset.
}

void _INI()
{
  WriteMem(cpu.regs.wordregs.hl, ReadPort16(cpu.regs.byteregs.c, cpu.regs.byteregs.b));
  --cpu.regs.byteregs.b;
  ++cpu.regs.wordregs.hl;

  // Z is set if b-1 is 0; otherwise, it is reset.
  if ((cpu.regs.byteregs.b & 0xFF) == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~Z_FLAG_MASK;
  }

  // N is set.
  cpu.regs.byteregs.f |= N_FLAG_MASK;
}

void _INIR()
{
  do {
    PutMem(cpu.regs.wordregs.hl, ReadPort16(cpu.regs.byteregs.c, cpu.regs.byteregs.b));
    --cpu.regs.byteregs.b;
    ++cpu.regs.wordregs.hl;

    if (cpu.regs.byteregs.b != 0)
    {
      cpu.cycles += 21;
    }
    else
    {
      cpu.cycles += 16;
    }
  } while (cpu.regs.byteregs.b != 0);

  // Z is set.
  cpu.regs.byteregs.f |= Z_FLAG_MASK;

  // N is set.
  cpu.regs.byteregs.f |= N_FLAG_MASK;

  cpu.pc += 2;
}

void _IND()
{
  PutMem(cpu.regs.wordregs.hl, ReadPort16(cpu.regs.byteregs.c, cpu.regs.byteregs.b));
  --cpu.regs.byteregs.b;
  --cpu.regs.wordregs.hl;

  // Z is set if b-1 is 0; otherwise, it is reset.
  if ((cpu.regs.byteregs.b & 0xFF) == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~Z_FLAG_MASK;
  }

  // N is set.
  cpu.regs.byteregs.f |= N_FLAG_MASK;
}

void _INDR()
{
  do {
    PutMem(cpu.regs.wordregs.hl, ReadPort16(cpu.regs.byteregs.c, cpu.regs.byteregs.b));
    --cpu.regs.byteregs.b;
    --cpu.regs.wordregs.hl;

    if (cpu.regs.byteregs.b != 0)
    {
      cpu.cycles += 21;
    }
    else
    {
      cpu.cycles += 16;
    }
  } while (cpu.regs.byteregs.b != 0);

  // Z is set
  cpu.regs.byteregs.f |= Z_FLAG_MASK;

  // N is set.
  cpu.regs.byteregs.f |= N_FLAG_MASK;

  cpu.pc += 2;
}

void _RETI()
{
  cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
  cpu.sp += 2;
  cpu.cycles += 14;
}

void _RETN()
{
  cpu.pc = (GetMem(cpu.sp+1) << 8) + GetMem(cpu.sp);
  cpu.iff1 = cpu.iff2;
  cpu.sp += 2;
  cpu.cycles += 14;
}

void _LD_A_I()
{
  cpu.regs.byteregs.a = cpu.i;

  if (cpu.i >= 0x80) // negative
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~S_FLAG_MASK;
  }

  if (cpu.i == 0x00)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~Z_FLAG_MASK;
  }

  cpu.regs.byteregs.f &= ~H_FLAG_MASK;

  if (cpu.iff2 == 0)
  {
    cpu.regs.byteregs.f &= ~P_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f |= P_FLAG_MASK;
  }

  cpu.regs.byteregs.f &= ~N_FLAG_MASK;

  // TODO: If an interrupt occurs during execution of this instruction, the Parity flag contains a 0.

}

void _LD_A_R()
{
  cpu.regs.byteregs.a = cpu.r;

  if (cpu.r >= 0x80) // negative
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~S_FLAG_MASK;
  }

  if (cpu.r == 0x00)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~Z_FLAG_MASK;
  }

  cpu.regs.byteregs.f &= ~H_FLAG_MASK;

  if (cpu.iff2 == 0)
  {
    cpu.regs.byteregs.f &= ~P_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f |= P_FLAG_MASK;
  }

  cpu.regs.byteregs.f &= ~N_FLAG_MASK;

  // TODO: If an interrupt occurs during execution of this instruction, the Parity flag contains a 0.

}

void _LD_I_A()
{
  cpu.i = cpu.regs.byteregs.a;
}

void _LD_R_A()
{
  cpu.r = cpu.regs.byteregs.a;
}

void _LD_dd_At_nn()
{
  int d = (GetMem(cpu.pc+1) >> 4) & 0x03;
  int n = GetMem(cpu.pc+2) + (GetMem(cpu.pc+3) << 8);

  switch (d)
  {
    case 0:
      cpu.regs.byteregs.b = ReadMem(n+1);
      cpu.regs.byteregs.c = ReadMem(n);
      break;

    case 1:
      cpu.regs.byteregs.d = ReadMem(n+1);
      cpu.regs.byteregs.e = ReadMem(n);
      break;

    case 2:
      cpu.regs.byteregs.h = ReadMem(n+1);
      cpu.regs.byteregs.l = ReadMem(n);
      break;

    case 3:
      cpu.sp = (ReadMem(n+1) << 8) + ReadMem(n);
      break;
  }
}

void _LD_At_nn_dd()
{
  int n = GetMem(cpu.pc+2) + (GetMem(cpu.pc+3) << 8);
  int d = (GetMem(cpu.pc+1) >> 4) & 0x03;

  switch (d)
  {
    case 0:
      WriteMem(n+1, cpu.regs.byteregs.b);
      WriteMem(n, cpu.regs.byteregs.c);
      break;

    case 1:
      WriteMem(n+1, cpu.regs.byteregs.d);
      WriteMem(n, cpu.regs.byteregs.e);
      break;

    case 2:
      WriteMem(n+1, cpu.regs.byteregs.h);
      WriteMem(n, cpu.regs.byteregs.l);
      break;

    case 3:
      WriteMem(n+1, (cpu.sp >> 8) & 0xFF);
      WriteMem(n, cpu.sp & 0xFF);
      break;
  }
}

void _LDI()
{
  ldi();
}

void _LDIR()
{
  do {
    ldi();

    if (cpu.regs.wordregs.bc != 0)
    {
	    cpu.cycles += 21;
    }
    else
    {
	    cpu.cycles += 16;
    }
  } while (cpu.regs.wordregs.bc != 0);

  cpu.pc += 2;
}

void _LDD()
{
  ldd();
}

void _LDDR()
{
  do {
    ldd();

    if (cpu.regs.wordregs.bc != 0)
    {
	    cpu.cycles += 21;
    }
    else
    {
	    cpu.cycles += 16;
    }
  } while (cpu.regs.wordregs.bc != 0);

  cpu.pc += 2;
}

void _CPI()
{
  cpi();
}

void _CPIR()
{
  do {
    cpi();
    cpu.cycles += 21;
  } while ((get_bc() != 0) && (getflag(Z_FLAG_MASK) == 0));

  cpu.cycles += 16;
  cpu.pc += 2;
}

void _CPD()
{
  cpd();
}

void _CPDR()
{
  do {
    cpd();
    cpu.cycles += 21;
  } while ((get_bc() != 0) && (getflag(Z_FLAG_MASK) == 0));

  cpu.cycles += 16;
  cpu.pc += 2;
}

void _NEG()
{
  cpu.regs.byteregs.a = subb(0, cpu.regs.byteregs.a, 0);
}

void _IM0()
{
  cpu.imode = 0;
}

void _IM1()
{
  cpu.imode = 1;
}

void _IM2()
{
  cpu.imode = 2;
}

void _ADC_HL_ss()
{
  byte     s = (GetMem(cpu.pc+1) >> 4) & 0x03;
  uint16_t w = 0;
  uint16_t cy = 0;

  switch (s)
  {
    case 0: // 0100 1010 (0x4A)
      w = cpu.regs.wordregs.bc;
      break;

    case 1: // 0101 1010 (0x5A)
      w = cpu.regs.wordregs.de;
      break;

    case 2: // 0110 1010 (0x6A)
      w = cpu.regs.wordregs.hl;
      break;

    case 3: // 0111 1010 (0x7A)
      w = cpu.sp;
      break;
  }

  if ((cpu.regs.byteregs.f & C_FLAG_MASK) != 0)
  {
    cy = 1;
  }

  uint16_t result = addw(cpu.regs.wordregs.hl, w, cy);

  if (result & 0x8000)
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~S_FLAG_MASK;
  }

  if (result == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }
  else
  {
    cpu.regs.byteregs.f &= ~Z_FLAG_MASK;
  }

  cpu.regs.wordregs.hl = result;
}

void _SBC_HL_BC()
{
  sbchl(cpu.regs.wordregs.bc);
}

void _SBC_HL_DE()
{
  sbchl(cpu.regs.wordregs.de);
}

void _SBC_HL_HL()
{
  sbchl(cpu.regs.wordregs.hl);
}

void _SBC_HL_SP()
{
  sbchl(cpu.sp);
}

void _RLD()
{
  word n = GetMem(cpu.regs.wordregs.hl);
  word a = cpu.regs.byteregs.a;

  a = (a & 0xF0) | (n >> 4);
  n = (n << 4) | (cpu.regs.byteregs.a & 0x0F);

  PutMem(cpu.regs.wordregs.hl, n & 0xFF);
  cpu.regs.byteregs.a = a & 0xFF;

  // C is not affected.
  cpu.regs.byteregs.f &= C_FLAG_MASK;

  // S is set if result is negative; otherwise, it is reset.
  if ((a & 0x80) != 0)
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }

  // Z is set if result is 0; otherwise, it is reset.
  if ((a & 0xFF) == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }

  // H is reset.

  // P/V is set if parity even; otherwise, it is reset.
  cpu.regs.byteregs.f |= byParity[a & 0xFF];

  // N is reset.

  setflag(X_FLAG_MASK, getbit(3, cpu.regs.byteregs.a));
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.byteregs.a));
}

void _RRD()
{
  word a = cpu.regs.byteregs.a;
  word b = GetMem(cpu.regs.wordregs.hl);

  PutMem(cpu.regs.wordregs.hl, (b >> 4) | ((a << 4) & 0xF0));
  cpu.regs.byteregs.a = (a & 0xF0) | (b & 0x0F);

  // C is not affected.
  cpu.regs.byteregs.f &= C_FLAG_MASK;

  // S is set if result is negative; otherwise, it is reset.
  if (cpu.regs.byteregs.a & 0x80)
  {
    cpu.regs.byteregs.f |= S_FLAG_MASK;
  }

  // Z is set if result is 0; otherwise, it is reset.
  if (cpu.regs.byteregs.a == 0)
  {
    cpu.regs.byteregs.f |= Z_FLAG_MASK;
  }

  // H is reset.

  // P/V is set if parity even; otherwise, it is reset.
  cpu.regs.byteregs.f |= byParity[cpu.regs.byteregs.a];

  // N is reset.

  setflag(X_FLAG_MASK, getbit(3, cpu.regs.byteregs.a));
  setflag(Y_FLAG_MASK, getbit(5, cpu.regs.byteregs.a));
}

void _IN_C()
{
  cpu.regs.byteregs.a = ReadPort(cpu.regs.byteregs.c);
}

void _OUT_C_0()
{
  WritePort(cpu.regs.byteregs.a, 0);
}

/////////////////////////////////////////////////
InstType inst_ED[] = {
  {1, 4,  _UNDEF},       // 0x00
  {1, 4,  _UNDEF},       // 0x01
  {1, 4,  _UNDEF},       // 0x02
  {1, 4,  _UNDEF},       // 0x03
  {1, 4,  _UNDEF},       // 0x04
  {1, 4,  _UNDEF},       // 0x05
  {1, 4,  _UNDEF},       // 0x06
  {1, 4,  _UNDEF},       // 0x07
  {1, 4,  _UNDEF},       // 0x08
  {1, 4,  _UNDEF},       // 0x09
  {1, 4,  _UNDEF},       // 0x0A
  {1, 4,  _UNDEF},       // 0x0B
  {1, 4,  _UNDEF},       // 0x0C
  {1, 4,  _UNDEF},       // 0x0D
  {1, 4,  _UNDEF},       // 0x0E
  {1, 4,  _UNDEF},       // 0x0F
  {1, 4,  _UNDEF},       // 0x10
  {1, 4,  _UNDEF},       // 0x11
  {1, 4,  _UNDEF},       // 0x12
  {1, 4,  _UNDEF},       // 0x13
  {1, 4,  _UNDEF},       // 0x14
  {1, 4,  _UNDEF},       // 0x15
  {1, 4,  _UNDEF},       // 0x16
  {1, 4,  _UNDEF},       // 0x17
  {1, 4,  _UNDEF},       // 0x18
  {1, 4,  _UNDEF},       // 0x19
  {1, 4,  _UNDEF},       // 0x1A
  {1, 4,  _UNDEF},       // 0x1B
  {1, 4,  _UNDEF},       // 0x1C
  {1, 4,  _UNDEF},       // 0x1D
  {1, 4,  _UNDEF},       // 0x1E
  {1, 4,  _UNDEF},       // 0x1F
  {1, 4,  _UNDEF},       // 0x20
  {1, 4,  _UNDEF},       // 0x21
  {1, 4,  _UNDEF},       // 0x22
  {1, 4,  _UNDEF},       // 0x23
  {1, 4,  _UNDEF},       // 0x24
  {1, 4,  _UNDEF},       // 0x25
  {1, 4,  _UNDEF},       // 0x26
  {1, 4,  _UNDEF},       // 0x27
  {1, 4,  _UNDEF},       // 0x28
  {1, 4,  _UNDEF},       // 0x29
  {1, 4,  _UNDEF},       // 0x2A
  {1, 4,  _UNDEF},       // 0x2B
  {1, 4,  _UNDEF},       // 0x2C
  {1, 4,  _UNDEF},       // 0x2D
  {1, 4,  _UNDEF},       // 0x2E
  {1, 4,  _UNDEF},       // 0x2F
  {1, 4,  _UNDEF},       // 0x30
  {1, 4,  _UNDEF},       // 0x31
  {1, 4,  _UNDEF},       // 0x32
  {1, 4,  _UNDEF},       // 0x33
  {1, 4,  _UNDEF},       // 0x34
  {1, 4,  _UNDEF},       // 0x35
  {1, 4,  _UNDEF},       // 0x36
  {1, 4,  _UNDEF},       // 0x37
  {1, 4,  _UNDEF},       // 0x38
  {1, 4,  _UNDEF},       // 0x39
  {1, 4,  _UNDEF},       // 0x3A
  {1, 4,  _UNDEF},       // 0x3B
  {1, 4,  _UNDEF},       // 0x3C
  {1, 4,  _UNDEF},       // 0x3D
  {1, 4,  _UNDEF},       // 0x3E
  {1, 4,  _UNDEF},       // 0x3F
  {2, 12, _IN_r_C},      // 0x40
  {2, 12, _OUT_C_B},     // 0x41
  {2, 15, _SBC_HL_BC},   // 0x42
  {4, 20, _LD_At_nn_dd}, // 0x43
  {2, 8,  _NEG},         // 0x44
  {0, 0,  _RETN},        // 0x45
  {2, 8,  _IM0},         // 0x46
  {2, 9,  _LD_I_A},      // 0x47
  {2, 12, _IN_r_C},      // 0x48
  {2, 12, _OUT_C_C},     // 0x49
  {2, 15, _ADC_HL_ss},   // 0x4A
  {4, 20, _LD_dd_At_nn}, // 0x4B
  {1, 4,  _UNDEF},       // 0x4C
  {0, 0,  _RETI},        // 0x4D
  {1, 4,  _UNDEF},       // 0x4E
  {2, 9,  _LD_R_A},      // 0x4F
  {2, 12, _IN_r_C},      // 0x50
  {2, 12, _OUT_C_D},     // 0x51
  {2, 15, _SBC_HL_DE},   // 0x52
  {4, 20, _LD_At_nn_dd}, // 0x53
  {1, 4,  _UNDEF},       // 0x54
  {1, 4,  _UNDEF},       // 0x55
  {2, 8,  _IM1},         // 0x56
  {2, 9,  _LD_A_I},      // 0x57
  {2, 12, _IN_r_C},      // 0x58
  {2, 12, _OUT_C_E},     // 0x59
  {2, 15, _ADC_HL_ss},   // 0x5A
  {4, 20, _LD_dd_At_nn}, // 0x5B
  {1, 4,  _UNDEF},       // 0x5C
  {1, 4,  _UNDEF},       // 0x5D
  {2, 8,  _IM2},         // 0x5E
  {2, 9,  _LD_A_R},      // 0x5F
  {2, 12, _IN_r_C},      // 0x60
  {2, 12, _OUT_C_H},     // 0x61
  {2, 15, _SBC_HL_HL},   // 0x62
  {4, 20, _LD_At_nn_dd}, // 0x63
  {1, 4,  _UNDEF},       // 0x64
  {1, 4,  _UNDEF},       // 0x65
  {1, 4,  _UNDEF},       // 0x66
  {2, 18, _RRD},         // 0x67
  {2, 12, _IN_r_C},      // 0x68
  {2, 12, _OUT_C_L},     // 0x69
  {2, 15, _ADC_HL_ss},   // 0x6A
  {4, 20, _LD_dd_At_nn}, // 0x6B
  {1, 4,  _UNDEF},       // 0x6C
  {1, 4,  _UNDEF},       // 0x6D
  {1, 4,  _UNDEF},       // 0x6E
  {2, 18, _RLD},         // 0x6F
  {2, 12, _IN_C},        // 0x70 *
  {2, 12, _OUT_C_0},     // 0x71 *
  {2, 15, _SBC_HL_SP},   // 0x72
  {4, 20, _LD_At_nn_dd}, // 0x73
  {1, 4,  _UNDEF},       // 0x74
  {1, 4,  _UNDEF},       // 0x75
  {1, 4,  _UNDEF},       // 0x76
  {1, 4,  _UNDEF},       // 0x77
  {2, 12, _IN_r_C},      // 0x78
  {2, 12, _OUT_C_A},     // 0x79
  {2, 15, _ADC_HL_ss},   // 0x7A
  {4, 20, _LD_dd_At_nn}, // 0x7B
  {1, 4,  _UNDEF},       // 0x7C
  {1, 4,  _UNDEF},       // 0x7D
  {1, 4,  _UNDEF},       // 0x7E
  {1, 4,  _UNDEF},       // 0x7F
  {1, 4,  _UNDEF},       // 0x80
  {1, 4,  _UNDEF},       // 0x81
  {1, 4,  _UNDEF},       // 0x82
  {1, 4,  _UNDEF},       // 0x83
  {1, 4,  _UNDEF},       // 0x84
  {1, 4,  _UNDEF},       // 0x85
  {1, 4,  _UNDEF},       // 0x86
  {1, 4,  _UNDEF},       // 0x87
  {1, 4,  _UNDEF},       // 0x88
  {1, 4,  _UNDEF},       // 0x89
  {1, 4,  _UNDEF},       // 0x8A
  {1, 4,  _UNDEF},       // 0x8B
  {1, 4,  _UNDEF},       // 0x8C
  {1, 4,  _UNDEF},       // 0x8D
  {1, 4,  _UNDEF},       // 0x8E
  {1, 4,  _UNDEF},       // 0x8F
  {1, 4,  _UNDEF},       // 0x90
  {1, 4,  _UNDEF},       // 0x91
  {1, 4,  _UNDEF},       // 0x92
  {1, 4,  _UNDEF},       // 0x93
  {1, 4,  _UNDEF},       // 0x94
  {1, 4,  _UNDEF},       // 0x95
  {1, 4,  _UNDEF},       // 0x96
  {1, 4,  _UNDEF},       // 0x97
  {1, 4,  _UNDEF},       // 0x98
  {1, 4,  _UNDEF},       // 0x99
  {1, 4,  _UNDEF},       // 0x9A
  {1, 4,  _UNDEF},       // 0x9B
  {1, 4,  _UNDEF},       // 0x9C
  {1, 4,  _UNDEF},       // 0x9D
  {1, 4,  _UNDEF},       // 0x9E
  {1, 4,  _UNDEF},       // 0x9F
  {2, 16, _LDI},         // 0xA0
  {2, 16, _CPI},         // 0xA1
  {2, 16, _INI},         // 0xA2
  {2, 16, _OUTI},        // 0xA3
  {1, 4,  _UNDEF},       // 0xA4
  {1, 4,  _UNDEF},       // 0xA5
  {1, 4,  _UNDEF},       // 0xA6
  {1, 4,  _UNDEF},       // 0xA7
  {2, 16, _LDD},         // 0xA8
  {2, 16, _CPD},         // 0xA9
  {2, 16, _IND},         // 0xAA
  {2, 16, _OUTD},        // 0xAB
  {1, 4,  _UNDEF},       // 0xAC
  {1, 4,  _UNDEF},       // 0xAD
  {1, 4,  _UNDEF},       // 0xAE
  {1, 4,  _UNDEF},       // 0xAF
  {0, 0,  _LDIR},        // 0xB0
  {0, 0,  _CPIR},        // 0xB1
  {0, 0,  _INIR},        // 0xB2
  {0, 0,  _OTIR},        // 0xB3
  {1, 4,  _UNDEF},       // 0xB4
  {1, 4,  _UNDEF},       // 0xB5
  {1, 4,  _UNDEF},       // 0xB6
  {1, 4,  _UNDEF},       // 0xB7
  {0, 0,  _LDDR},        // 0xB8
  {0, 0,  _CPDR},        // 0xB9
  {0, 0,  _INDR},        // 0xBA
  {0, 0,  _OTDR},        // 0xBB
  {1, 4,  _UNDEF},       // 0xBC
  {1, 4,  _UNDEF},       // 0xBD
  {1, 4,  _UNDEF},       // 0xBE
  {1, 4,  _UNDEF},       // 0xBF
  {1, 4,  _UNDEF},       // 0xC0
  {1, 4,  _UNDEF},       // 0xC1
  {1, 4,  _UNDEF},       // 0xC2
  {1, 4,  _UNDEF},       // 0xC3
  {1, 4,  _UNDEF},       // 0xC4
  {1, 4,  _UNDEF},       // 0xC5
  {1, 4,  _UNDEF},       // 0xC6
  {1, 4,  _UNDEF},       // 0xC7
  {1, 4,  _UNDEF},       // 0xC8
  {1, 4,  _UNDEF},       // 0xC9
  {1, 4,  _UNDEF},       // 0xCA
  {1, 4,  _UNDEF},       // 0xCB
  {1, 4,  _UNDEF},       // 0xCC
  {1, 4,  _UNDEF},       // 0xCD
  {1, 4,  _UNDEF},       // 0xCE
  {1, 4,  _UNDEF},       // 0xCF
  {1, 4,  _UNDEF},       // 0xD0
  {1, 4,  _UNDEF},       // 0xD1
  {1, 4,  _UNDEF},       // 0xD2
  {1, 4,  _UNDEF},       // 0xD3
  {1, 4,  _UNDEF},       // 0xD4
  {1, 4,  _UNDEF},       // 0xD5
  {1, 4,  _UNDEF},       // 0xD6
  {1, 4,  _UNDEF},       // 0xD7
  {1, 4,  _UNDEF},       // 0xD8
  {1, 4,  _UNDEF},       // 0xD9
  {1, 4,  _UNDEF},       // 0xDA
  {1, 4,  _UNDEF},       // 0xDB
  {1, 4,  _UNDEF},       // 0xDC
  {1, 4,  _UNDEF},       // 0xDD
  {1, 4,  _UNDEF},       // 0xDE
  {1, 4,  _UNDEF},       // 0xDF
  {1, 4,  _UNDEF},       // 0xE0
  {1, 4,  _UNDEF},       // 0xE1
  {1, 4,  _UNDEF},       // 0xE2
  {1, 4,  _UNDEF},       // 0xE3
  {1, 4,  _UNDEF},       // 0xE4
  {1, 4,  _UNDEF},       // 0xE5
  {1, 4,  _UNDEF},       // 0xE6
  {1, 4,  _UNDEF},       // 0xE7
  {1, 4,  _UNDEF},       // 0xE8
  {1, 4,  _UNDEF},       // 0xE9
  {1, 4,  _UNDEF},       // 0xEA
  {1, 4,  _UNDEF},       // 0xEB
  {1, 4,  _UNDEF},       // 0xEC
  {1, 4,  _UNDEF},       // 0xED
  {1, 4,  _UNDEF},       // 0xEE
  {1, 4,  _UNDEF},       // 0xEF
  {1, 4,  _UNDEF},       // 0xF0
  {1, 4,  _UNDEF},       // 0xF1
  {1, 4,  _UNDEF},       // 0xF2
  {1, 4,  _UNDEF},       // 0xF3
  {1, 4,  _UNDEF},       // 0xF4
  {1, 4,  _UNDEF},       // 0xF5
  {1, 4,  _UNDEF},       // 0xF6
  {1, 4,  _UNDEF},       // 0xF7
  {1, 4,  _UNDEF},       // 0xF8
  {1, 4,  _UNDEF},       // 0xF9
  {1, 4,  _UNDEF},       // 0xFA
  {1, 4,  _UNDEF},       // 0xFB
  {1, 4,  _UNDEF},       // 0xFC
  {1, 4,  _UNDEF},       // 0xFD
  {1, 4,  _UNDEF},       // 0xFE
  {1, 4,  _UNDEF},       // 0xFF
};

void _OPCODE_ED()
{
  byte by = GetMem(cpu.pc+1);

  if (inst_ED[by].bytes == 0)
  {
    inst_ED[by].inst();
  }
  else
  {
    word pc = cpu.pc;
    inst_ED[by].inst();
    cpu.pc = pc + inst_ED[by].bytes;
    cpu.cycles += inst_ED[by].cycles;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void _LD_r_At_IY_Plus_d()
{
  BYTE r = (GetMem(cpu.pc+1) >> 3) & 0x7;

	union two_comp {
		BYTE by;
		char n;
	} d;

  d.by = GetMem(cpu.pc+2);

  int a = cpu.regs.wordregs.iy + d.n;

  switch (r)
  {
    case 0:
      cpu.regs.byteregs.b = ReadMem(a);
      break;

    case 1:
      cpu.regs.byteregs.c = ReadMem(a);
      break;

    case 2:
      cpu.regs.byteregs.d = ReadMem(a);
      break;

    case 3:
      cpu.regs.byteregs.e = ReadMem(a);
      break;

    case 4:
      cpu.regs.byteregs.h = ReadMem(a);
      break;

    case 5:
      cpu.regs.byteregs.l = ReadMem(a);
      break;

    case 6:
      // undefined
      break;

    case 7:
      cpu.regs.byteregs.a = ReadMem(a);
      break;
  }
}

void _LD_At_IY_Plus_d_r()
{
  BYTE r = GetMem(cpu.pc+1) & 0x7;

	union two_comp {
		BYTE by;
		char n;
	} d;

  d.by = GetMem(cpu.pc+2);

  int a = cpu.regs.wordregs.iy + d.n;

  switch (r)
  {
    case 0:
      WriteMem(a, cpu.regs.byteregs.b);
      break;

    case 1:
      WriteMem(a, cpu.regs.byteregs.c);
      break;

    case 2:
      WriteMem(a, cpu.regs.byteregs.d);
      break;

    case 3:
      WriteMem(a, cpu.regs.byteregs.e);
      break;

    case 4:
      WriteMem(a, cpu.regs.byteregs.h);
      break;

    case 5:
      WriteMem(a, cpu.regs.byteregs.l);
      break;

    case 6:
      // undefined
      break;

    case 7:
      WriteMem(a, cpu.regs.byteregs.a);
      break;
  }
}

void _LD_At_IY_Plus_d_n()
{
  int a;
	union two_comp {
		BYTE by;
		char n;
	} d;

  d.by = GetMem(cpu.pc+2);
  a = cpu.regs.wordregs.iy + d.n;
  WriteMem(a, GetMem(cpu.pc+3));
}

void _LD_IY_nn()
{
  cpu.regs.byteregs.iyh = GetMem(cpu.pc+3);
  cpu.regs.byteregs.iyl = GetMem(cpu.pc+2);
}

void _LD_IY_At_nn()
{
  int n = GetMem(cpu.pc+2) + (GetMem(cpu.pc+3) << 8);
  cpu.regs.byteregs.iyh = ReadMem(n+1);
  cpu.regs.byteregs.iyl = ReadMem(n);
}

void _LD_At_nn_IY()
{
  int n = GetMem(cpu.pc+2) + (GetMem(cpu.pc+3) << 8);
  WriteMem(n+1, cpu.regs.byteregs.iyh);
  WriteMem(n, cpu.regs.byteregs.iyl);
}

void _LD_SP_IY()
{
  cpu.sp = cpu.regs.wordregs.iy;
}

void _PUSH_IY()
{
  PutMem(cpu.sp-1, cpu.regs.byteregs.iyh);
  PutMem(cpu.sp-2, cpu.regs.byteregs.iyl);
  cpu.sp -= 2;
}

void _POP_IY()
{
  cpu.regs.byteregs.iyh = GetMem(cpu.sp+1);
  cpu.regs.byteregs.iyl = GetMem(cpu.sp);
  cpu.sp += 2;
}

void _EX_At_SP_IY()
{
  byte x;

  x = cpu.regs.byteregs.iyh;
  cpu.regs.byteregs.iyh = GetMem(cpu.sp+1);
  PutMem(cpu.sp+1, x);

  x = cpu.regs.byteregs.iyl;
  cpu.regs.byteregs.iyl = GetMem(cpu.sp);
  PutMem(cpu.sp, x);
}

void _ADD_A_At_IY_Plus_d()
{
	union two_comp {
		BYTE by;
		char n;
	} d;
  word a;

  d.by = GetMem(cpu.pc+2);
  a    = cpu.regs.wordregs.iy + d.n;
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, GetMem(a), 0);
}

void _ADC_A_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, GetMem(a), getflag(C_FLAG_MASK));
}

void _SUB_A_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, GetMem(a), 0);
}

void _SBC_A_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, GetMem(a), getflag(C_FLAG_MASK));
}

void _AND_A_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  land(GetMem(a));
}

void _OR_A_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  lor(GetMem(a));
}

void _XOR_A_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  lxor(GetMem(a));
}

void _CP_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cp(GetMem(a));
}

void _INC_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = inc(p);
  WriteMem(a, p);
}

void _DEC_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = dec(p);
  WriteMem(a, p);
}

void _ADD_IY_pp()
{
  byte p = (GetMem(cpu.pc+1) >> 4) & 0x03;

  switch (p)
  {
    case 0: // 0000 1001 (0x09)
      addiy(cpu.regs.wordregs.bc);
      break;

    case 1: // 0001 1001 (0x19)
      addiy(cpu.regs.wordregs.de);
      break;

    case 2: // 0010 1001 (0x29)
      addiy(cpu.regs.wordregs.iy);
      break;

    case 3: // 0011 1001 (0x39)
      addiy(cpu.sp);
      break;
  }
}

void _INC_IY()
{
  ++cpu.regs.wordregs.iy;
}

void _DEC_IY()
{
  --cpu.regs.wordregs.iy;
}

void _RLC_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_rlc(p);
  WriteMem(a, p);
}

void _RL_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_rl(p);
  WriteMem(a, p);
}

void _RRC_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_rrc(p);
  WriteMem(a, p);
}

void _RR_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_rr(p);
  WriteMem(a, p);
}

void _SLA_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_sla(p);
  WriteMem(a, p);
}

void _SLL_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_sll(p);
  WriteMem(a, p);
}

void _SRA_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_sra(p);
  WriteMem(a, p);
}

void _SRL_At_IY_Plus_d()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = cb_srl(p);
  WriteMem(a, p);
}

void _BIT_b_At_IY_Plus_d()
{
  byte b = (GetMem(cpu.pc+3) >> 3) & 0x7;
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);

  cb_bit(p, b);
  setflag(X_FLAG_MASK, getbit(3, a >> 8));
  setflag(Y_FLAG_MASK, getbit(5, a >> 8));
}

void _SET_b_At_IY_Plus_d()
{
  byte mask[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
  byte b = (GetMem(cpu.pc+3) >> 3) & 0x7;
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = p | mask[b];
  WriteMem(a, p);
}

void _RES_b_At_IY_Plus_d()
{
  byte mask[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
  byte b = (GetMem(cpu.pc+3) >> 3) & 0x7;
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  byte p = ReadMem(a);
  p = p & ~mask[b];
  WriteMem(a, p);
}

void _JP_At_IY()
{
  cpu.pc = cpu.regs.wordregs.iy;
  cpu.cycles += 8;
}

void _RLC_At_IY_Plus_d_B()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rlc(GetMem(a));
}

void _RLC_At_IY_Plus_d_C()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_rlc(GetMem(a));
}

void _RLC_At_IY_Plus_d_D()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_rlc(GetMem(a));
}

void _RLC_At_IY_Plus_d_E()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_rlc(GetMem(a));
}

void _RLC_At_IY_Plus_d_H()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_rlc(GetMem(a));
}

void _RLC_At_IY_Plus_d_L()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_rlc(GetMem(a));
}

void _RLC_At_IY_Plus_d_A()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rlc(GetMem(a));
}

void _RRC_At_IY_Plus_d_B()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rrc(GetMem(a));
}

void _RRC_At_IY_Plus_d_C()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_rrc(GetMem(a));
}

void _RRC_At_IY_Plus_d_D()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_rrc(GetMem(a));
}

void _RRC_At_IY_Plus_d_E()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_rrc(GetMem(a));
}

void _RRC_At_IY_Plus_d_H()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_rrc(GetMem(a));
}

void _RRC_At_IY_Plus_d_L()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_rrc(GetMem(a));
}

void _RRC_At_IY_Plus_d_A()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rrc(GetMem(a));
}

void _RL_At_IY_Plus_d_B()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rl(GetMem(a));
}

void _RL_At_IY_Plus_d_C()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_rl(GetMem(a));
}

void _RL_At_IY_Plus_d_D()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_rl(GetMem(a));
}

void _RL_At_IY_Plus_d_E()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_rl(GetMem(a));
}

void _RL_At_IY_Plus_d_H()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_rl(GetMem(a));
}

void _RL_At_IY_Plus_d_L()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_rl(GetMem(a));
}

void _RL_At_IY_Plus_d_A()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rl(GetMem(a));
}

void _RR_At_IY_Plus_d_B()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rr(GetMem(a));
}

void _RR_At_IY_Plus_d_C()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_rr(GetMem(a));
}

void _RR_At_IY_Plus_d_D()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_rr(GetMem(a));
}

void _RR_At_IY_Plus_d_E()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_rr(GetMem(a));
}

void _RR_At_IY_Plus_d_H()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_rr(GetMem(a));
}

void _RR_At_IY_Plus_d_L()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_rr(GetMem(a));
}

void _RR_At_IY_Plus_d_A()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_rr(GetMem(a));
}

void _SLA_At_IY_Plus_d_B()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sla(GetMem(a));
}

void _SLA_At_IY_Plus_d_C()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_sla(GetMem(a));
}

void _SLA_At_IY_Plus_d_D()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_sla(GetMem(a));
}

void _SLA_At_IY_Plus_d_E()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_sla(GetMem(a));
}

void _SLA_At_IY_Plus_d_H()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_sla(GetMem(a));
}

void _SLA_At_IY_Plus_d_L()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_sla(GetMem(a));
}

void _SLA_At_IY_Plus_d_A()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sla(GetMem(a));
}

void _SRA_At_IY_Plus_d_B()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sra(GetMem(a));
}

void _SRA_At_IY_Plus_d_C()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_sra(GetMem(a));
}

void _SRA_At_IY_Plus_d_D()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_sra(GetMem(a));
}

void _SRA_At_IY_Plus_d_E()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_sra(GetMem(a));
}

void _SRA_At_IY_Plus_d_H()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_sra(GetMem(a));
}

void _SRA_At_IY_Plus_d_L()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_sra(GetMem(a));
}

void _SRA_At_IY_Plus_d_A()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sra(GetMem(a));
}

void _SLL_At_IY_Plus_d_B()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sll(GetMem(a));
}

void _SLL_At_IY_Plus_d_C()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_sll(GetMem(a));
}

void _SLL_At_IY_Plus_d_D()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_sll(GetMem(a));
}

void _SLL_At_IY_Plus_d_E()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_sll(GetMem(a));
}

void _SLL_At_IY_Plus_d_H()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_sll(GetMem(a));
}

void _SLL_At_IY_Plus_d_L()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_sll(GetMem(a));
}

void _SLL_At_IY_Plus_d_A()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_sll(GetMem(a));
}

void _SRL_At_IY_Plus_d_B()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_srl(GetMem(a));
}

void _SRL_At_IY_Plus_d_C()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.c = cb_srl(GetMem(a));
}

void _SRL_At_IY_Plus_d_D()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.d = cb_srl(GetMem(a));
}

void _SRL_At_IY_Plus_d_E()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.e = cb_srl(GetMem(a));
}

void _SRL_At_IY_Plus_d_H()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.h = cb_srl(GetMem(a));
}

void _SRL_At_IY_Plus_d_L()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.l = cb_srl(GetMem(a));
}

void _SRL_At_IY_Plus_d_A()
{
  word a = cpu.regs.wordregs.iy + (char)GetMem(cpu.pc+2);
  cpu.regs.byteregs.b = cb_srl(GetMem(a));
}

/////////////////////////////////////////////////
InstType inst_FDCB[] = {
  {4, 23, _RLC_At_IY_Plus_d_B}, // 0x00 *
  {4, 23, _RLC_At_IY_Plus_d_C}, // 0x01 *
  {4, 23, _RLC_At_IY_Plus_d_D}, // 0x02 *
  {4, 23, _RLC_At_IY_Plus_d_E}, // 0x03 *
  {4, 23, _RLC_At_IY_Plus_d_H}, // 0x04 *
  {4, 23, _RLC_At_IY_Plus_d_L}, // 0x05 *
  {4, 23, _RLC_At_IY_Plus_d},   // 0x06
  {4, 23, _RLC_At_IY_Plus_d_A}, // 0x07 *
  {4, 23, _RRC_At_IY_Plus_d_B}, // 0x08 *
  {4, 23, _RRC_At_IY_Plus_d_C}, // 0x09 *
  {4, 23, _RRC_At_IY_Plus_d_D}, // 0x0A *
  {4, 23, _RRC_At_IY_Plus_d_E}, // 0x0B *
  {4, 23, _RRC_At_IY_Plus_d_H}, // 0x0C *
  {4, 23, _RRC_At_IY_Plus_d_L}, // 0x0D *
  {4, 23, _RRC_At_IY_Plus_d},   // 0x0E
  {4, 23, _RRC_At_IY_Plus_d_A}, // 0x0F *
  {4, 23, _RL_At_IY_Plus_d_B},  // 0x10 *
  {4, 23, _RL_At_IY_Plus_d_C},  // 0x11 *
  {4, 23, _RL_At_IY_Plus_d_D},  // 0x12 *
  {4, 23, _RL_At_IY_Plus_d_E},  // 0x13 *
  {4, 23, _RL_At_IY_Plus_d_H},  // 0x14 *
  {4, 23, _RL_At_IY_Plus_d_L},  // 0x15 *
  {4, 23, _RL_At_IY_Plus_d},    // 0x16
  {4, 23, _RL_At_IY_Plus_d_A},  // 0x17 *
  {4, 23, _RR_At_IY_Plus_d_B},  // 0x18 *
  {4, 23, _RR_At_IY_Plus_d_C},  // 0x19 *
  {4, 23, _RR_At_IY_Plus_d_D},  // 0x1A *
  {4, 23, _RR_At_IY_Plus_d_E},  // 0x1B *
  {4, 23, _RR_At_IY_Plus_d_H},  // 0x1C *
  {4, 23, _RR_At_IY_Plus_d_L},  // 0x1D *
  {4, 23, _RR_At_IY_Plus_d},    // 0x1E
  {4, 23, _RR_At_IY_Plus_d_A},  // 0x1F *
  {4, 23, _SLA_At_IY_Plus_d_B}, // 0x20 *
  {4, 23, _SLA_At_IY_Plus_d_C}, // 0x21 *
  {4, 23, _SLA_At_IY_Plus_d_D}, // 0x22 *
  {4, 23, _SLA_At_IY_Plus_d_E}, // 0x23 *
  {4, 23, _SLA_At_IY_Plus_d_H}, // 0x24 *
  {4, 23, _SLA_At_IY_Plus_d_L}, // 0x25 *
  {4, 23, _SLA_At_IY_Plus_d},   // 0x26
  {4, 23, _SLA_At_IY_Plus_d_A}, // 0x27 *
  {4, 23, _SRA_At_IY_Plus_d_B}, // 0x28 *
  {4, 23, _SRA_At_IY_Plus_d_C}, // 0x29 *
  {4, 23, _SRA_At_IY_Plus_d_D}, // 0x2A *
  {4, 23, _SRA_At_IY_Plus_d_E}, // 0x2B *
  {4, 23, _SRA_At_IY_Plus_d_H}, // 0x2C *
  {4, 23, _SRA_At_IY_Plus_d_L}, // 0x2D *
  {4, 23, _SRA_At_IY_Plus_d},   // 0x2E
  {4, 23, _SRA_At_IY_Plus_d_A}, // 0x2F *
  {4, 23, _SLL_At_IY_Plus_d_B}, // 0x30 *
  {4, 23, _SLL_At_IY_Plus_d_C}, // 0x31 *
  {4, 23, _SLL_At_IY_Plus_d_D}, // 0x32 *
  {4, 23, _SLL_At_IY_Plus_d_E}, // 0x33 *
  {4, 23, _SLL_At_IY_Plus_d_H}, // 0x34 *
  {4, 23, _SLL_At_IY_Plus_d_L}, // 0x35 *
  {4, 23, _SLL_At_IY_Plus_d},   // 0x36 *
  {4, 23, _SLL_At_IY_Plus_d_A}, // 0x37 *
  {4, 23, _SRL_At_IY_Plus_d_B}, // 0x38 *
  {4, 23, _SRL_At_IY_Plus_d_C}, // 0x39 *
  {4, 23, _SRL_At_IY_Plus_d_D}, // 0x3A *
  {4, 23, _SRL_At_IY_Plus_d_E}, // 0x3B *
  {4, 23, _SRL_At_IY_Plus_d_H}, // 0x3C *
  {4, 23, _SRL_At_IY_Plus_d_L}, // 0x3D *
  {4, 23, _SRL_At_IY_Plus_d},   // 0x3E
  {4, 23, _SRL_At_IY_Plus_d_A}, // 0x3F *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x40 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x41 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x42 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x43 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x44 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x45 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x46
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x47 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x48 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x49 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x4A *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x4B *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x4C *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x4D *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x4E
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x4F *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x50 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x51 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x52 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x53 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x54 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x55 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x56
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x57 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x58 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x59 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x5A *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x5B *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x5C *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x5D *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x5E
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x5F *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x60 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x61 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x62 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x63 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x64 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x65 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x66
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x67 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x68 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x69 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x6A *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x6B *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x6C *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x6D *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x6E
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x6F *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x70 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x71 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x72 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x73 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x74 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x75 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x76
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x77 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x78 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x79 *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x7A *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x7B *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x7C *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x7D *
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x7E
  {4, 20, _BIT_b_At_IY_Plus_d}, // 0x7F *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x80 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x81 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x82 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x83 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x84 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x85 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x86
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x87 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x88 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x89 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x8A *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x8B *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x8C *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x8D *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x8E
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x8F *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x90 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x91 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x92 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x93 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x94 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x95 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x96
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x97 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x98 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x99 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x9A *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x9B *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x9C *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x9D *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x9E
  {4, 23, _RES_b_At_IY_Plus_d}, // 0x9F *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xA0 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xA1 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xA2 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xA3 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xA4 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xA5 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xA6
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xA7 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xA8 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xA9 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xAA *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xAB *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xAC *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xAD *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xAE
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xAF *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xB0 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xB1 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xB2 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xB3 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xB4 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xB5 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xB6
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xB7 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xB8 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xB9 *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xBA *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xBB *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xBC *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xBD *
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xBE
  {4, 23, _RES_b_At_IY_Plus_d}, // 0xBF *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xC0 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xC1 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xC2 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xC3 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xC4 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xC5 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xC6
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xC7 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xC8 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xC9 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xCA *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xCB *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xCC *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xCD *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xCE
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xCF *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xD0 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xD1 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xD2 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xD3 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xD4 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xD5 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xD6
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xD7 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xD8 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xD9 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xDA *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xDB *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xDC *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xDD *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xDE
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xDF *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xE0 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xE1 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xE2 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xE3 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xE4 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xE5 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xE6
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xE7 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xE8 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xE9 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xEA *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xEB *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xEC *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xED *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xEE
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xEF *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xF0 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xF1 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xF2 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xF3 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xF4 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xF5 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xF6
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xF7 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xF8 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xF9 *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xFA *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xFB *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xFC *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xFD *
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xFE
  {4, 23, _SET_b_At_IY_Plus_d}, // 0xFF *
};

void _OPCODE_FD_CB()
{
  BYTE by = GetMem(cpu.pc+3);

  if (inst_FDCB[by].bytes == 0)
  {
    inst_FDCB[by].inst();
  }
  else
  {
    word pc = cpu.pc;
    inst_FDCB[by].inst();
    cpu.pc = pc + inst_FDCB[by].bytes;
    cpu.cycles += inst_FDCB[by].cycles;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void _ADD_A_IYL()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.iyl, 0);
}

void _ADD_A_IYH()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.iyh, 0);
}

void _ADC_A_IYL()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.iyl, getflag(C_FLAG_MASK));
}

void _ADC_A_IYH()
{
  cpu.regs.byteregs.a = addb(cpu.regs.byteregs.a, cpu.regs.byteregs.iyh, getflag(C_FLAG_MASK));
}

void _SUB_A_IYL()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.iyl, 0);
}

void _SUB_A_IYH()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.iyh, 0);
}

void _SBC_A_IYL()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.iyl, getflag(C_FLAG_MASK));
}

void _SBC_A_IYH()
{
  cpu.regs.byteregs.a = subb(cpu.regs.byteregs.a, cpu.regs.byteregs.iyh, getflag(C_FLAG_MASK));
}

void _AND_A_IYL()
{
  land(cpu.regs.byteregs.iyl);
}

void _AND_A_IYH()
{
  land(cpu.regs.byteregs.iyh);
}

void _XOR_A_IYL()
{
  lxor(cpu.regs.byteregs.iyl);
}

void _XOR_A_IYH()
{
  lxor(cpu.regs.byteregs.iyh);
}

void _OR_A_IYL()
{
  lor(cpu.regs.byteregs.iyl);
}

void _OR_A_IYH()
{
  lor(cpu.regs.byteregs.iyh);
}

void _CP_A_IYL()
{
  cp(cpu.regs.byteregs.iyl);
}

void _CP_A_IYH()
{
  cp(cpu.regs.byteregs.iyh);
}

void _LD_IYL_B()
{
  cpu.regs.byteregs.iyl = cpu.regs.byteregs.b;
}

void _LD_IYL_C()
{
  cpu.regs.byteregs.iyl = cpu.regs.byteregs.c;
}

void _LD_IYL_D()
{
  cpu.regs.byteregs.iyl = cpu.regs.byteregs.d;
}

void _LD_IYL_E()
{
  cpu.regs.byteregs.iyl = cpu.regs.byteregs.e;
}

void _LD_IYL_H()
{
  cpu.regs.byteregs.iyl = cpu.regs.byteregs.h;
}

void _LD_IYL_L()
{
  cpu.regs.byteregs.iyl = cpu.regs.byteregs.l;
}

void _LD_IYL_A()
{
  cpu.regs.byteregs.iyl = cpu.regs.byteregs.a;
}

void _LD_IYH_B()
{
  cpu.regs.byteregs.iyh = cpu.regs.byteregs.b;
}

void _LD_IYH_C()
{
  cpu.regs.byteregs.iyh = cpu.regs.byteregs.c;
}

void _LD_IYH_D()
{
  cpu.regs.byteregs.iyh = cpu.regs.byteregs.d;
}

void _LD_IYH_E()
{
  cpu.regs.byteregs.iyh = cpu.regs.byteregs.e;
}

void _LD_IYH_H()
{
  cpu.regs.byteregs.iyh = cpu.regs.byteregs.h;
}

void _LD_IYH_L()
{
  cpu.regs.byteregs.iyh = cpu.regs.byteregs.l;
}

void _LD_IYH_A()
{
  cpu.regs.byteregs.iyh = cpu.regs.byteregs.a;
}

void _INC_IYH()
{
  cpu.regs.byteregs.iyh = inc(cpu.regs.byteregs.iyh);
}

void _INC_IYL()
{
  cpu.regs.byteregs.iyl = inc(cpu.regs.byteregs.iyl);
}

void _DEC_IYH()
{
  cpu.regs.byteregs.iyh = dec(cpu.regs.byteregs.iyh);
}

void _DEC_IYL()
{
  cpu.regs.byteregs.iyl = dec(cpu.regs.byteregs.iyl);
}

void _LD_IYH_n()
{
  cpu.regs.byteregs.iyh = GetMem(cpu.pc+2);
}

void _LD_IYL_n()
{
  cpu.regs.byteregs.iyl = GetMem(cpu.pc+2);
}

void _LD_B_IYH()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.iyh;
}

void _LD_B_IYL()
{
  cpu.regs.byteregs.b = cpu.regs.byteregs.iyl;
}

void _LD_C_IYH()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.iyh;
}

void _LD_C_IYL()
{
  cpu.regs.byteregs.c = cpu.regs.byteregs.iyl;
}

void _LD_D_IYH()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.iyh;
}

void _LD_D_IYL()
{
  cpu.regs.byteregs.d = cpu.regs.byteregs.iyl;
}

void _LD_E_IYH()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.iyh;
}

void _LD_E_IYL()
{
  cpu.regs.byteregs.e = cpu.regs.byteregs.iyl;
}

void _LD_IYH_IYH()
{
  cpu.regs.byteregs.iyh = cpu.regs.byteregs.iyh;
}

void _LD_IYH_IYL()
{
  cpu.regs.byteregs.iyh = cpu.regs.byteregs.iyl;
}

void _LD_IYL_IYH()
{
  cpu.regs.byteregs.iyl = cpu.regs.byteregs.iyh;
}

void _LD_IYL_IYL()
{
  cpu.regs.byteregs.iyl = cpu.regs.byteregs.iyl;
}

void _LD_A_IYH()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.iyh;
}

void _LD_A_IYL()
{
  cpu.regs.byteregs.a = cpu.regs.byteregs.iyl;
}

/////////////////////////////////////////////////
InstType inst_FD[] = {
  {1, 4,  _UNDEF},              // 0x00
  {1, 4,  _UNDEF},              // 0x01
  {1, 4,  _UNDEF},              // 0x02
  {1, 4,  _UNDEF},              // 0x03
  {2, 8,  _INC_B},              // 0x04 *
  {2, 8,  _DEC_B},              // 0x05 *
  {3, 11, _LD_B_n},             // 0x06 *
  {1, 4,  _UNDEF},              // 0x07
  {1, 4,  _UNDEF},              // 0x08
  {2, 15, _ADD_IY_pp},          // 0x09
  {1, 4,  _UNDEF},              // 0x0A
  {1, 4,  _UNDEF},              // 0x0B
  {2, 8,  _INC_C},              // 0x0C *
  {2, 8,  _DEC_C},              // 0x0D *
  {3, 11, _LD_C_n},             // 0x0E *
  {1, 4,  _UNDEF},              // 0x0F
  {1, 4,  _UNDEF},              // 0x10
  {1, 4,  _UNDEF},              // 0x11
  {1, 4,  _UNDEF},              // 0x12
  {1, 4,  _UNDEF},              // 0x13
  {2, 8,  _INC_D},              // 0x14 *
  {2, 8,  _DEC_D},              // 0x15 *
  {3, 11, _LD_D_n},             // 0x16 *
  {1, 4,  _UNDEF},              // 0x17
  {1, 4,  _UNDEF},              // 0x18
  {2, 15, _ADD_IY_pp},          // 0x19
  {1, 4,  _UNDEF},              // 0x1A
  {1, 4,  _UNDEF},              // 0x1B
  {2, 8,  _INC_E},              // 0x1C *
  {2, 8,  _DEC_E},              // 0x1D *
  {3, 11, _LD_E_n},             // 0x1E *
  {1, 4,  _UNDEF},              // 0x1F
  {1, 4,  _UNDEF},              // 0x20
  {4, 14, _LD_IY_nn},           // 0x21
  {4, 20, _LD_At_nn_IY},        // 0x22
  {2, 10, _INC_IY},             // 0x23
  {2, 8,  _INC_IYH},            // 0x24 *
  {2, 8,  _INC_IYL},            // 0x25 *
  {3, 11, _LD_IYH_n},           // 0x26 *
  {1, 4,  _UNDEF},              // 0x27
  {1, 4,  _UNDEF},              // 0x28
  {2, 15, _ADD_IY_pp},          // 0x29
  {4, 20, _LD_IY_At_nn},        // 0x2A
  {2, 10, _DEC_IY},             // 0x2B
  {2, 8,  _DEC_IYH},            // 0x2C *
  {2, 8,  _DEC_IYL},            // 0x2D *
  {3, 11, _LD_IYL_n},           // 0x2E *
  {1, 4,  _UNDEF},              // 0x2F
  {1, 4,  _UNDEF},              // 0x30
  {1, 4,  _UNDEF},              // 0x31
  {1, 4,  _UNDEF},              // 0x32
  {1, 4,  _UNDEF},              // 0x33
  {3, 23, _INC_At_IY_Plus_d},   // 0x34
  {3, 23, _DEC_At_IY_Plus_d},   // 0x35
  {4, 19, _LD_At_IY_Plus_d_n},  // 0x36
  {1, 4,  _UNDEF},              // 0x37
  {1, 4,  _UNDEF},              // 0x38
  {2, 15, _ADD_IY_pp},          // 0x39
  {1, 4,  _UNDEF},              // 0x3A
  {1, 4,  _UNDEF},              // 0x3B
  {2, 8,  _INC_A},              // 0x3C *
  {2, 8,  _DEC_A},              // 0x3D *
  {3, 11, _LD_A_n},             // 0x3E *
  {1, 4,  _UNDEF},              // 0x3F
  {2, 8,  _LD_B_B},             // 0x40 *
  {2, 8,  _LD_B_C},             // 0x41 *
  {2, 8,  _LD_B_D},             // 0x42 *
  {2, 8,  _LD_B_E},             // 0x43 *
  {2, 8,  _LD_B_IYH},           // 0x44 *
  {2, 8,  _LD_B_IYL},           // 0x45 *
  {3, 19, _LD_r_At_IY_Plus_d},  // 0x46
  {2, 8,  _LD_B_A},             // 0x47 *
  {2, 8,  _LD_C_B},             // 0x48 *
  {2, 8,  _LD_C_C},             // 0x49 *
  {2, 8,  _LD_C_D},             // 0x4A *
  {2, 8,  _LD_C_E},             // 0x4B *
  {2, 8,  _LD_C_IYH},           // 0x4C *
  {2, 8,  _LD_C_IYL},           // 0x4D *
  {3, 19, _LD_r_At_IY_Plus_d},  // 0x4E
  {2, 8,  _LD_C_A},             // 0x4F *
  {2, 8,  _LD_D_B},             // 0x50 *
  {2, 8,  _LD_D_C},             // 0x51 *
  {2, 8,  _LD_D_D},             // 0x52 *
  {2, 8,  _LD_D_E},             // 0x53 *
  {2, 8,  _LD_D_IYH},           // 0x54 *
  {2, 8,  _LD_D_IYL},           // 0x55 *
  {3, 19, _LD_r_At_IY_Plus_d},  // 0x56
  {2, 8,  _LD_D_A},             // 0x57 *
  {2, 8,  _LD_E_B},             // 0x58 *
  {2, 8,  _LD_E_C},             // 0x59 *
  {2, 8,  _LD_E_D},             // 0x5A *
  {2, 8,  _LD_E_E},             // 0x5B *
  {2, 8,  _LD_E_IYH},           // 0x5C *
  {2, 8,  _LD_E_IYL},           // 0x5D *
  {3, 19, _LD_r_At_IY_Plus_d},  // 0x5E
  {2, 8,  _LD_E_A},             // 0x5F *
  {2, 8,  _LD_IYH_B},           // 0x60 *
  {2, 8,  _LD_IYH_C},           // 0x61 *
  {2, 8,  _LD_IYH_D},           // 0x62 *
  {2, 8,  _LD_IYH_E},           // 0x63 *
  {2, 8,  _LD_IYH_IYH},         // 0x64 *
  {2, 8,  _LD_IYH_IYL},         // 0x65 *
  {3, 19, _LD_r_At_IY_Plus_d},  // 0x66
  {2, 8,  _LD_IYH_A},           // 0x67 *
  {2, 8,  _LD_IYL_B},           // 0x68 *
  {2, 8,  _LD_IYL_C},           // 0x69 *
  {2, 8,  _LD_IYL_D},           // 0x6A *
  {2, 8,  _LD_IYL_E},           // 0x6B *
  {2, 8,  _LD_IYL_IYH},         // 0x6C *
  {2, 8,  _LD_IYL_IYL},         // 0x6D *
  {3, 19, _LD_r_At_IY_Plus_d},  // 0x6E
  {2, 8,  _LD_IYL_A},           // 0x6F *
  {3, 19, _LD_At_IY_Plus_d_r},  // 0x70
  {3, 19, _LD_At_IY_Plus_d_r},  // 0x71
  {3, 19, _LD_At_IY_Plus_d_r},  // 0x72
  {3, 19, _LD_At_IY_Plus_d_r},  // 0x73
  {3, 19, _LD_At_IY_Plus_d_r},  // 0x74
  {3, 19, _LD_At_IY_Plus_d_r},  // 0x75
  {1, 4,  _UNDEF},              // 0x76
  {3, 19, _LD_At_IY_Plus_d_r},  // 0x77
  {2, 8,  _LD_A_B},             // 0x78 *
  {2, 8,  _LD_A_C},             // 0x79 *
  {2, 8,  _LD_A_D},             // 0x7A *
  {2, 8,  _LD_A_E},             // 0x7B *
  {2, 8,  _LD_A_IYH},           // 0x7C *
  {2, 8,  _LD_A_IYL},           // 0x7D *
  {3, 19, _LD_r_At_IY_Plus_d},  // 0x7E
  {2, 8,  _LD_A_A},             // 0x7F *
  {2, 8,  _ADD_A_B},            // 0x80 *
  {2, 8,  _ADD_A_C},            // 0x81 *
  {2, 8,  _ADD_A_D},            // 0x82 *
  {2, 8,  _ADD_A_E},            // 0x83 *
  {2, 8,  _ADD_A_IYH},          // 0x84 *
  {2, 8,  _ADD_A_IYL},          // 0x85 *
  {3, 19, _ADD_A_At_IY_Plus_d}, // 0x86
  {2, 8,  _ADD_A_A},            // 0x87 *
  {2, 8,  _ADC_A_B},            // 0x88 *
  {2, 8,  _ADC_A_C},            // 0x89 *
  {2, 8,  _ADC_A_D},            // 0x8A *
  {2, 8,  _ADC_A_E},            // 0x8B *
  {2, 8,  _ADC_A_IYH},          // 0x8C *
  {2, 8,  _ADC_A_IYL},          // 0x8D *
  {3, 19, _ADC_A_At_IY_Plus_d}, // 0x8E
  {2, 8,  _ADC_A_A},            // 0x8F *
  {2, 8,  _SUB_A_B},            // 0x90 *
  {2, 8,  _SUB_A_C},            // 0x91 *
  {2, 8,  _SUB_A_D},            // 0x92 *
  {2, 8,  _SUB_A_E},            // 0x93 *
  {2, 8,  _SUB_A_IYH},          // 0x94 *
  {2, 8,  _SUB_A_IYL},          // 0x95 *
  {3, 19, _SUB_A_At_IY_Plus_d}, // 0x96
  {2, 8,  _SUB_A_A},            // 0x97 *
  {2, 8,  _SBC_A_B},            // 0x98 *
  {2, 8,  _SBC_A_C},            // 0x99 *
  {2, 8,  _SBC_A_D},            // 0x9A *
  {2, 8,  _SBC_A_E},            // 0x9B *
  {2, 8,  _SBC_A_IYH},          // 0x9C *
  {2, 8,  _SBC_A_IYL},          // 0x9D *
  {3, 19, _SBC_A_At_IY_Plus_d}, // 0x9E
  {2, 8,  _SBC_A_A},            // 0x9F *
  {2, 8,  _AND_A_B},            // 0xA0 *
  {2, 8,  _AND_A_C},            // 0xA1 *
  {2, 8,  _AND_A_D},            // 0xA2 *
  {2, 8,  _AND_A_E},            // 0xA3 *
  {2, 8,  _AND_A_IYH},          // 0xA4 *
  {2, 8,  _AND_A_IYL},          // 0xA5 *
  {3, 19, _AND_A_At_IY_Plus_d}, // 0xA6
  {2, 8,  _AND_A_A},            // 0xA7 *
  {2, 8,  _XOR_A_B},            // 0xA8 *
  {2, 8,  _XOR_A_C},            // 0xA9 *
  {2, 8,  _XOR_A_D},            // 0xAA *
  {2, 8,  _XOR_A_E},            // 0xAB *
  {2, 8,  _XOR_A_IYH},          // 0xAC *
  {2, 8,  _XOR_A_IYL},          // 0xAD *
  {3, 19, _XOR_A_At_IY_Plus_d}, // 0xAE
  {2, 8,  _XOR_A_A},            // 0xAF *
  {2, 8,  _OR_A_B},             // 0xB0 *
  {2, 8,  _OR_A_C},             // 0xB1 *
  {2, 8,  _OR_A_D},             // 0xB2 *
  {2, 8,  _OR_A_E},             // 0xB3 *
  {2, 8,  _OR_A_IYH},           // 0xB4 *
  {2, 8,  _OR_A_IYL},           // 0xB5 *
  {3, 19, _OR_A_At_IY_Plus_d},  // 0xB6
  {2, 8,  _OR_A_A},             // 0xB7 *
  {2, 8,  _CP_B},               // 0xB8 *
  {2, 8,  _CP_C},               // 0xB9 *
  {2, 8,  _CP_D},               // 0xBA *
  {2, 8,  _CP_E},               // 0xBB *
  {2, 8,  _CP_A_IYH},           // 0xBC *
  {2, 8,  _CP_A_IYL},           // 0xBD *
  {3, 19, _CP_At_IY_Plus_d},    // 0xBE
  {2, 8,  _CP_A},               // 0xBF *
  {1, 4,  _UNDEF},              // 0xC0
  {1, 4,  _UNDEF},              // 0xC1
  {1, 4,  _UNDEF},              // 0xC2
  {1, 4,  _UNDEF},              // 0xC3
  {1, 4,  _UNDEF},              // 0xC4
  {1, 4,  _UNDEF},              // 0xC5
  {1, 4,  _UNDEF},              // 0xC6
  {1, 4,  _UNDEF},              // 0xC7
  {1, 4,  _UNDEF},              // 0xC8
  {1, 4,  _UNDEF},              // 0xC9
  {1, 4,  _UNDEF},              // 0xCA
  {0, 0,  _OPCODE_FD_CB},       // 0xCB
  {1, 4,  _UNDEF},              // 0xCC
  {1, 4,  _UNDEF},              // 0xCD
  {1, 4,  _UNDEF},              // 0xCE
  {1, 4,  _UNDEF},              // 0xCF
  {1, 4,  _UNDEF},              // 0xD0
  {1, 4,  _UNDEF},              // 0xD1
  {1, 4,  _UNDEF},              // 0xD2
  {1, 4,  _UNDEF},              // 0xD3
  {1, 4,  _UNDEF},              // 0xD4
  {1, 4,  _UNDEF},              // 0xD5
  {1, 4,  _UNDEF},              // 0xD6
  {1, 4,  _UNDEF},              // 0xD7
  {1, 4,  _UNDEF},              // 0xD8
  {1, 4,  _UNDEF},              // 0xD9
  {1, 4,  _UNDEF},              // 0xDA
  {1, 4,  _UNDEF},              // 0xDB
  {1, 4,  _UNDEF},              // 0xDC
  {1, 4,  _UNDEF},              // 0xDD
  {1, 4,  _UNDEF},              // 0xDE
  {1, 4,  _UNDEF},              // 0xDF
  {1, 4,  _UNDEF},              // 0xE0
  {2, 14, _POP_IY},             // 0xE1
  {1, 4,  _UNDEF},              // 0xE2
  {2, 23, _EX_At_SP_IY},        // 0xE3
  {1, 4,  _UNDEF},              // 0xE4
  {2, 15, _PUSH_IY},            // 0xE5
  {1, 4,  _UNDEF},              // 0xE6
  {1, 4,  _UNDEF},              // 0xE7
  {1, 4,  _UNDEF},              // 0xE8
  {0, 0,  _JP_At_IY},           // 0xE9
  {1, 4,  _UNDEF},              // 0xEA
  {1, 4,  _UNDEF},              // 0xEB
  {1, 4,  _UNDEF},              // 0xEC
  {1, 4,  _UNDEF},              // 0xED
  {1, 4,  _UNDEF},              // 0xEE
  {1, 4,  _UNDEF},              // 0xEF
  {1, 4,  _UNDEF},              // 0xF0
  {1, 4,  _UNDEF},              // 0xF1
  {1, 4,  _UNDEF},              // 0xF2
  {1, 4,  _UNDEF},              // 0xF3
  {1, 4,  _UNDEF},              // 0xF4
  {1, 4,  _UNDEF},              // 0xF5
  {1, 4,  _UNDEF},              // 0xF6
  {1, 4,  _UNDEF},              // 0xF7
  {1, 4,  _UNDEF},              // 0xF8
  {2, 10, _LD_SP_IY},           // 0xF9
  {1, 4,  _UNDEF},              // 0xFA
  {1, 4,  _UNDEF},              // 0xFB
  {1, 4,  _UNDEF},              // 0xFC
  {1, 4,  _UNDEF},              // 0xFD
  {1, 4,  _UNDEF},              // 0xFE
  {1, 4,  _UNDEF},              // 0xFF
};

void _OPCODE_FD()
{
  byte by = GetMem(cpu.pc+1);

  if (inst_FD[by].bytes == 0)
  {
    inst_FD[by].inst();
  }
  else
  {
    word pc = cpu.pc;
    inst_FD[by].inst();
    cpu.pc = pc + inst_FD[by].bytes;
    cpu.cycles += inst_FD[by].cycles;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

InstType inst[] = {
  {1, 4,  _NOP},          // 0x00
  {3, 10, _LD_BC_nn},     // 0x01
  {1, 7,  _LD_At_BC_A},   // 0x02
  {1, 6,  _INC_BC},       // 0x03
  {1, 4,  _INC_B},        // 0x04
  {1, 4,  _DEC_B},        // 0x05
  {2, 7,  _LD_B_n},       // 0x06
  {1, 4,  _RLCA},         // 0x07
  {1, 4,  _EX_AF_Alt_AF}, // 0x08
  {1, 11, _ADD_HL_BC},    // 0x09
  {1, 7,  _LD_A_At_BC},   // 0x0A
  {1, 6,  _DEC_BC},       // 0x0B
  {1, 4,  _INC_C},        // 0x0C
  {1, 4,  _DEC_C},        // 0x0D
  {2, 7,  _LD_C_n},       // 0x0E
  {1, 4,  _RRCA},         // 0x0F
  {0, 0,  _DJNZ_e},       // 0x10
  {3, 10, _LD_DE_nn},     // 0x11
  {1, 7,  _LD_At_DE_A},   // 0x12
  {1, 6,  _INC_DE},       // 0x13
  {1, 4,  _INC_D},        // 0x14
  {1, 4,  _DEC_D},        // 0x15
  {2, 7,  _LD_D_n},       // 0x16
  {1, 4,  _RLA},          // 0x17
  {0, 0,  _JR_e},         // 0x18
  {1, 11, _ADD_HL_DE},    // 0x19
  {1, 7,  _LD_A_At_DE},   // 0x1A
  {1, 6,  _DEC_DE},       // 0x1B
  {1, 4,  _INC_E},        // 0x1C
  {1, 4,  _DEC_E},        // 0x1D
  {2, 7,  _LD_E_n},       // 0x1E
  {1, 4,  _RRA},          // 0x1F
  {0, 0,  _JR_NZ_e},      // 0x20
  {3, 10, _LD_HL_nn},     // 0x21
  {3, 16, _LD_At_nn_HL},  // 0x22
  {1, 6,  _INC_HL},       // 0x23
  {1, 4,  _INC_H},        // 0x24
  {1, 4,  _DEC_H},        // 0x25
  {2, 7,  _LD_H_n},       // 0x26
  {1, 4,  _DAA},          // 0x27
  {0, 0,  _JR_Z_e},       // 0x28
  {1, 11, _ADD_HL_HL},    // 0x29
  {3, 16, _LD_HL_At_nn},  // 0x2A
  {1, 6,  _DEC_HL},       // 0x2B
  {1, 4,  _INC_L},        // 0x2C
  {1, 4,  _DEC_L},        // 0x2D
  {2, 7,  _LD_L_n},       // 0x2E
  {1, 4,  _CPL},          // 0x2F
  {0, 0,  _JR_NC_e},      // 0x30
  {3, 10, _LD_SP_nn},     // 0x31
  {3, 13, _LD_At_nn_A},   // 0x32
  {1, 6,  _INC_SP},       // 0x33
  {1, 11, _INC_At_HL},    // 0x34
  {1, 11, _DEC_At_HL},    // 0x35
  {2, 10, _LD_At_HL_n},   // 0x36
  {1, 4,  _SCF},          // 0x37
  {0, 0,  _JR_C_e},       // 0x38
  {1, 11, _ADD_HL_SP},    // 0x39
  {3, 13, _LD_A_At_nn},   // 0x3A
  {1, 6,  _DEC_SP},       // 0x3B
  {1, 4,  _INC_A},        // 0x3C
  {1, 4,  _DEC_A},        // 0x3D
  {2, 7,  _LD_A_n},       // 0x3E
  {1, 4,  _CCF},          // 0x3F
  {1, 4,  _LD_B_B},       // 0x40
  {1, 4,  _LD_B_C},       // 0x41
  {1, 4,  _LD_B_D},       // 0x42
  {1, 4,  _LD_B_E},       // 0x43
  {1, 4,  _LD_B_H},       // 0x44
  {1, 4,  _LD_B_L},       // 0x45
  {1, 7,  _LD_B_At_HL},   // 0x46
  {1, 4,  _LD_B_A},       // 0x47
  {1, 4,  _LD_C_B},       // 0x48
  {1, 4,  _LD_C_C},       // 0x49
  {1, 4,  _LD_C_D},       // 0x4A
  {1, 4,  _LD_C_E},       // 0x4B
  {1, 4,  _LD_C_H},       // 0x4C
  {1, 4,  _LD_C_L},       // 0x4D
  {1, 7,  _LD_C_At_HL},   // 0x4E
  {1, 4,  _LD_C_A},       // 0x4F
  {1, 4,  _LD_D_B},       // 0x50
  {1, 4,  _LD_D_C},       // 0x51
  {1, 4,  _LD_D_D},       // 0x52
  {1, 4,  _LD_D_E},       // 0x53
  {1, 4,  _LD_D_H},       // 0x54
  {1, 4,  _LD_D_L},       // 0x55
  {1, 7,  _LD_D_At_HL},   // 0x56
  {1, 4,  _LD_D_A},       // 0x57
  {1, 4,  _LD_E_B},       // 0x58
  {1, 4,  _LD_E_C},       // 0x59
  {1, 4,  _LD_E_D},       // 0x5A
  {1, 4,  _LD_E_E},       // 0x5B
  {1, 4,  _LD_E_H},       // 0x5C
  {1, 4,  _LD_E_L},       // 0x5D
  {1, 7,  _LD_E_At_HL},   // 0x5E
  {1, 4,  _LD_E_A},       // 0x5F
  {1, 4,  _LD_H_B},       // 0x60
  {1, 4,  _LD_H_C},       // 0x61
  {1, 4,  _LD_H_D},       // 0x62
  {1, 4,  _LD_H_E},       // 0x63
  {1, 4,  _LD_H_H},       // 0x64
  {1, 4,  _LD_H_L},       // 0x65
  {1, 7,  _LD_H_At_HL},   // 0x66
  {1, 4,  _LD_H_A},       // 0x67
  {1, 4,  _LD_L_B},       // 0x68
  {1, 4,  _LD_L_C},       // 0x69
  {1, 4,  _LD_L_D},       // 0x6A
  {1, 4,  _LD_L_E},       // 0x6B
  {1, 4,  _LD_L_H},       // 0x6C
  {1, 4,  _LD_L_L},       // 0x6D
  {1, 7,  _LD_L_At_HL},   // 0x6E
  {1, 4,  _LD_L_A},       // 0x6F
  {1, 7,  _LD_At_HL_B},   // 0x70
  {1, 7,  _LD_At_HL_C},   // 0x71
  {1, 7,  _LD_At_HL_D},   // 0x72
  {1, 7,  _LD_At_HL_E},   // 0x73
  {1, 7,  _LD_At_HL_H},   // 0x74
  {1, 7,  _LD_At_HL_L},   // 0x75
  {1, 4,  _HALT},         // 0x76
  {1, 7,  _LD_At_HL_A},   // 0x77
  {1, 4,  _LD_A_B},       // 0x78
  {1, 4,  _LD_A_C},       // 0x79
  {1, 4,  _LD_A_D},       // 0x7A
  {1, 4,  _LD_A_E},       // 0x7B
  {1, 4,  _LD_A_H},       // 0x7C
  {1, 4,  _LD_A_L},       // 0x7D
  {1, 7,  _LD_A_At_HL},   // 0x7E
  {1, 4,  _LD_A_A},       // 0x7F
  {1, 4,  _ADD_A_B},      // 0x80
  {1, 4,  _ADD_A_C},      // 0x81
  {1, 4,  _ADD_A_D},      // 0x82
  {1, 4,  _ADD_A_E},      // 0x83
  {1, 4,  _ADD_A_H},      // 0x84
  {1, 4,  _ADD_A_L},      // 0x85
  {1, 7,  _ADD_A_At_HL},  // 0x86
  {1, 4,  _ADD_A_A},      // 0x87
  {1, 4,  _ADC_A_B},      // 0x88
  {1, 4,  _ADC_A_C},      // 0x89
  {1, 4,  _ADC_A_D},      // 0x8A
  {1, 4,  _ADC_A_E},      // 0x8B
  {1, 4,  _ADC_A_H},      // 0x8C
  {1, 4,  _ADC_A_L},      // 0x8D
  {1, 7,  _ADC_A_At_HL},  // 0x8E
  {1, 4,  _ADC_A_A},      // 0x8F
  {1, 4,  _SUB_A_B},      // 0x90
  {1, 4,  _SUB_A_C},      // 0x91
  {1, 4,  _SUB_A_D},      // 0x92
  {1, 4,  _SUB_A_E},      // 0x93
  {1, 4,  _SUB_A_H},      // 0x94
  {1, 4,  _SUB_A_L},      // 0x95
  {1, 7,  _SUB_A_At_HL},  // 0x96
  {1, 4,  _SUB_A_A},      // 0x97
  {1, 4,  _SBC_A_B},      // 0x98
  {1, 4,  _SBC_A_C},      // 0x99
  {1, 4,  _SBC_A_D},      // 0x9A
  {1, 4,  _SBC_A_E},      // 0x9B
  {1, 4,  _SBC_A_H},      // 0x9C
  {1, 4,  _SBC_A_L},      // 0x9D
  {1, 7,  _SBC_A_At_HL},  // 0x9E
  {1, 4,  _SBC_A_A},      // 0x9F
  {1, 4,  _AND_A_B},      // 0xA0
  {1, 4,  _AND_A_C},      // 0xA1
  {1, 4,  _AND_A_D},      // 0xA2
  {1, 4,  _AND_A_E},      // 0xA3
  {1, 4,  _AND_A_H},      // 0xA4
  {1, 4,  _AND_A_L},      // 0xA5
  {1, 7,  _AND_A_At_HL},  // 0xA6
  {1, 4,  _AND_A_A},      // 0xA7
  {1, 4,  _XOR_A_B},      // 0xA8
  {1, 4,  _XOR_A_C},      // 0xA9
  {1, 4,  _XOR_A_D},      // 0xAA
  {1, 4,  _XOR_A_E},      // 0xAB
  {1, 4,  _XOR_A_H},      // 0xAC
  {1, 4,  _XOR_A_L},      // 0xAD
  {1, 7,  _XOR_A_At_HL},  // 0xAE
  {1, 4,  _XOR_A_A},      // 0xAF
  {1, 4,  _OR_A_B},       // 0xB0
  {1, 4,  _OR_A_C},       // 0xB1
  {1, 4,  _OR_A_D},       // 0xB2
  {1, 4,  _OR_A_E},       // 0xB3
  {1, 4,  _OR_A_H},       // 0xB4
  {1, 4,  _OR_A_L},       // 0xB5
  {1, 7,  _OR_A_At_HL},   // 0xB6
  {1, 4,  _OR_A_A},       // 0xB7
  {1, 4,  _CP_B},         // 0xB8
  {1, 4,  _CP_C},         // 0xB9
  {1, 4,  _CP_D},         // 0xBA
  {1, 4,  _CP_E},         // 0xBB
  {1, 4,  _CP_H},         // 0xBC
  {1, 4,  _CP_L},         // 0xBD
  {1, 7,  _CP_At_HL},     // 0xBE
  {1, 4,  _CP_A},         // 0xBF
  {0, 0,  _RET_nz},       // 0xC0
  {1, 10, _POP_BC},       // 0xC1
  {0, 0,  _JP_nz_nn},     // 0xC2
  {0, 0,  _JP_nn},        // 0xC3
  {0, 0,  _CALL_nz_nn},   // 0xC4
  {1, 11, _PUSH_BC},      // 0xC5
  {2, 7,  _ADD_A_n},      // 0xC6
  {0, 0,  _RST_00},       // 0xC7
  {0, 0,  _RET_z},        // 0xC8
  {0, 0,  _RET},          // 0xC9
  {0, 0,  _JP_z_nn},      // 0xCA
  {0, 0,  _OPCODE_CB},    // 0xCB
  {0, 0,  _CALL_z_nn},    // 0xCC
  {0, 0,  _CALL_nn},      // 0xCD
  {2, 7,  _ADC_A_n},      // 0xCE
  {0, 0,  _RST_08},       // 0xCF
  {0, 0,  _RET_nc},       // 0xD0
  {1, 10, _POP_DE},       // 0xD1
  {0, 0,  _JP_nc_nn},     // 0xD2
  {2, 11, _OUT_n_A},      // 0xD3
  {0, 0,  _CALL_nc_nn},   // 0xD4
  {1, 11, _PUSH_DE},      // 0xD5
  {2, 7,  _SUB_A_n},      // 0xD6
  {0, 0,  _RST_10},       // 0xD7
  {0, 0,  _RET_c},        // 0xD8
  {1, 4,  _EXX},          // 0xD9
  {0, 0,  _JP_c_nn},      // 0xDA
  {2, 11, _IN_A},         // 0xDB
  {0, 0,  _CALL_c_nn},    // 0xDC
  {0, 0,  _OPCODE_DD},    // 0xDD
  {2, 7,  _SBC_A_n},      // 0xDE
  {0, 0,  _RST_18},       // 0xDF
  {0, 0,  _RET_npv},      // 0xE0
  {1, 10, _POP_HL},       // 0xE1
  {0, 0,  _JP_npv_nn},    // 0xE2
  {1, 19, _EX_At_SP_HL},  // 0xE3
  {0, 0,  _CALL_npv_nn},  // 0xE4
  {1, 11, _PUSH_HL},      // 0xE5
  {2, 7,  _AND_A_n},      // 0xE6
  {0, 0,  _RST_20},       // 0xE7
  {0, 0,  _RET_pv},       // 0xE8
  {0, 0,  _JP_At_HL},     // 0xE9
  {0, 0,  _JP_pv_nn},     // 0xEA
  {1, 4,  _EX_DE_HL},     // 0xEB
  {0, 0,  _CALL_pv_nn},   // 0xEC
  {0, 0,  _OPCODE_ED},    // 0xED
  {2, 7,  _XOR_A_n},      // 0xEE
  {0, 0,  _RST_28},       // 0xEF
  {0, 0,  _RET_ns},       // 0xF0
  {1, 10, _POP_AF},       // 0xF1
  {0, 0,  _JP_ns_nn},     // 0xF2
  {1, 4,  _DI},           // 0xF3
  {0, 0,  _CALL_ns_nn},   // 0xF4
  {1, 11, _PUSH_AF},      // 0xF5
  {2, 7,  _OR_A_n},       // 0xF6
  {0, 0,  _RST_30},       // 0xF7
  {0, 0,  _RET_s},        // 0xF8
  {1, 6,  _LD_SP_HL},     // 0xF9
  {0, 0 , _JP_s_nn},      // 0xFA
  {1, 4,  _EI},           // 0xFB
  {0, 0,  _CALL_s_nn},    // 0xFC
  {0, 0,  _OPCODE_FD},    // 0xFD
  {2, 7,  _CP_n},         // 0xFE
  {0, 0,  _RST_38},       // 0xFF
};

#ifdef ENABLE_LOGGING
/////////////////////////////////////////////////
void LogNextInstruction(word pc)
{
  char szBuf[128];
  char szInst[64];
	GetInstructionName(pc, szInst, sizeof(szInst)-2);

  sprintf_s(szBuf, sizeof(szBuf), "%04X ", pc);
  strcat_s(szBuf, sizeof(szBuf), szInst);

//  sprintf_s(szInst, sizeof(szInst)-2, "%02X %02X %02X %02X\r\n", GetMem(pc), GetMem(pc+1), GetMem(pc+2), GetMem(pc+3));
  strcat_s(szBuf, sizeof(szBuf), (char*)"\r\n"); //szInst);
  WriteLogFile(szBuf);
}
#endif

/////////////////////////////////////////////////

void EmuExecInst(byte by)
{
  if (inst[by].bytes == 0)
  {
    inst[by].inst();
  }
  else
  {
    word pc = cpu.pc;
    inst[by].inst();
    cpu.pc = pc + inst[by].bytes;
    cpu.cycles += inst[by].cycles;
  }
}

// NMI call address 0x0066
void EmuProcessIntr(byte by)
{
  // The purpose of IFF2 is to save the status of IFF1 when a nonmaskable interrupt occurs. 
  // When a nonmaskable interrupt is accepted, IFF1 resets to prevent further interrupts until 
  // reenabled by the programmer. Therefore, after a nonmaskable interrupt is accepted, mask-
  // able interrupts are disabled but the previous state of IFF1 is saved so that the complete 
  // state of the CPU just prior to the nonmaskable interrupt can be restored at any time. When 
  // a Load Register A with Register I (LD A, I) instruction or a Load Register A with Register 
  // R (LD A, R) instruction is executed, the state of IFF2 is copied to the parity flag, where it 
  // can be tested or stored.
  // 
  // A second method of restoring the status of IFF1 is through the execution of a Return From 
  // Nonmaskable Interrupt (RETN) instruction. This instruction indicates that the nonmask-
  // able interrupt service routine is complete and the contents of IFF2 are now copied back 
  // into IFF1 so that the status of IFF1 just prior to the acceptance of the nonmaskable inter-
  // rupt is restored automatically.

  // one instruction is executed before the interrupt is processed
  EmuExecInst(by);

  cpu.intr = 0;
  cpu.iff1 = 0;
  cpu.iff2 = 0;

  switch (cpu.imode)
  {
    case 0: // instruction to be executed is inserted on the data bus by the interrupting device
      ASSERT(FALSE);
      break;

    case 1: // RST 0x0038
      WriteMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
      WriteMem(cpu.sp-2, cpu.pc & 0xFF);

      cpu.pc = 0x0038;
      cpu.sp -= 2;
      cpu.cycles += 11;
      break;

    case 2: // The IM 2 instruction sets the vectored Interrupt Mode 2. This mode allows an indirect call 
            // to any memory location by an 8-bit vector supplied from the peripheral device. This vector 
            // then becomes the least-significant eight bits of the indirect pointer, while the I Register in 
            // the CPU provides the most-significant eight bits. This address points to an address in a 
            // vector table that is the starting address for the interrupt service routine.
      ASSERT(FALSE);
      break;
  }
}

void EmuProcessNmi(void)
{
    WriteMem(cpu.sp-1, (cpu.pc >> 8) & 0xFF);
    WriteMem(cpu.sp-2, cpu.pc & 0xFF);

    cpu.nmi = 0;
    cpu.pc  = 0x0066;
    cpu.sp -= 2;
    cpu.cycles += 11;
}

void EmuExecute(byte bySingleStep)
{
  byte by;

  if (bySingleStep)
  {
    by = GetMem(cpu.pc);

    if (cpu.intr && cpu.iff1) // interrupt enabled and one is requested
    {
      EmuProcessIntr(by);
    }
    else if (cpu.nmi)
    {
      EmuProcessNmi();
    }
    else
    {
      EmuExecInst(by);
    }
  }
  else
  {
    while (cpu.cycles < EXEC_LOOP_COUNT)
    {
#ifdef ENABLE_LOGGING
		  if (g_bLogOpen)
		  {
        LogNextInstruction(cpu.pc);
      }
#endif

      by = GetMem(cpu.pc);

      if (cpu.intr && cpu.iff1) // interrupt enabled and one is requested
      {
        EmuProcessIntr(by);
      }
      else if (cpu.nmi)
      {
        EmuProcessNmi();
      }
      else
      {
        EmuExecInst(by);
      }
    }
  }

  if (cpu.cycles >= EXEC_LOOP_COUNT)
  {
    g_nSystemTickCount += EXEC_LOOP_TIME;
    cpu.nTotalCycles += EXEC_LOOP_TIME;
    cpu.cycles -= EXEC_LOOP_COUNT;

    g_nRtcCounter += EXEC_LOOP_COUNT;

    if (g_nRtcIntrPeriod <= g_nRtcCounter)
    {
      g_nRtcCounter -= g_nRtcIntrPeriod;
      (*pRtcIntr)();
    }
  }
}
