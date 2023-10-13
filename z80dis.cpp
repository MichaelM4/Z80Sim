#include "pch.h"
#include "Z80Dis.h"
#include "Z80Emu.h"

//extern byte g_byMemory[0x10000];

typedef struct {
  char* pszName;
  int   nSize;
} DisAsmType;

extern byte (*pMemRead)(word);

/****************************************************************************************************************************/

static DisAsmType InstCB[] = {
  {"RLC B", 2},       // 0x00
  {"RLC C", 2},       // 0x01
  {"RLC D", 2},       // 0x02
  {"RLC E", 2},       // 0x03
  {"RLC H", 2},       // 0x04
  {"RLC L", 2},       // 0x05
  {"RLC (HL)", 2},    // 0x06
  {"RLC A", 2},       // 0x07
  {"RRC B", 2},       // 0x08
  {"RRC C", 2},       // 0x09
  {"RRC D", 2},       // 0x0A
  {"RRC E", 2},       // 0x0B
  {"RRC H", 2},       // 0x0C
  {"RRC L", 2},       // 0x0D
  {"RRC (HL)", 2},    // 0x0E
  {"RRC A", 2},       // 0x0F
  {"RL B", 2},        // 0x10
  {"RL C", 2},        // 0x11
  {"RL D", 2},        // 0x12
  {"RL E", 2},        // 0x13
  {"RL H", 2},        // 0x14
  {"RL L", 2},        // 0x15
  {"RL (HL)", 2},     // 0x16
  {"RL A", 2},        // 0x17
  {"RR B", 2},        // 0x18
  {"RR C", 2},        // 0x19
  {"RR D", 2},        // 0x1A
  {"RR E", 2},        // 0x1B
  {"RR H", 2},        // 0x1C
  {"RR L", 2},        // 0x1D
  {"RR (HL)", 2},     // 0x1E
  {"RR A", 2},        // 0x1F
  {"SLA B", 2},       // 0x20
  {"SLA C", 2},       // 0x21
  {"SLA D", 2},       // 0x22
  {"SLA E", 2},       // 0x23
  {"SLA H", 2},       // 0x24
  {"SLA L", 2},       // 0x25
  {"SLA (HL)", 2},    // 0x26
  {"SLA A", 2},       // 0x27
  {"SRA B", 2},       // 0x28
  {"SRA C", 2},       // 0x29
  {"SRA D", 2},       // 0x2A
  {"SRA E", 2},       // 0x2B
  {"SRA H", 2},       // 0x2C
  {"SRA L", 2},       // 0x2D
  {"SRA (HL)", 2},    // 0x2E
  {"SRA A", 2},       // 0x2F
  {"UNDEF", 1},       // 0x30
  {"UNDEF", 1},       // 0x31
  {"UNDEF", 1},       // 0x32
  {"UNDEF", 1},       // 0x33
  {"UNDEF", 1},       // 0x34
  {"UNDEF", 1},       // 0x35
  {"UNDEF", 1},       // 0x36
  {"UNDEF", 1},       // 0x37
  {"SRL B", 2},       // 0x38
  {"SRL C", 2},       // 0x39
  {"SRL D", 2},       // 0x3A
  {"SRL E", 2},       // 0x3B
  {"SRL H", 2},       // 0x3C
  {"SRL L", 2},       // 0x3D
  {"SRL (HL)", 2},    // 0x3E
  {"SRL A", 2},       // 0x3F
  {"BIT 0,B", 2},     // 0x40
  {"BIT 0,C", 2},     // 0x41
  {"BIT 0,D", 2},     // 0x42
  {"BIT 0,E", 2},     // 0x43
  {"BIT 0,H", 2},     // 0x44
  {"BIT 0,L", 2},     // 0x45
  {"BIT 0,(HL)", 2},  // 0x46
  {"BIT 0,A", 2},     // 0x47
  {"BIT 1,B", 2},     // 0x48
  {"BIT 1,C", 2},     // 0x49
  {"BIT 1,D", 2},     // 0x4A
  {"BIT 1,E", 2},     // 0x4B
  {"BIT 1,H", 2},     // 0x4C
  {"BIT 1,L", 2},     // 0x4D
  {"BIT 1,(HL)", 2},  // 0x4E
  {"BIT 1,A", 2},     // 0x4F
  {"BIT 2,B", 2},     // 0x50
  {"BIT 2,C", 2},     // 0x51
  {"BIT 2,D", 2},     // 0x52
  {"BIT 2,E", 2},     // 0x53
  {"BIT 2,H", 2},     // 0x54
  {"BIT 2,L", 2},     // 0x55
  {"BIT 2,(HL)", 2},  // 0x56
  {"BIT 2,A", 2},     // 0x57
  {"BIT 3,B", 2},     // 0x58
  {"BIT 3,C", 2},     // 0x59
  {"BIT 3,D", 2},     // 0x5A
  {"BIT 3,E", 2},     // 0x5B
  {"BIT 3,H", 2},     // 0x5C
  {"BIT 3,L", 2},     // 0x5D
  {"BIT 3,(HL)", 2},  // 0x5E
  {"BIT 3,A", 2},     // 0x5F
  {"BIT 4,B", 2},     // 0x60
  {"BIT 4,C", 2},     // 0x61
  {"BIT 4,D", 2},     // 0x62
  {"BIT 4,E", 2},     // 0x63
  {"BIT 4,H", 2},     // 0x64
  {"BIT 4,L", 2},     // 0x65
  {"BIT 4,(HL)", 2},  // 0x66
  {"BIT 4,A", 2},     // 0x67
  {"BIT 5,B", 2},     // 0x68
  {"BIT 5,C", 2},     // 0x69
  {"BIT 5,D", 2},     // 0x6A
  {"BIT 5,E", 2},     // 0x6B
  {"BIT 5,H", 2},     // 0x6C
  {"BIT 5,L", 2},     // 0x6D
  {"BIT 5,(HL)", 2},  // 0x6E
  {"BIT 5,A", 2},     // 0x6F
  {"BIT 6,B", 2},     // 0x70
  {"BIT 6,C", 2},     // 0x71
  {"BIT 6,D", 2},     // 0x72
  {"BIT 6,E", 2},     // 0x73
  {"BIT 6,H", 2},     // 0x74
  {"BIT 6,L", 2},     // 0x75
  {"BIT 6,(HL)", 2},  // 0x76
  {"BIT 6,A", 2},     // 0x77
  {"BIT 7,B", 2},     // 0x78
  {"BIT 7,C", 2},     // 0x79
  {"BIT 7,D", 2},     // 0x7A
  {"BIT 7,E", 2},     // 0x7B
  {"BIT 7,H", 2},     // 0x7C
  {"BIT 7,L", 2},     // 0x7D
  {"BIT 7,(HL)", 2},  // 0x7E
  {"BIT 7,A", 2},     // 0x7F
  {"RES 0,B", 2},     // 0x80
  {"RES 0,C", 2},     // 0x81
  {"RES 0,D", 2},     // 0x82
  {"RES 0,E", 2},     // 0x83
  {"RES 0,H", 2},     // 0x84
  {"RES 0,L", 2},     // 0x85
  {"RES 0,(HL)", 2}, // 0x86
  {"RES 0,A", 2},     // 0x87
  {"RES 1,B", 2},     // 0x88
  {"RES 1,C", 2},     // 0x89
  {"RES 1,D", 2},     // 0x8A
  {"RES 1,E", 2},     // 0x8B
  {"RES 1,H", 2},     // 0x8C
  {"RES 1,L", 2},     // 0x8D
  {"RES 1,(HL)", 2},  // 0x8E
  {"RES 1,A", 2},     // 0x8F
  {"RES 2,B", 2},     // 0x90
  {"RES 2,C", 2},     // 0x91
  {"RES 2,D", 2},     // 0x92
  {"RES 2,E", 2},     // 0x93
  {"RES 2,H", 2},     // 0x94
  {"RES 2,L", 2},     // 0x95
  {"RES 2,(HL)", 2},  // 0x96
  {"RES 2,A", 2},     // 0x97
  {"RES 3,B", 2},     // 0x98
  {"RES 3,C", 2},     // 0x99
  {"RES 3,D", 2},     // 0x9A
  {"RES 3,E", 2},     // 0x9B
  {"RES 3,H", 2},     // 0x9C
  {"RES 3,L", 2},     // 0x9D
  {"RES 3,(HL)", 2},  // 0x9E
  {"RES 3,A", 2},     // 0x9F
  {"RES 4,B", 2},     // 0xA0
  {"RES 4,C", 2},     // 0xA1
  {"RES 4,D", 2},     // 0xA2
  {"RES 4,E", 2},     // 0xA3
  {"RES 4,H", 2},     // 0xA4
  {"RES 4,L", 2},     // 0xA5
  {"RES 4,(HL)", 2},  // 0xA6
  {"RES 4,A", 2},     // 0xA7
  {"RES 5,B", 2},     // 0xA8
  {"RES 5,C", 2},     // 0xA9
  {"RES 5,D", 2},     // 0xAA
  {"RES 5,E", 2},     // 0xAB
  {"RES 5,H", 2},     // 0xAC
  {"RES 5,L", 2},     // 0xAD
  {"RES 5,(HL)", 2},  // 0xAE
  {"RES 5,A", 2},     // 0xAF
  {"RES 6,B", 2},     // 0xB0
  {"RES 6,C", 2},     // 0xB1
  {"RES 6,D", 2},     // 0xB2
  {"RES 6,E", 2},     // 0xB3
  {"RES 6,H", 2},     // 0xB4
  {"RES 6,L", 2},     // 0xB5
  {"RES 6,(HL)", 2},  // 0xB6
  {"RES 6,A", 2},     // 0xB7
  {"RES 7,B", 2},     // 0xB8
  {"RES 7,C", 2},     // 0xB9
  {"RES 7,D", 2},     // 0xBA
  {"RES 7,E", 2},     // 0xBB
  {"RES 7,H", 2},     // 0xBC
  {"RES 7,L", 2},     // 0xBD
  {"RES 7,(HL)", 2},  // 0xBE
  {"RES 7,A", 2},     // 0xBF
  {"SET 0,B", 2},     // 0xC0
  {"SET 0,C", 2},     // 0xC1
  {"SET 0,D", 2},     // 0xC2
  {"SET 0,E", 2},     // 0xC3
  {"SET 0,H", 2},     // 0xC4
  {"SET 0,L", 2},     // 0xC5
  {"SET 0,(HL)", 2},  // 0xC6
  {"SET 0,A", 2},     // 0xC7
  {"SET 1,B", 2},     // 0xC8
  {"SET 1,C", 2},     // 0xC9
  {"SET 1,D", 2},     // 0xCA
  {"SET 1,E", 2},     // 0xCB
  {"SET 1,H", 2},     // 0xCC
  {"SET 1,L", 2},     // 0xCD
  {"SET 1,(HL)", 2},  // 0xCE
  {"SET 1,A", 2},     // 0xCF
  {"SET 2,B", 2},     // 0xD0
  {"SET 2,C", 2},     // 0xD1
  {"SET 2,D", 2},     // 0xD2
  {"SET 2,E", 2},     // 0xD3
  {"SET 2,H", 2},     // 0xD4
  {"SET 2,L", 2},     // 0xD5
  {"SET 2,(HL)", 2},  // 0xD6
  {"SET 2,A", 2},     // 0xD7
  {"SET 3,B", 2},     // 0xD8
  {"SET 3,C", 2},     // 0xD9
  {"SET 3,D", 2},     // 0xDA
  {"SET 3,E", 2},     // 0xDB
  {"SET 3,H", 2},     // 0xDC
  {"SET 3,L", 2},     // 0xDD
  {"SET 3,(HL)", 2},  // 0xDE
  {"SET 3,A", 2},     // 0xDF
  {"SET 4,B", 2},     // 0xE0
  {"SET 4,C", 2},     // 0xE1
  {"SET 4,D", 2},     // 0xE2
  {"SET 4,E", 2},     // 0xE3
  {"SET 4,H", 2},     // 0xE4
  {"SET 4,L", 2},     // 0xE5
  {"SET 4,(HL)", 2},  // 0xE6
  {"SET 4,A", 2},     // 0xE7
  {"SET 5,B", 2},     // 0xE8
  {"SET 5,C", 2},     // 0xE9
  {"SET 5,D", 2},     // 0xEA
  {"SET 5,E", 2},     // 0xEB
  {"SET 5,H", 2},     // 0xEC
  {"SET 5,L", 2},     // 0xED
  {"SET 5,(HL)", 2},  // 0xEE
  {"SET 5,A", 2},     // 0xEF
  {"SET 6,B", 2},     // 0xF0
  {"SET 6,C", 2},     // 0xF1
  {"SET 6,D", 2},     // 0xF2
  {"SET 6,E", 2},     // 0xF3
  {"SET 6,H", 2},     // 0xF4
  {"SET 6,L", 2},     // 0xF5
  {"SET 6,(HL)", 2},  // 0xF6
  {"SET 6,A", 2},     // 0xF7
  {"SET 7,B", 2},     // 0xF8
  {"SET 7,C", 2},     // 0xF9
  {"SET 7,D", 2},     // 0xFA
  {"SET 7,E", 2},     // 0xFB
  {"SET 7,H", 2},     // 0xFC
  {"SET 7,L", 2},     // 0xFD
  {"SET 7,(HL)", 2},  // 0xFE
  {"SET 7,A", 2},     // 0xFF
};

static DisAsmType InstDD[] = {
  {"UNDEF", 1},              // 0x00
  {"UNDEF", 1},              // 0x01
  {"UNDEF", 1},              // 0x02
  {"UNDEF", 1},              // 0x03
  {"UNDEF", 1},              // 0x04
  {"UNDEF", 1},              // 0x05
  {"UNDEF", 1},              // 0x06
  {"UNDEF", 1},              // 0x07
  {"UNDEF", 1},              // 0x08
  {"ADD IX,BC", 2},          // 0x09
  {"UNDEF", 1},              // 0x0A
  {"UNDEF", 1},              // 0x0B
  {"UNDEF", 1},              // 0x0C
  {"UNDEF", 1},              // 0x0D
  {"UNDEF", 1},              // 0x0E
  {"UNDEF", 1},              // 0x0F
  {"UNDEF", 1},              // 0x10
  {"UNDEF", 1},              // 0x11
  {"UNDEF", 1},              // 0x12
  {"UNDEF", 1},              // 0x13
  {"UNDEF", 1},              // 0x14
  {"UNDEF", 1},              // 0x15
  {"UNDEF", 1},              // 0x16
  {"UNDEF", 1},              // 0x17
  {"UNDEF", 1},              // 0x18
  {"ADD IX,DE", 2},          // 0x19
  {"UNDEF", 1},              // 0x1A
  {"UNDEF", 1},              // 0x1B
  {"UNDEF", 1},              // 0x1C
  {"UNDEF", 1},              // 0x1D
  {"UNDEF", 1},              // 0x1E
  {"UNDEF", 1},              // 0x1F
  {"UNDEF", 1},              // 0x20
  {"LD IX,nn", 4},           // 0x21
  {"LD (nn),IX", 4},         // 0x22
  {"INC IX", 2},             // 0x23
  {"UNDEF", 1},              // 0x24
  {"UNDEF", 1},              // 0x25
  {"UNDEF", 1},              // 0x26
  {"UNDEF", 1},              // 0x27
  {"UNDEF", 1},              // 0x28
  {"ADD IX,IX", 2},          // 0x29
  {"LD IX,(nn)", 4},         // 0x2A
  {"DEC IX", 1},             // 0x2B
  {"UNDEF", 1},              // 0x2C
  {"UNDEF", 1},              // 0x2D
  {"UNDEF", 1},              // 0x2E
  {"UNDEF", 1},              // 0x2F
  {"UNDEF", 1},              // 0x30
  {"UNDEF", 1},              // 0x31
  {"UNDEF", 1},              // 0x32
  {"UNDEF", 1},              // 0x33
  {"INC (IX+d)", 2},         // 0x34
  {"DEC (IX+d)", 2},         // 0x35
  {"LD (IX+d),n", 4},        // 0x36
  {"UNDEF", 1},              // 0x37
  {"UNDEF", 1},              // 0x38
  {"ADD IX,SP", 2},          // 0x39
  {"UNDEF", 1},              // 0x3A
  {"UNDEF", 1},              // 0x3B
  {"UNDEF", 1},              // 0x3C
  {"UNDEF", 1},              // 0x3D
  {"UNDEF", 1},              // 0x3E
  {"UNDEF", 1},              // 0x3F
  {"UNDEF", 1},              // 0x40
  {"UNDEF", 1},              // 0x41
  {"UNDEF", 1},              // 0x42
  {"UNDEF", 1},              // 0x43
  {"UNDEF", 1},              // 0x44
  {"UNDEF", 1},              // 0x45
  {"LD B,(IX+d)", 3},        // 0x46
  {"UNDEF", 1},              // 0x47
  {"UNDEF", 1},              // 0x48
  {"UNDEF", 1},              // 0x49
  {"UNDEF", 1},              // 0x4A
  {"UNDEF", 1},              // 0x4B
  {"UNDEF", 1},              // 0x4C
  {"UNDEF", 1},              // 0x4D
  {"LD C,(IX+d)", 3},        // 0x4E
  {"UNDEF", 1},              // 0x4F
  {"UNDEF", 1},              // 0x50
  {"UNDEF", 1},              // 0x51
  {"UNDEF", 1},              // 0x52
  {"UNDEF", 1},              // 0x53
  {"UNDEF", 1},              // 0x54
  {"UNDEF", 1},              // 0x55
  {"LD D,(IX+d)", 3},        // 0x56
  {"UNDEF", 1},              // 0x57
  {"UNDEF", 1},              // 0x58
  {"UNDEF", 1},              // 0x59
  {"UNDEF", 1},              // 0x5A
  {"UNDEF", 1},              // 0x5B
  {"UNDEF", 1},              // 0x5C
  {"UNDEF", 1},              // 0x5D
  {"LD E,(IX+d)", 3},        // 0x5E
  {"UNDEF", 1},              // 0x5F
  {"UNDEF", 1},              // 0x60
  {"UNDEF", 1},              // 0x61
  {"UNDEF", 1},              // 0x62
  {"UNDEF", 1},              // 0x63
  {"UNDEF", 1},              // 0x64
  {"UNDEF", 1},              // 0x65
  {"LD H,(IX+d)", 3},        // 0x66
  {"UNDEF", 1},              // 0x67
  {"UNDEF", 1},              // 0x68
  {"UNDEF", 1},              // 0x69
  {"UNDEF", 1},              // 0x6A
  {"UNDEF", 1},              // 0x6B
  {"UNDEF", 1},              // 0x6C
  {"UNDEF", 1},              // 0x6D
  {"LD L,(IX+d)", 3},        // 0x6E
  {"UNDEF", 1},              // 0x6F
  {"LD (IX+d),B", 3},        // 0x70
  {"LD (IX+d),C", 3},        // 0x71
  {"LD (IX+d),D", 3},        // 0x72
  {"LD (IX+d),E", 3},        // 0x73
  {"LD (IX+d),H", 3},        // 0x74
  {"LD (IX+d),L", 3},        // 0x75
  {"UNDEF", 1},              // 0x76
  {"LD (IX+d),A", 3},        // 0x77
  {"UNDEF", 1},              // 0x78
  {"UNDEF", 1},              // 0x79
  {"UNDEF", 1},              // 0x7A
  {"UNDEF", 1},              // 0x7B
  {"UNDEF", 1},              // 0x7C
  {"UNDEF", 1},              // 0x7D
  {"LD A,(IX+d)", 3},        // 0x7E
  {"UNDEF", 1},              // 0x7F
  {"UNDEF", 1},              // 0x80
  {"UNDEF", 1},              // 0x81
  {"UNDEF", 1},              // 0x82
  {"UNDEF", 1},              // 0x83
  {"UNDEF", 1},              // 0x84
  {"UNDEF", 1},              // 0x85
  {"ADD A,(IX+d)", 3},       // 0x86
  {"UNDEF", 1},              // 0x87
  {"UNDEF", 1},              // 0x88
  {"UNDEF", 1},              // 0x89
  {"UNDEF", 1},              // 0x8A
  {"UNDEF", 1},              // 0x8B
  {"UNDEF", 1},              // 0x8C
  {"UNDEF", 1},              // 0x8D
  {"ADC A,(IX+d)", 3},       // 0x8E
  {"UNDEF", 1},              // 0x8F
  {"UNDEF", 1},              // 0x90
  {"UNDEF", 1},              // 0x91
  {"UNDEF", 1},              // 0x92
  {"UNDEF", 1},              // 0x93
  {"UNDEF", 1},              // 0x94
  {"UNDEF", 1},              // 0x95
  {"SUB A,(IX+d)", 3},       // 0x96
  {"UNDEF", 1},              // 0x97
  {"UNDEF", 1},              // 0x98
  {"UNDEF", 1},              // 0x99
  {"UNDEF", 1},              // 0x9A
  {"UNDEF", 1},              // 0x9B
  {"UNDEF", 1},              // 0x9C
  {"UNDEF", 1},              // 0x9D
  {"SBC A,(IX+d)", 3},       // 0x9E
  {"UNDEF", 1},              // 0x9F
  {"UNDEF", 1},              // 0xA0
  {"UNDEF", 1},              // 0xA1
  {"UNDEF", 1},              // 0xA2
  {"UNDEF", 1},              // 0xA3
  {"UNDEF", 1},              // 0xA4
  {"UNDEF", 1},              // 0xA5
  {"AND A,(IX+d)", 3},       // 0xA6
  {"UNDEF", 1},              // 0xA7
  {"UNDEF", 1},              // 0xA8
  {"UNDEF", 1},              // 0xA9
  {"UNDEF", 1},              // 0xAA
  {"UNDEF", 1},              // 0xAB
  {"UNDEF", 1},              // 0xAC
  {"UNDEF", 1},              // 0xAD
  {"XOR A,(IX+d)", 3},       // 0xAE
  {"UNDEF", 1},              // 0xAF
  {"UNDEF", 1},              // 0xB0
  {"UNDEF", 1},              // 0xB1
  {"UNDEF", 1},              // 0xB2
  {"UNDEF", 1},              // 0xB3
  {"UNDEF", 1},              // 0xB4
  {"UNDEF", 1},              // 0xB5
  {"OR A,(IX+d)", 3},        // 0xB6
  {"UNDEF", 1},              // 0xB7
  {"UNDEF", 1},              // 0xB8
  {"UNDEF", 1},              // 0xB9
  {"UNDEF", 1},              // 0xBA
  {"UNDEF", 1},              // 0xBB
  {"UNDEF", 1},              // 0xBC
  {"UNDEF", 1},              // 0xBD
  {"CP (IX+d)", 3},          // 0xBE
  {"UNDEF", 1},              // 0xBF
  {"UNDEF", 1},              // 0xC0
  {"UNDEF", 1},              // 0xC1
  {"UNDEF", 1},              // 0xC2
  {"UNDEF", 1},              // 0xC3
  {"UNDEF", 1},              // 0xC4
  {"UNDEF", 1},              // 0xC5
  {"UNDEF", 1},              // 0xC6
  {"UNDEF", 1},              // 0xC7
  {"UNDEF", 1},              // 0xC8
  {"UNDEF", 1},              // 0xC9
  {"UNDEF", 1},              // 0xCA
  {"OPCODE_DD_CB", 1},       // 0xCB
  {"UNDEF", 1},              // 0xCC
  {"UNDEF", 1},              // 0xCD
  {"UNDEF", 1},              // 0xCE
  {"UNDEF", 1},              // 0xCF
  {"UNDEF", 1},              // 0xD0
  {"UNDEF", 1},              // 0xD1
  {"UNDEF", 1},              // 0xD2
  {"UNDEF", 1},              // 0xD3
  {"UNDEF", 1},              // 0xD4
  {"UNDEF", 1},              // 0xD5
  {"UNDEF", 1},              // 0xD6
  {"UNDEF", 1},              // 0xD7
  {"UNDEF", 1},              // 0xD8
  {"UNDEF", 1},              // 0xD9
  {"UNDEF", 1},              // 0xDA
  {"UNDEF", 1},              // 0xDB
  {"UNDEF", 1},              // 0xDC
  {"UNDEF", 1},              // 0xDD
  {"UNDEF", 1},              // 0xDE
  {"UNDEF", 1},              // 0xDF
  {"UNDEF", 1},              // 0xE0
  {"POP IX", 2},             // 0xE1
  {"UNDEF", 1},              // 0xE2
  {"EX (SP),IX", 2},         // 0xE3
  {"UNDEF", 1},              // 0xE4
  {"PUSH IX", 1},            // 0xE5
  {"UNDEF", 1},              // 0xE6
  {"UNDEF", 1},              // 0xE7
  {"UNDEF", 1},              // 0xE8
  {"JP (IX)", 2},            // 0xE9
  {"UNDEF", 1},              // 0xEA
  {"UNDEF", 1},              // 0xEB
  {"UNDEF", 1},              // 0xEC
  {"UNDEF", 1},              // 0xED
  {"UNDEF", 1},              // 0xEE
  {"UNDEF", 1},              // 0xEF
  {"UNDEF", 1},              // 0xF0
  {"UNDEF", 1},              // 0xF1
  {"UNDEF", 1},              // 0xF2
  {"UNDEF", 1},              // 0xF3
  {"UNDEF", 1},              // 0xF4
  {"UNDEF", 1},              // 0xF5
  {"UNDEF", 1},              // 0xF6
  {"UNDEF", 1},              // 0xF7
  {"UNDEF", 1},              // 0xF8
  {"LD SP,IX", 2},           // 0xF9
  {"UNDEF", 1},              // 0xFA
  {"UNDEF", 1},              // 0xFB
  {"UNDEF", 1},              // 0xFC
  {"UNDEF", 1},              // 0xFD
  {"UNDEF", 1},              // 0xFE
  {"UNDEF", 1},              // 0xFF
};

/////////////////////////////////////////////////
static DisAsmType InstED[] = {
  {"UNDEF", 1},       // 0x00
  {"UNDEF", 1},       // 0x01
  {"UNDEF", 1},       // 0x02
  {"UNDEF", 1},       // 0x03
  {"UNDEF", 1},       // 0x04
  {"UNDEF", 1},       // 0x05
  {"UNDEF", 1},       // 0x06
  {"UNDEF", 1},       // 0x07
  {"UNDEF", 1},       // 0x08
  {"UNDEF", 1},       // 0x09
  {"UNDEF", 1},       // 0x0A
  {"UNDEF", 1},       // 0x0B
  {"UNDEF", 1},       // 0x0C
  {"UNDEF", 1},       // 0x0D
  {"UNDEF", 1},       // 0x0E
  {"UNDEF", 1},       // 0x0F
  {"UNDEF", 1},       // 0x10
  {"UNDEF", 1},       // 0x11
  {"UNDEF", 1},       // 0x12
  {"UNDEF", 1},       // 0x13
  {"UNDEF", 1},       // 0x14
  {"UNDEF", 1},       // 0x15
  {"UNDEF", 1},       // 0x16
  {"UNDEF", 1},       // 0x17
  {"UNDEF", 1},       // 0x18
  {"UNDEF", 1},       // 0x19
  {"UNDEF", 1},       // 0x1A
  {"UNDEF", 1},       // 0x1B
  {"UNDEF", 1},       // 0x1C
  {"UNDEF", 1},       // 0x1D
  {"UNDEF", 1},       // 0x1E
  {"UNDEF", 1},       // 0x1F
  {"UNDEF", 1},       // 0x20
  {"UNDEF", 1},       // 0x21
  {"UNDEF", 1},       // 0x22
  {"UNDEF", 1},       // 0x23
  {"UNDEF", 1},       // 0x24
  {"UNDEF", 1},       // 0x25
  {"UNDEF", 1},       // 0x26
  {"UNDEF", 1},       // 0x27
  {"UNDEF", 1},       // 0x28
  {"UNDEF", 1},       // 0x29
  {"UNDEF", 1},       // 0x2A
  {"UNDEF", 1},       // 0x2B
  {"UNDEF", 1},       // 0x2C
  {"UNDEF", 1},       // 0x2D
  {"UNDEF", 1},       // 0x2E
  {"UNDEF", 1},       // 0x2F
  {"UNDEF", 1},       // 0x30
  {"UNDEF", 1},       // 0x31
  {"UNDEF", 1},       // 0x32
  {"UNDEF", 1},       // 0x33
  {"UNDEF", 1},       // 0x34
  {"UNDEF", 1},       // 0x35
  {"UNDEF", 1},       // 0x36
  {"UNDEF", 1},       // 0x37
  {"UNDEF", 1},       // 0x38
  {"UNDEF", 1},       // 0x39
  {"UNDEF", 1},       // 0x3A
  {"UNDEF", 1},       // 0x3B
  {"UNDEF", 1},       // 0x3C
  {"UNDEF", 1},       // 0x3D
  {"UNDEF", 1},       // 0x3E
  {"UNDEF", 1},       // 0x3F
  {"IN B,C", 2},      // 0x40
  {"OUT c,B", 2},     // 0x41
  {"SBC HL,BC", 2},   // 0x42
  {"LD (nn),BC", 4},  // 0x43
  {"NEG", 2},         // 0x44
  {"RETN", 2},        // 0x45
  {"IM0", 2},         // 0x46
  {"LD I,A", 2},      // 0x47
  {"IN C,C", 2},      // 0x48
  {"OUT c,C", 2},     // 0x49
  {"ADC HL,BC", 1},   // 0x4A
  {"LD BC,(nn)", 4},  // 0x4B
  {"UNDEF", 1},       // 0x4C
  {"RETI", 2},        // 0x4D
  {"UNDEF", 1},       // 0x4E
  {"LD R,A", 2},      // 0x4F
  {"IN D,C", 2},      // 0x50
  {"OUT c,D", 2},     // 0x51
  {"SBC HL,DE", 2},   // 0x52
  {"LD (nn),DE", 4},  // 0x53
  {"UNDEF", 1},       // 0x54
  {"UNDEF", 1},       // 0x55
  {"IM1", 2},         // 0x56
  {"LD A,I", 2},      // 0x57
  {"IN E,C", 2},      // 0x58
  {"OUT c,E", 2},     // 0x59
  {"ADC HL,DE", 2},   // 0x5A
  {"LD DE,(nn)", 4},  // 0x5B
  {"UNDEF", 1},       // 0x5C
  {"UNDEF", 1},       // 0x5D
  {"IM2", 2},         // 0x5E
  {"LD A,R", 2},      // 0x5F
  {"IN H,C", 2},      // 0x60
  {"OUT c,H", 2},     // 0x61
  {"SBC HL,HL", 2},   // 0x62
  {"LD (nn),HL", 4},  // 0x63
  {"UNDEF", 1},       // 0x64
  {"UNDEF", 1},       // 0x65
  {"UNDEF", 1},       // 0x66
  {"RRD", 2},         // 0x67
  {"IN L,C", 2},      // 0x68
  {"OUT c,L", 2},     // 0x69
  {"ADC HL,HL", 2},   // 0x6A
  {"LD HL,(nn)", 4},  // 0x6B
  {"UNDEF", 1},       // 0x6C
  {"UNDEF", 1},       // 0x6D
  {"UNDEF", 1},       // 0x6E
  {"RLD", 2},         // 0x6F
  {"UNDEF", 1},       // 0x70
  {"UNDEF", 1},       // 0x71
  {"SBC HL,SP", 2},   // 0x72
  {"LD (nn),SP", 4},  // 0x73
  {"UNDEF", 1},       // 0x74
  {"UNDEF", 1},       // 0x75
  {"UNDEF", 1},       // 0x76
  {"UNDEF", 1},       // 0x77
  {"IN A,C", 2},      // 0x78
  {"OUT c,A", 2},     // 0x79
  {"ADC HL,SP", 2},   // 0x7A
  {"LD SP,(nn)", 4},  // 0x7B
  {"UNDEF", 1},       // 0x7C
  {"UNDEF", 1},       // 0x7D
  {"UNDEF", 1},       // 0x7E
  {"UNDEF", 1},       // 0x7F
  {"UNDEF", 1},       // 0x80
  {"UNDEF", 1},       // 0x81
  {"UNDEF", 1},       // 0x82
  {"UNDEF", 1},       // 0x83
  {"UNDEF", 1},       // 0x84
  {"UNDEF", 1},       // 0x85
  {"UNDEF", 1},       // 0x86
  {"UNDEF", 1},       // 0x87
  {"UNDEF", 1},       // 0x88
  {"UNDEF", 1},       // 0x89
  {"UNDEF", 1},       // 0x8A
  {"UNDEF", 1},       // 0x8B
  {"UNDEF", 1},       // 0x8C
  {"UNDEF", 1},       // 0x8D
  {"UNDEF", 1},       // 0x8E
  {"UNDEF", 1},       // 0x8F
  {"UNDEF", 1},       // 0x90
  {"UNDEF", 1},       // 0x91
  {"UNDEF", 1},       // 0x92
  {"UNDEF", 1},       // 0x93
  {"UNDEF", 1},       // 0x94
  {"UNDEF", 1},       // 0x95
  {"UNDEF", 1},       // 0x96
  {"UNDEF", 1},       // 0x97
  {"UNDEF", 1},       // 0x98
  {"UNDEF", 1},       // 0x99
  {"UNDEF", 1},       // 0x9A
  {"UNDEF", 1},       // 0x9B
  {"UNDEF", 1},       // 0x9C
  {"UNDEF", 1},       // 0x9D
  {"UNDEF", 1},       // 0x9E
  {"UNDEF", 1},       // 0x9F
  {"LDI", 2},         // 0xA0
  {"CPI", 2},         // 0xA1
  {"INI", 2},         // 0xA2
  {"OUTI", 2},        // 0xA3
  {"UNDEF", 1},       // 0xA4
  {"UNDEF", 1},       // 0xA5
  {"UNDEF", 1},       // 0xA6
  {"UNDEF", 1},       // 0xA7
  {"LDD", 2},         // 0xA8
  {"CPD", 2},         // 0xA9
  {"IND", 2},         // 0xAA
  {"OUTD", 2},        // 0xAB
  {"UNDEF", 1},       // 0xAC
  {"UNDEF", 1},       // 0xAD
  {"UNDEF", 1},       // 0xAE
  {"UNDEF", 1},       // 0xAF
  {"LDIR", 2},        // 0xB0
  {"CPIR", 2},        // 0xB1
  {"INIR", 2},        // 0xB2
  {"OTIR", 2},        // 0xB3
  {"UNDEF", 1},       // 0xB4
  {"UNDEF", 1},       // 0xB5
  {"UNDEF", 1},       // 0xB6
  {"UNDEF", 1},       // 0xB7
  {"LDDR", 2},        // 0xB8
  {"CPDR", 2},        // 0xB9
  {"INDR", 2},        // 0xBA
  {"OTDR", 2},        // 0xBB
  {"UNDEF", 1},       // 0xBC
  {"UNDEF", 1},       // 0xBD
  {"UNDEF", 1},       // 0xBE
  {"UNDEF", 1},       // 0xBF
  {"UNDEF", 1},       // 0xC0
  {"UNDEF", 1},       // 0xC1
  {"UNDEF", 1},       // 0xC2
  {"UNDEF", 1},       // 0xC3
  {"UNDEF", 1},       // 0xC4
  {"UNDEF", 1},       // 0xC5
  {"UNDEF", 1},       // 0xC6
  {"UNDEF", 1},       // 0xC7
  {"UNDEF", 1},       // 0xC8
  {"UNDEF", 1},       // 0xC9
  {"UNDEF", 1},       // 0xCA
  {"UNDEF", 1},       // 0xCB
  {"UNDEF", 1},       // 0xCC
  {"UNDEF", 1},       // 0xCD
  {"UNDEF", 1},       // 0xCE
  {"UNDEF", 1},       // 0xCF
  {"UNDEF", 1},       // 0xD0
  {"UNDEF", 1},       // 0xD1
  {"UNDEF", 1},       // 0xD2
  {"UNDEF", 1},       // 0xD3
  {"UNDEF", 1},       // 0xD4
  {"UNDEF", 1},       // 0xD5
  {"UNDEF", 1},       // 0xD6
  {"UNDEF", 1},       // 0xD7
  {"UNDEF", 1},       // 0xD8
  {"UNDEF", 1},       // 0xD9
  {"UNDEF", 1},       // 0xDA
  {"UNDEF", 1},       // 0xDB
  {"UNDEF", 1},       // 0xDC
  {"UNDEF", 1},       // 0xDD
  {"UNDEF", 1},       // 0xDE
  {"UNDEF", 1},       // 0xDF
  {"UNDEF", 1},       // 0xE0
  {"UNDEF", 1},       // 0xE1
  {"UNDEF", 1},       // 0xE2
  {"UNDEF", 1},       // 0xE3
  {"UNDEF", 1},       // 0xE4
  {"UNDEF", 1},       // 0xE5
  {"UNDEF", 1},       // 0xE6
  {"UNDEF", 1},       // 0xE7
  {"UNDEF", 1},       // 0xE8
  {"UNDEF", 1},       // 0xE9
  {"UNDEF", 1},       // 0xEA
  {"UNDEF", 1},       // 0xEB
  {"UNDEF", 1},       // 0xEC
  {"UNDEF", 1},       // 0xED
  {"UNDEF", 1},       // 0xEE
  {"UNDEF", 1},       // 0xEF
  {"UNDEF", 1},       // 0xF0
  {"UNDEF", 1},       // 0xF1
  {"UNDEF", 1},       // 0xF2
  {"UNDEF", 1},       // 0xF3
  {"UNDEF", 1},       // 0xF4
  {"UNDEF", 1},       // 0xF5
  {"UNDEF", 1},       // 0xF6
  {"UNDEF", 1},       // 0xF7
  {"UNDEF", 1},       // 0xF8
  {"UNDEF", 1},       // 0xF9
  {"UNDEF", 1},       // 0xFA
  {"UNDEF", 1},       // 0xFB
  {"UNDEF", 1},       // 0xFC
  {"UNDEF", 1},       // 0xFD
  {"UNDEF", 1},       // 0xFE
  {"UNDEF", 1},       // 0xFF
};

int GetInstructionName_FD_CB(int pc, char* psz, int nMaxLen)
{
  int nSize = 1;

  switch ((*pMemRead)(pc+3))
  {
    case 0x06:
      strcpy_s(psz, nMaxLen, "RLC (IY+d)");
      nSize = 4;
      break;

    case 0x0E:
      strcpy_s(psz, nMaxLen, "RRC (IY+d)");
      nSize = 4;
      break;

    case 0x16:
      strcpy_s(psz, nMaxLen, "RL (IY+d)");
      nSize = 4;
      break;

    case 0x1E:
      strcpy_s(psz, nMaxLen, "RR (IY+d)");
      nSize = 4;
      break;

    case 0x26:
      strcpy_s(psz, nMaxLen, "SLA (IY+d)");
      nSize = 4;
      break;

    case 0x2E:
      strcpy_s(psz, nMaxLen, "SRA (IY+d)");
      nSize = 4;
      break;

    case 0x3E:
      strcpy_s(psz, nMaxLen, "SRL (IY+d)");
      nSize = 4;
      break;

    case 0x46: // 0100 0110
    case 0x4E: // 0100 1110
    case 0x56: // 0101 0110
    case 0x5E: // 0101 1110
    case 0x66: // 0110 0110
    case 0x6E: // 0110 1110
    case 0x76: // 0111 0110
    case 0x7E: // 0111 1110
      sprintf_s(psz, nMaxLen, "BIT %d,(IY+d)", ((*pMemRead)(pc+3) >> 3) & 7);
      nSize = 4;
      break;

    case 0x86: // 1000 0110
    case 0x8E: // 1000 1110
    case 0x96: // 1001 0110
    case 0x9E: // 1001 1110
    case 0xA6: // 1010 0110
    case 0xAE: // 1010 1110
    case 0xB6: // 1011 0110
    case 0xBE: // 1011 1110
      sprintf_s(psz, nMaxLen, "RES %d,(IY+d)", ((*pMemRead)(pc+3) >> 3) & 7);
      nSize = 4;
      break;

    case 0xC6: // 1100 0110
    case 0xCE: // 1100 1110
    case 0xD6: // 1101 0110
    case 0xDE: // 1101 1110
    case 0xE6: // 1110 0110
    case 0xEE: // 1110 1110
    case 0xF6: // 1111 0110
    case 0xFE: // 1111 1110
      sprintf_s(psz, nMaxLen, "SET %d,(IY+d)", ((*pMemRead)(pc+3) >> 3) & 7);
      nSize = 4;
      break;

    default:
      strcpy_s(psz, nMaxLen, "_UNDEF");
      nSize = 1;
      break;
  }

  return nSize;
}

static DisAsmType InstFD[] = {
  {"UNDEF", 1},              // 0x00
  {"UNDEF", 1},              // 0x01
  {"UNDEF", 1},              // 0x02
  {"UNDEF", 1},              // 0x03
  {"UNDEF", 1},              // 0x04
  {"UNDEF", 1},              // 0x05
  {"UNDEF", 1},              // 0x06
  {"UNDEF", 1},              // 0x07
  {"UNDEF", 1},              // 0x08
  {"ADD IY,BC", 2},          // 0x09
  {"UNDEF", 1},              // 0x0A
  {"UNDEF", 1},              // 0x0B
  {"UNDEF", 1},              // 0x0C
  {"UNDEF", 1},              // 0x0D
  {"UNDEF", 1},              // 0x0E
  {"UNDEF", 1},              // 0x0F
  {"UNDEF", 1},              // 0x10
  {"UNDEF", 1},              // 0x11
  {"UNDEF", 1},              // 0x12
  {"UNDEF", 1},              // 0x13
  {"UNDEF", 1},              // 0x14
  {"UNDEF", 1},              // 0x15
  {"UNDEF", 1},              // 0x16
  {"UNDEF", 1},              // 0x17
  {"UNDEF", 1},              // 0x18
  {"ADD IY,DE", 2},          // 0x19
  {"UNDEF", 1},              // 0x1A
  {"UNDEF", 1},              // 0x1B
  {"UNDEF", 1},              // 0x1C
  {"UNDEF", 1},              // 0x1D
  {"UNDEF", 1},              // 0x1E
  {"UNDEF", 1},              // 0x1F
  {"UNDEF", 1},              // 0x20
  {"LD IY,nn", 4},           // 0x21
  {"LD (nn),IY", 4},         // 0x22
  {"INC IY", 2},             // 0x23
  {"UNDEF", 1},              // 0x24
  {"UNDEF", 1},              // 0x25
  {"UNDEF", 1},              // 0x26
  {"UNDEF", 1},              // 0x27
  {"UNDEF", 1},              // 0x28
  {"ADD IY,IY", 2},          // 0x29
  {"LD IY,(nn)", 4},         // 0x2A
  {"DEC IY", 1},             // 0x2B
  {"UNDEF", 1},              // 0x2C
  {"UNDEF", 1},              // 0x2D
  {"UNDEF", 1},              // 0x2E
  {"UNDEF", 1},              // 0x2F
  {"UNDEF", 1},              // 0x30
  {"UNDEF", 1},              // 0x31
  {"UNDEF", 1},              // 0x32
  {"UNDEF", 1},              // 0x33
  {"INC (IY+d)", 3},         // 0x34
  {"DEC (IY+d)", 3},         // 0x35
  {"LD (IY+d),n", 4},        // 0x36
  {"UNDEF", 1},              // 0x37
  {"UNDEF", 1},              // 0x38
  {"ADD IY,SP", 2},          // 0x39
  {"UNDEF", 1},              // 0x3A
  {"UNDEF", 1},              // 0x3B
  {"UNDEF", 1},              // 0x3C
  {"UNDEF", 1},              // 0x3D
  {"UNDEF", 1},              // 0x3E
  {"UNDEF", 1},              // 0x3F
  {"UNDEF", 1},              // 0x40
  {"UNDEF", 1},              // 0x41
  {"UNDEF", 1},              // 0x42
  {"UNDEF", 1},              // 0x43
  {"UNDEF", 1},              // 0x44
  {"UNDEF", 1},              // 0x45
  {"LD B,(IY+d)", 3},        // 0x46
  {"UNDEF", 1},              // 0x47
  {"UNDEF", 1},              // 0x48
  {"UNDEF", 1},              // 0x49
  {"UNDEF", 1},              // 0x4A
  {"UNDEF", 1},              // 0x4B
  {"UNDEF", 1},              // 0x4C
  {"UNDEF", 1},              // 0x4D
  {"LD C,(IY+d)", 3},        // 0x4E
  {"UNDEF", 1},              // 0x4F
  {"UNDEF", 1},              // 0x50
  {"UNDEF", 1},              // 0x51
  {"UNDEF", 1},              // 0x52
  {"UNDEF", 1},              // 0x53
  {"UNDEF", 1},              // 0x54
  {"UNDEF", 1},              // 0x55
  {"LD D,(IY+d)", 3},        // 0x56
  {"UNDEF", 1},              // 0x57
  {"UNDEF", 1},              // 0x58
  {"UNDEF", 1},              // 0x59
  {"UNDEF", 1},              // 0x5A
  {"UNDEF", 1},              // 0x5B
  {"UNDEF", 1},              // 0x5C
  {"UNDEF", 1},              // 0x5D
  {"LD E,(IY+d)", 3},        // 0x5E
  {"UNDEF", 1},              // 0x5F
  {"UNDEF", 1},              // 0x60
  {"UNDEF", 1},              // 0x61
  {"UNDEF", 1},              // 0x62
  {"UNDEF", 1},              // 0x63
  {"UNDEF", 1},              // 0x64
  {"UNDEF", 1},              // 0x65
  {"LD H,(IY+d)", 3},        // 0x66
  {"UNDEF", 1},              // 0x67
  {"UNDEF", 1},              // 0x68
  {"UNDEF", 1},              // 0x69
  {"UNDEF", 1},              // 0x6A
  {"UNDEF", 1},              // 0x6B
  {"UNDEF", 1},              // 0x6C
  {"UNDEF", 1},              // 0x6D
  {"LD L,(IY+d)", 3},        // 0x6E
  {"UNDEF", 1},              // 0x6F
  {"LD (IY+d),B", 3},        // 0x70
  {"LD (IY+d),C", 3},        // 0x71
  {"LD (IY+d),D", 3},        // 0x72
  {"LD (IY+d),E", 3},        // 0x73
  {"LD (IY+d),H", 3},        // 0x74
  {"LD (IY+d),L", 3},        // 0x75
  {"UNDEF", 1},              // 0x76
  {"LD (IY+d),A", 3},        // 0x77
  {"UNDEF", 1},              // 0x78
  {"UNDEF", 1},              // 0x79
  {"UNDEF", 1},              // 0x7A
  {"UNDEF", 1},              // 0x7B
  {"UNDEF", 1},              // 0x7C
  {"UNDEF", 1},              // 0x7D
  {"LD A,(IY+d)", 3},        // 0x7E
  {"UNDEF", 1},              // 0x7F
  {"UNDEF", 1},              // 0x80
  {"UNDEF", 1},              // 0x81
  {"UNDEF", 1},              // 0x82
  {"UNDEF", 1},              // 0x83
  {"UNDEF", 1},              // 0x84
  {"UNDEF", 1},              // 0x85
  {"ADD A,(IY+d)", 3},       // 0x86
  {"UNDEF", 1},              // 0x87
  {"UNDEF", 1},              // 0x88
  {"UNDEF", 1},              // 0x89
  {"UNDEF", 1},              // 0x8A
  {"UNDEF", 1},              // 0x8B
  {"UNDEF", 1},              // 0x8C
  {"UNDEF", 1},              // 0x8D
  {"ADC A,(IY+d)", 3},       // 0x8E
  {"UNDEF", 1},              // 0x8F
  {"UNDEF", 1},              // 0x90
  {"UNDEF", 1},              // 0x91
  {"UNDEF", 1},              // 0x92
  {"UNDEF", 1},              // 0x93
  {"UNDEF", 1},              // 0x94
  {"UNDEF", 1},              // 0x95
  {"SUB A(IY+d)", 3},        // 0x96
  {"UNDEF", 1},              // 0x97
  {"UNDEF", 1},              // 0x98
  {"UNDEF", 1},              // 0x99
  {"UNDEF", 1},              // 0x9A
  {"UNDEF", 1},              // 0x9B
  {"UNDEF", 1},              // 0x9C
  {"UNDEF", 1},              // 0x9D
  {"SBC A,(IY+d)", 3},       // 0x9E
  {"UNDEF", 1},              // 0x9F
  {"UNDEF", 1},              // 0xA0
  {"UNDEF", 1},              // 0xA1
  {"UNDEF", 1},              // 0xA2
  {"UNDEF", 1},              // 0xA3
  {"UNDEF", 1},              // 0xA4
  {"UNDEF", 1},              // 0xA5
  {"AND A,(IY+d)", 3},       // 0xA6
  {"UNDEF", 1},              // 0xA7
  {"UNDEF", 1},              // 0xA8
  {"UNDEF", 1},              // 0xA9
  {"UNDEF", 1},              // 0xAA
  {"UNDEF", 1},              // 0xAB
  {"UNDEF", 1},              // 0xAC
  {"UNDEF", 1},              // 0xAD
  {"XOR A,(IY+d)", 3},       // 0xAE
  {"UNDEF", 1},              // 0xAF
  {"UNDEF", 1},              // 0xB0
  {"UNDEF", 1},              // 0xB1
  {"UNDEF", 1},              // 0xB2
  {"UNDEF", 1},              // 0xB3
  {"UNDEF", 1},              // 0xB4
  {"UNDEF", 1},              // 0xB5
  {"OR A,(IY+d)", 3},        // 0xB6
  {"UNDEF", 1},              // 0xB7
  {"UNDEF", 1},              // 0xB8
  {"UNDEF", 1},              // 0xB9
  {"UNDEF", 1},              // 0xBA
  {"UNDEF", 1},              // 0xBB
  {"UNDEF", 1},              // 0xBC
  {"UNDEF", 1},              // 0xBD
  {"CP (IY+d)", 3},          // 0xBE
  {"UNDEF", 1},              // 0xBF
  {"UNDEF", 1},              // 0xC0
  {"UNDEF", 1},              // 0xC1
  {"UNDEF", 1},              // 0xC2
  {"UNDEF", 1},              // 0xC3
  {"UNDEF", 1},              // 0xC4
  {"UNDEF", 1},              // 0xC5
  {"UNDEF", 1},              // 0xC6
  {"UNDEF", 1},              // 0xC7
  {"UNDEF", 1},              // 0xC8
  {"UNDEF", 1},              // 0xC9
  {"UNDEF", 1},              // 0xCA
  {"OPCODE_FD_CB", 1},       // 0xCB
  {"UNDEF", 1},              // 0xCC
  {"UNDEF", 1},              // 0xCD
  {"UNDEF", 1},              // 0xCE
  {"UNDEF", 1},              // 0xCF
  {"UNDEF", 1},              // 0xD0
  {"UNDEF", 1},              // 0xD1
  {"UNDEF", 1},              // 0xD2
  {"UNDEF", 1},              // 0xD3
  {"UNDEF", 1},              // 0xD4
  {"UNDEF", 1},              // 0xD5
  {"UNDEF", 1},              // 0xD6
  {"UNDEF", 1},              // 0xD7
  {"UNDEF", 1},              // 0xD8
  {"UNDEF", 1},              // 0xD9
  {"UNDEF", 1},              // 0xDA
  {"UNDEF", 1},              // 0xDB
  {"UNDEF", 1},              // 0xDC
  {"UNDEF", 1},              // 0xDD
  {"UNDEF", 1},              // 0xDE
  {"UNDEF", 1},              // 0xDF
  {"UNDEF", 1},              // 0xE0
  {"POP IY", 1},             // 0xE1
  {"UNDEF", 1},              // 0xE2
  {"EX (SP),IY", 2},         // 0xE3
  {"UNDEF", 1},              // 0xE4
  {"PUSH IY", 2},            // 0xE5
  {"UNDEF", 1},              // 0xE6
  {"UNDEF", 1},              // 0xE7
  {"UNDEF", 1},              // 0xE8
  {"JP (IY)", 2},            // 0xE9
  {"UNDEF", 1},              // 0xEA
  {"UNDEF", 1},              // 0xEB
  {"UNDEF", 1},              // 0xEC
  {"UNDEF", 1},              // 0xED
  {"UNDEF", 1},              // 0xEE
  {"UNDEF", 1},              // 0xEF
  {"UNDEF", 1},              // 0xF0
  {"UNDEF", 1},              // 0xF1
  {"UNDEF", 1},              // 0xF2
  {"UNDEF", 1},              // 0xF3
  {"UNDEF", 1},              // 0xF4
  {"UNDEF", 1},              // 0xF5
  {"UNDEF", 1},              // 0xF6
  {"UNDEF", 1},              // 0xF7
  {"UNDEF", 1},              // 0xF8
  {"LD SP,IY", 2},           // 0xF9
  {"UNDEF", 1},              // 0xFA
  {"UNDEF", 1},              // 0xFB
  {"UNDEF", 1},              // 0xFC
  {"UNDEF", 1},              // 0xFD
  {"UNDEF", 1},              // 0xFE
  {"UNDEF", 1},              // 0xFF
};

static DisAsmType Inst[] = {
  {"NOP", 1},          // 0x00
  {"LD BC,nn", 3},     // 0x01
  {"LD (BC),A", 1},    // 0x02
  {"INC BC", 1},       // 0x03
  {"INC B", 1},        // 0x04
  {"DEC B", 1},        // 0x05
  {"LD B,n", 2},       // 0x06
  {"RLCA", 1},         // 0x07
  {"EX AF,AF'", 1},    // 0x08
  {"ADD HL,BC", 1},    // 0x09
  {"LD A,(BC)", 1},    // 0x0A
  {"DEC BC", 1},       // 0x0B
  {"INC C", 1},        // 0x0C
  {"DEC C", 1},        // 0x0D
  {"LD C,n", 2},       // 0x0E
  {"RRCA", 1},         // 0x0F
  {"DJNZ e", 2},       // 0x10
  {"LD DE,nn", 3},     // 0x11
  {"LD (DE),A", 1},    // 0x12
  {"INC DE", 1},       // 0x13
  {"INC D", 1},        // 0x14
  {"DEC D", 1},        // 0x15
  {"LD D,n", 2},       // 0x16
  {"RLA", 1},          // 0x17
  {"JR e", 2},         // 0x18
  {"ADD HL,DE", 1},    // 0x19
  {"LD A,(DE)", 1},    // 0x1A
  {"DEC DE", 1},       // 0x1B
  {"INC E", 1},        // 0x1C
  {"DEC E", 1},        // 0x1D
  {"LD E,n", 2},       // 0x1E
  {"RRA", 1},          // 0x1F
  {"JR NZ,e", 2},      // 0x20
  {"LD HL,nn", 3},     // 0x21
  {"LD (nn),HL", 3},   // 0x22
  {"INC HL", 1},       // 0x23
  {"INC H", 1},        // 0x24
  {"DEC H", 1},        // 0x25
  {"LD H,n", 2},       // 0x26
  {"DAA", 1},          // 0x27
  {"JR Z,e", 2},       // 0x28
  {"ADD HL,HL", 1},    // 0x29
  {"LD HL,(nn)", 3},   // 0x2A
  {"DEC HL", 1},       // 0x2B
  {"INC L", 1},        // 0x2C
  {"DEC L", 1},        // 0x2D
  {"LD L,n", 2},       // 0x2E
  {"CPL", 1},          // 0x2F
  {"JR NC,e", 2},      // 0x30
  {"LD SP,nn", 3},     // 0x31
  {"LD (nn),A", 3},    // 0x32
  {"INC SP", 1},       // 0x33
  {"INC (HL)", 1},     // 0x34
  {"DEC (HL)", 1},     // 0x35
  {"LD (HL),n", 2},    // 0x36
  {"SCF", 1},          // 0x37
  {"JR C,e", 2},       // 0x38
  {"ADD HL,SP", 1},    // 0x39
  {"LD A,(nn)", 3},    // 0x3A
  {"DEC SP", 1},       // 0x3B
  {"INC A", 1},        // 0x3C
  {"DEC A", 1},        // 0x3D
  {"LD A,n", 2},       // 0x3E
  {"CCF", 1},          // 0x3F
  {"LD B,B", 1},       // 0x40
  {"LD B,C", 1},       // 0x41
  {"LD B,D", 1},       // 0x42
  {"LD B,E", 1},       // 0x43
  {"LD B,H", 1},       // 0x44
  {"LD B,L", 1},       // 0x45
  {"LD B,(HL)", 1},    // 0x46
  {"LD B,A", 1},       // 0x47
  {"LD C,B", 1},       // 0x48
  {"LD C,C", 1},       // 0x49
  {"LD C,D", 1},       // 0x4A
  {"LD C,E", 1},       // 0x4B
  {"LD C,H", 1},       // 0x4C
  {"LD C,L", 1},       // 0x4D
  {"LD C,(HL)", 1},    // 0x4E
  {"LD C,A", 1},       // 0x4F
  {"LD D,B", 1},       // 0x50
  {"LD D,C", 1},       // 0x51
  {"LD D,D", 1},       // 0x52
  {"LD D,E", 1},       // 0x53
  {"LD D,H", 1},       // 0x54
  {"LD D,L", 1},       // 0x55
  {"LD D,(HL)", 1},    // 0x56
  {"LD D,A", 1},       // 0x57
  {"LD E,B", 1},       // 0x58
  {"LD E,C", 1},       // 0x59
  {"LD E,D", 1},       // 0x5A
  {"LD E,E", 1},       // 0x5B
  {"LD E,H", 1},       // 0x5C
  {"LD E,L", 1},       // 0x5D
  {"LD E,(HL)", 1},    // 0x5E
  {"LD E,A", 1},       // 0x5F
  {"LD H,B", 1},       // 0x60
  {"LD H,C", 1},       // 0x61
  {"LD H,D", 1},       // 0x62
  {"LD H,E", 1},       // 0x63
  {"LD H,H", 1},       // 0x64
  {"LD H,L", 1},       // 0x65
  {"LD H,(HL)", 1},    // 0x66
  {"LD H,A", 1},       // 0x67
  {"LD L,B", 1},       // 0x68
  {"LD L,C", 1},       // 0x69
  {"LD L,D", 1},       // 0x6A
  {"LD L,E", 1},       // 0x6B
  {"LD L,H", 1},       // 0x6C
  {"LD L,L", 1},       // 0x6D
  {"LD L,(HL)", 1},    // 0x6E
  {"LD L,A", 1},       // 0x6F
  {"LD (HL),B", 1},    // 0x70
  {"LD (HL),C", 1},    // 0x71
  {"LD (HL),D", 1},    // 0x72
  {"LD (HL),E", 1},    // 0x73
  {"LD (HL),H", 1},    // 0x74
  {"LD (HL),L", 1},    // 0x75
  {"HALT", 1},         // 0x76
  {"LD (HL),A", 1},    // 0x77
  {"LD A,B", 1},       // 0x78
  {"LD A,C", 1},       // 0x79
  {"LD A,D", 1},       // 0x7A
  {"LD A,E", 1},       // 0x7B
  {"LD A,H", 1},       // 0x7C
  {"LD A,L", 1},       // 0x7D
  {"LD A,(HL)", 1},    // 0x7E
  {"LD A,A", 1},       // 0x7F
  {"ADD A,B", 1},      // 0x80
  {"ADD A,C", 1},      // 0x81
  {"ADD A,D", 1},      // 0x82
  {"ADD A,E", 1},      // 0x83
  {"ADD A,H", 1},      // 0x84
  {"ADD A,L", 1},      // 0x85
  {"ADD A,(HL)", 1},   // 0x86
  {"ADD A,A", 1},      // 0x87
  {"ADC A,B", 1},      // 0x88
  {"ADC A,C", 1},      // 0x89
  {"ADC A,D", 1},      // 0x8A
  {"ADC A,E", 1},      // 0x8B
  {"ADC A,H", 1},      // 0x8C
  {"ADC A,L", 1},      // 0x8D
  {"ADC A,(HL)", 1},   // 0x8E
  {"ADC A,A", 1},      // 0x8F
  {"SUB A,B", 1},      // 0x90
  {"SUB A,C", 1},      // 0x91
  {"SUB A,D", 1},      // 0x92
  {"SUB A,E", 1},      // 0x93
  {"SUB A,H", 1},      // 0x94
  {"SUB A,L", 1},      // 0x95
  {"SUB A,(HL)", 1},   // 0x96
  {"SUB A,A", 1},      // 0x97
  {"SBC A,B", 1},      // 0x98
  {"SBC A,C", 1},      // 0x99
  {"SBC A,D", 1},      // 0x9A
  {"SBC A,E", 1},      // 0x9B
  {"SBC A,H", 1},      // 0x9C
  {"SBC A,L", 1},      // 0x9D
  {"SBC A,(HL)", 1},   // 0x9E
  {"SBC A,A", 1},      // 0x9F
  {"AND A,B", 1},      // 0xA0
  {"AND A,C", 1},      // 0xA1
  {"AND A,D", 1},      // 0xA2
  {"AND A,E", 1},      // 0xA3
  {"AND A,H", 1},      // 0xA4
  {"AND A,L", 1},      // 0xA5
  {"AND A,(HL)", 1},   // 0xA6
  {"AND A,A", 1},      // 0xA7
  {"XOR A,B", 1},      // 0xA8
  {"XOR A,C", 1},      // 0xA9
  {"XOR A,D", 1},      // 0xAA
  {"XOR A,E", 1},      // 0xAB
  {"XOR A,H", 1},      // 0xAC
  {"XOR A,L", 1},      // 0xAD
  {"XOR A,(HL)", 1},   // 0xAE
  {"XOR A,A", 1},      // 0xAF
  {"OR A,B", 1},       // 0xB0
  {"OR A,C", 1},       // 0xB1
  {"OR A,D", 1},       // 0xB2
  {"OR A,E", 1},       // 0xB3
  {"OR A,H", 1},       // 0xB4
  {"OR A,L", 1},       // 0xB5
  {"OR A,(HL)", 1},    // 0xB6
  {"OR A,A", 1},       // 0xB7
  {"CP B", 1},         // 0xB8
  {"CP C", 1},         // 0xB9
  {"CP D", 1},         // 0xBA
  {"CP E", 1},         // 0xBB
  {"CP H", 1},         // 0xBC
  {"CP L", 1},         // 0xBD
  {"CP (HL)", 1},      // 0xBE
  {"CP A", 1},         // 0xBF
  {"RET nz", 1},       // 0xC0
  {"POP BC", 1},       // 0xC1
  {"JP nz,nn", 3},     // 0xC2
  {"JP nn", 3},        // 0xC3
  {"CALL nz,nn", 3},   // 0xC4
  {"PUSH BC", 1},      // 0xC5
  {"ADD A,n", 2},      // 0xC6
  {"RST 00", 1},       // 0xC7
  {"RET z", 1},        // 0xC8
  {"RET", 1},          // 0xC9
  {"JP z,nn", 3},      // 0xCA
  {"OPCODE CB", 1},    // 0xCB
  {"CALL z,nn", 3},    // 0xCC
  {"CALL nn", 3},      // 0xCD
  {"ADC A,n", 2},      // 0xCE
  {"RST 08", 1},       // 0xCF
  {"RET nc", 1},       // 0xD0
  {"POP DE", 1},       // 0xD1
  {"JP nc,nn", 3},     // 0xD2
  {"OUT (n),A", 2},    // 0xD3
  {"CALL nc,nn", 3},   // 0xD4
  {"PUSH DE", 1},      // 0xD5
  {"SUB A,n", 2},      // 0xD6
  {"RST 10", 1},       // 0xD7
  {"RET c", 1},        // 0xD8
  {"EXX", 1},          // 0xD9
  {"JP c,nn", 3},      // 0xDA
  {"IN A,(n)", 2},     // 0xDB
  {"CALL c,nn", 3},    // 0xDC
  {"OPCODE DD", 1},    // 0xDD
  {"SBC A,n", 2},      // 0xDE
  {"RST 18", 1},       // 0xDF
  {"RET npv", 1},      // 0xE0
  {"POP HL", 1},       // 0xE1
  {"JP npv,nn", 3},    // 0xE2
  {"EX (SP),HL", 1},   // 0xE3
  {"CALL npv,nn", 3},  // 0xE4
  {"PUSH HL", 1},      // 0xE5
  {"AND A,n", 2},      // 0xE6
  {"RST 20", 1},       // 0xE7
  {"RET pv", 1},       // 0xE8
  {"JP (HL)", 1},      // 0xE9
  {"JP pv,nn", 3},     // 0xEA
  {"EX DE,HL", 1},     // 0xEB
  {"CALL pv,nn", 3},   // 0xEC
  {"OPCODE ED", 1},    // 0xED
  {"XOR A,n", 2},      // 0xEE
  {"RST 28", 1},       // 0xEF
  {"RET ns", 1},       // 0xF0
  {"POP AF", 1},       // 0xF1
  {"JP ns,nn", 3},     // 0xF2
  {"DI", 1},           // 0xF3
  {"CALL ns,nn", 3},   // 0xF4
  {"PUSH AF", 1},      // 0xF5
  {"OR A,n", 2},       // 0xF6
  {"RST 30", 1},       // 0xF7
  {"RET s", 1},        // 0xF8
  {"LD SP,HL", 1},     // 0xF9
  {"JP s,nn", 3},      // 0xFA
  {"EI", 1},           // 0xFB
  {"CALL s,nn", 3},    // 0xFC
  {"OPCODE FD", 1},    // 0xFD
  {"CP n", 2},         // 0xFE
  {"RST 38", 1},       // 0xFF
};

/////////////////////////////////////////////////
int GetValue_nn(word addr)
{
  int nVal = ((*pMemRead)(addr+1) << 8) + (*pMemRead)(addr);
  return nVal;
}

/////////////////////////////////////////////////
int GetValue_n(word addr)
{
  int nVal = (*pMemRead)(addr);
  return nVal;
}

/////////////////////////////////////////////////
int GetValue_e(word addr)
{
  union {
    byte b;
    char n;
  } offset;

  offset.b = (*pMemRead)(addr);
  return offset.n;
}

/////////////////////////////////////////////////
int GetInstructionName(int pc, char* psz, int nMaxLen)
{
  BYTE by    = (*pMemRead)(pc);
  int  nSize = 1;

  switch (by)
  {
    case 0xCB:
      by = (*pMemRead)(pc+1);
      strcpy_s(psz, nMaxLen, InstCB[by].pszName);
      nSize = InstCB[by].nSize;
      break;

    case 0xDD:
      by = (*pMemRead)(pc+1);
      strcpy_s(psz, nMaxLen, InstDD[by].pszName);
      nSize = InstDD[by].nSize;
      break;

    case 0xED:
      by = (*pMemRead)(pc+1);
      strcpy_s(psz, nMaxLen, InstED[by].pszName);
      nSize = InstED[by].nSize;
      break;

    case 0xFD:
      by = (*pMemRead)(pc+1);

      if (by == 0xCB)
      {
        nSize = GetInstructionName_FD_CB(pc, psz, nMaxLen);
        break;
      }

      strcpy_s(psz, nMaxLen, InstFD[by].pszName);
      nSize = InstFD[by].nSize;
      break;

    //"LD BC,nn",     // 0x01
    case 0x01:
      sprintf_s(psz, nMaxLen, "LD BC,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"LD B,n",       // 0x06
    case 0x06:
      sprintf_s(psz, nMaxLen, "LD B,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"LD C,n",       // 0x0E
    case 0x0E:
      sprintf_s(psz, nMaxLen, "LD C,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"DJNZ e",       // 0x10
    case 0x10:
      sprintf_s(psz, nMaxLen, "DJNZ %04X", pc+GetValue_e(pc+1)+2);
      nSize = Inst[by].nSize;
      break;

    //"LD DE,nn",     // 0x11
    case 0x11:
      sprintf_s(psz, nMaxLen, "LD DE,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"LD D,n",       // 0x16
    case 0x16:
      sprintf_s(psz, nMaxLen, "LD D,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JR e",         // 0x18
    case 0x18:
      sprintf_s(psz, nMaxLen, "JR %04X", pc+GetValue_e(pc+1)+2);
      nSize = Inst[by].nSize;
      break;

    //"LD E,n",       // 0x1E
    case 0x1E:
      sprintf_s(psz, nMaxLen, "LD E,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JR NZ,e",      // 0x20
    case 0x20:
      sprintf_s(psz, nMaxLen, "JR NZ,%04X", pc+GetValue_e(pc+1)+2);
      nSize = Inst[by].nSize;
      break;

    //"LD HL,nn",     // 0x21
    case 0x21:
      sprintf_s(psz, nMaxLen, "LD HL,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"LD (nn),HL",   // 0x22
    case 0x22:
      sprintf_s(psz, nMaxLen, "LD (%04X),HL", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"LD H,n",       // 0x26
    case 0x26:
      sprintf_s(psz, nMaxLen, "LD H,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JR Z,e",       // 0x28
    case 0x28:
      sprintf_s(psz, nMaxLen, "JR Z,%04X", pc+GetValue_e(pc+1)+2);
      nSize = Inst[by].nSize;
      break;

    //"LD HL,(nn)",   // 0x2A
    case 0x2A:
      sprintf_s(psz, nMaxLen, "LD HL,(%04X)", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"LD L,n",       // 0x2E
    case 0x2E:
      sprintf_s(psz, nMaxLen, "LD L,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JR NC,e",      // 0x30
    case 0x30:
      sprintf_s(psz, nMaxLen, "JR NC,%04X", pc+GetValue_e(pc+1)+2);
      nSize = Inst[by].nSize;
      break;

    //"LD SP,nn",     // 0x31
    case 0x31:
      sprintf_s(psz, nMaxLen, "LD SP,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"LD (nn),A",    // 0x32
    case 0x32:
      sprintf_s(psz, nMaxLen, "LD (%04X),A", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"LD (HL),n",    // 0x36
    case 0x36:
      sprintf_s(psz, nMaxLen, "LD (HL),%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JR C,e",       // 0x38
    case 0x38:
      sprintf_s(psz, nMaxLen, "JR C,%04X", pc+GetValue_e(pc+1)+2);
      nSize = Inst[by].nSize;
      break;

    //"LD A,(nn)",    // 0x3A
    case 0x3A:
      sprintf_s(psz, nMaxLen, "LD A,(%04X)", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"LD A,n",       // 0x3E
    case 0x3E:
      sprintf_s(psz, nMaxLen, "LD A,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JP nz,nn",     // 0xC2
    case 0xC2:
      sprintf_s(psz, nMaxLen, "JP NZ,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JP nn",        // 0xC3
    case 0xC3:
      sprintf_s(psz, nMaxLen, "JP %04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"CALL nz,nn",   // 0xC4
    case 0xC4:
      sprintf_s(psz, nMaxLen, "CALL NZ,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"ADD A,n",      // 0xC6
    case 0xC6:
      sprintf_s(psz, nMaxLen, "ADD A,%04X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JP z,nn",      // 0xCA
    case 0xCA:
      sprintf_s(psz, nMaxLen, "JP Z,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"CALL z,nn",    // 0xCC
    case 0xCC:
      sprintf_s(psz, nMaxLen, "CALL Z,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"CALL nn",      // 0xCD
    case 0xCD:
      sprintf_s(psz, nMaxLen, "CALL %04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"ADC A,n",      // 0xCE
    case 0xCE:
      sprintf_s(psz, nMaxLen, "ADC A,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JP nc,nn",     // 0xD2
    case 0xD2:
      sprintf_s(psz, nMaxLen, "JP NC,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"OUT n,A",      // 0xD3
    case 0xD3:
      sprintf_s(psz, nMaxLen, "OUT %02X,A", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"CALL nc,nn",   // 0xD4
    case 0xD4:
      sprintf_s(psz, nMaxLen, "CALL NC,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"SUB A,n",      // 0xD6
    case 0xD6:
      sprintf_s(psz, nMaxLen, "SUB A,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JP c,nn",      // 0xDA
    case 0xDA:
      sprintf_s(psz, nMaxLen, "JP C,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"CALL c,nn",    // 0xDC
    case 0xDC:
      sprintf_s(psz, nMaxLen, "CALL C,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"SBC A,n",      // 0xDE
    case 0xDE:
      sprintf_s(psz, nMaxLen, "SBD A,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JP npv,nn",    // 0xE2
    case 0xE2:
      sprintf_s(psz, nMaxLen, "JP NPV,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"CALL npv,nn",  // 0xE4
    case 0xE4:
      sprintf_s(psz, nMaxLen, "CALL NPV,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"AND A,n",      // 0xE6
    case 0xE6:
      sprintf_s(psz, nMaxLen, "AND A,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JP pv,nn",     // 0xEA
    case 0xEA:
      sprintf_s(psz, nMaxLen, "JP PV,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"CALL pv,nn",   // 0xEC
    case 0xEC:
      sprintf_s(psz, nMaxLen, "CALL PV,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"XOR A,n",      // 0xEE
    case 0xEE:
      sprintf_s(psz, nMaxLen, "XOR A,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JP ns,nn",     // 0xF2
    case 0xF2:
      sprintf_s(psz, nMaxLen, "JP NS,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"CALL ns,nn",   // 0xF4
    case 0xF4:
      sprintf_s(psz, nMaxLen, "CALL NS,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"OR A,n",       // 0xF6
    case 0xF6:
      sprintf_s(psz, nMaxLen, "OR A,%02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"JP s,nn",      // 0xFA
    case 0xFA:
      sprintf_s(psz, nMaxLen, "JP S,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"CALL s,nn",    // 0xFC
    case 0xFC:
      sprintf_s(psz, nMaxLen, "CALL S,%04X", GetValue_nn(pc+1));
      nSize = Inst[by].nSize;
      break;

    //"CP n",         // 0xFE
    case 0xFE:
      sprintf_s(psz, nMaxLen, "CP %02X", GetValue_n(pc+1));
      nSize = Inst[by].nSize;
      break;

    default:
      sprintf_s(psz, nMaxLen, Inst[by].pszName);
      nSize = Inst[by].nSize;
      break;
  }

  return nSize;
}
