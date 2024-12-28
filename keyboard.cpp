#include "pch.h"
#include "Z80Emu.h"
#include "keyboard.h"

byte  g_byKeyboardMemory[0x400];
word  g_wKeyboardStart;
word  g_wKeyboardEnd;
int   g_nKeyboardReadCount = 0;
byte  g_byKeyboardMode;

/////////////////////////////////////////////////
/* keyboard address map
* 
  0x3800 - 0x3BFF

  A0-7

  A8  = x
  A9  = x
  A10 = 0
  A11 = 1

  A12 = 1
  A13 = 1
  A14 = 0
  A15 = 0
*/

typedef struct {
  unsigned int  c; // character
  unsigned int  a; // keyboard address
  unsigned char d; // data bus code
  unsigned char s; // shift state (1 = pressed)
} KeyCode;

KeyCode keysCodes[] = {
  {'@', 0x01, 0x01, 0},
  {'a', 0x01, 0x02, 0},
  {'A', 0x01, 0x02, 1},
  {'b', 0x01, 0x04, 0},
  {'B', 0x01, 0x04, 1},
  {'c', 0x01, 0x08, 0},
  {'C', 0x01, 0x08, 1},
  {'d', 0x01, 0x10, 0},
  {'D', 0x01, 0x10, 1},
  {'e', 0x01, 0x20, 0},
  {'E', 0x01, 0x20, 1},
  {'f', 0x01, 0x40, 0},
  {'F', 0x01, 0x40, 1},
  {'g', 0x01, 0x80, 0},
  {'G', 0x01, 0x80, 1},

  {'h', 0x02, 0x01, 0},
  {'H', 0x02, 0x01, 1},
  {'i', 0x02, 0x02, 0},
  {'I', 0x02, 0x02, 1},
  {'j', 0x02, 0x04, 0},
  {'J', 0x02, 0x04, 1},
  {'k', 0x02, 0x08, 0},
  {'K', 0x02, 0x08, 1},
  {'l', 0x02, 0x10, 0},
  {'L', 0x02, 0x10, 1},
  {'m', 0x02, 0x20, 0},
  {'M', 0x02, 0x20, 1},
  {'n', 0x02, 0x40, 0},
  {'N', 0x02, 0x40, 1},
  {'o', 0x02, 0x80, 0},
  {'O', 0x02, 0x80, 1},

  {'p', 0x04, 0x01, 0},
  {'P', 0x04, 0x01, 1},
  {'q', 0x04, 0x02, 0},
  {'Q', 0x04, 0x02, 1},
  {'r', 0x04, 0x04, 0},
  {'R', 0x04, 0x04, 1},
  {'s', 0x04, 0x08, 0},
  {'S', 0x04, 0x08, 1},
  {'t', 0x04, 0x10, 0},
  {'T', 0x04, 0x10, 1},
  {'u', 0x04, 0x20, 0},
  {'U', 0x04, 0x20, 1},
  {'v', 0x04, 0x40, 0},
  {'V', 0x04, 0x40, 1},
  {'w', 0x04, 0x80, 0},
  {'W', 0x04, 0x80, 1},

  {'x', 0x08, 0x01, 0},
  {'X', 0x08, 0x01, 1},
  {'y', 0x08, 0x02, 0},
  {'Y', 0x08, 0x02, 1},
  {'z', 0x08, 0x04, 0},
  {'Z', 0x08, 0x04, 1},

  {'0', 0x10, 0x01, 0},
  {'1', 0x10, 0x02, 0},
  {'!', 0x10, 0x02, 1},
  {'2', 0x10, 0x04, 0},
  {'"', 0x10, 0x04, 1},
  {'3', 0x10, 0x08, 0},
  {'#', 0x10, 0x08, 1},
  {'4', 0x10, 0x10, 0},
  {'$', 0x10, 0x10, 1},
  {'5', 0x10, 0x20, 0},
  {'%', 0x10, 0x20, 1},
  {'6', 0x10, 0x40, 0},
  {'&', 0x10, 0x40, 1},
  {'7', 0x10, 0x80, 0},
  {39,  0x10, 0x80, 1}, // ' = 39

  {'8', 0x20, 0x01, 0},
  {'(', 0x20, 0x01, 1},
  {'9', 0x20, 0x02, 0},
  {')', 0x20, 0x02, 1},
  {':', 0x20, 0x04, 0},
  {'*', 0x20, 0x04, 1},
  {';', 0x20, 0x08, 0},
  {'+', 0x20, 0x08, 1},
  {',', 0x20, 0x10, 0},
  {'<', 0x20, 0x10, 1},
  {'-', 0x20, 0x20, 0},
  {'=', 0x20, 0x20, 1},
  {'.', 0x20, 0x40, 0},
  {'>', 0x20, 0x40, 1},
  {'/', 0x20, 0x80, 0},
  {'?', 0x20, 0x80, 1},

  {' ', 0x40, 0x80, 0}, // space
  {8,   0x40, 0x20, 0}, // backspace = 8

  {13,                 0x40, 0x01, 0}, // enter
  {0x1000 | VK_HOME,   0x40, 0x02, 0}, // clear
  {0x1000 | VK_ESCAPE, 0x40, 0x04, 0}, // Esc = Break
  {0x1000 | VK_UP,     0x40, 0x08, 0}, // up arrow
  {0x1000 | VK_DOWN,   0x40, 0x10, 0}, // down arrow
  {0x1000 | VK_LEFT,   0x40, 0x20, 0}, // left arrow
  {0x1000 | VK_RIGHT,  0x40, 0x40, 0}, // right arrow

  {0, 0, 0 ,0},
};

byte g_byHomeKeyDown;

/////////////////////////////////////////////////
void InitKeyboard(void)
{
  g_nKeyboardReadCount = 0;
  g_byHomeKeyDown  = 0;

  memset(g_byKeyboardMemory, 0, sizeof(g_byKeyboardMemory));
}

/////////////////////////////////////////////////
void ClearKeyboardMemory(void)
{
  g_nKeyboardReadCount = 0;
  memset(g_byKeyboardMemory, 0, sizeof(g_byKeyboardMemory));

  if (GetKeyState(VK_HOME) & 0x8000)
  {
    g_byKeyboardMemory[0x40] |= 0x02; // home/clear key
  }
}

/////////////////////////////////////////////////
void GetKeyCode(unsigned int c, KeyCode* pk)
{
  int i = 0;

  while (keysCodes[i].c != 0)
  {
    if (keysCodes[i].c == c)
    {
      *pk = keysCodes[i];
      return; 
    }

    ++i;
  }

  pk->c = 0;
  pk->a = 0;
  pk->d = 0;
}

/////////////////////////////////////////////////
void SimulateKeyDown(unsigned int c)
{
  KeyCode k;
  int i;

  if (c == (0x1000 | VK_HOME))
  {
    g_byKeyboardMemory[0x40] |= 0x02; // home/clear key
    g_byHomeKeyDown = 1;
    return;
  }

  GetKeyCode(c, &k);

  if (k.a != 0)
  {
    for (i = 0; i < 0x400; ++i)
    {
      if ((i & k.a) & 0x7FF)
      {
        g_byKeyboardMemory[i] |= k.d;
      }
    }

    if (k.s)
    {
      g_byKeyboardMemory[0x80] |= 0x01;
    }

    if (GetKeyState(VK_HOME) & 0x8000)
    {
      g_byKeyboardMemory[0x40] |= 0x02; // home/clear key
      g_byHomeKeyDown = 0;
    }
    else
    {
      g_byKeyboardMemory[0x40] &= ~0x02; // home/clear key
    }

    g_nKeyboardReadCount = 16;
  }
}

/////////////////////////////////////////////////
void SimulateKeyUp(unsigned int c)
{
  if (g_byHomeKeyDown)
  {
    g_byKeyboardMemory[0x40] |= 0x02; // home/clear keay
    g_byHomeKeyDown = 0;
    g_nKeyboardReadCount = 16;
  }
}

