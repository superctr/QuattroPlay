#include "../drv/quattro.h"
#include "q_detect.h"
/*
  This is just a work in progress for now....
  Currently just going to document the sound driver versions.
  This will be implemented once I run a 'test suite' to accurately
  describe the differences and capabilities of the sound drivers, since
  disassembling every single one is just going to take too much time.
*/

#define CAP_1_2 0
#define CAP_1_7 0
#define CAP_1_A 0
#define CAP_2_0 0
#define CAP_3_B 0
#define CAP_NCV1 0
#define CAP_0N01 0
#define CAP_0N03 0
#define CAP_0N04 0

// Chronological table
const Q_Version Q_VersionTable[] =
{
    { Q_MCUTYPE_C74, 0,     0,     -1,         "Quattro Ver1.2", "c74_bios", CAP_1_2 }, // 1993-07-01 (C74 BIOS)
    { Q_MCUTYPE_C75, 0,     0,     -1,         "Quattro Ver1.2", "c75_bios", CAP_1_2 }, // 1993-07-01 (C75 BIOS)
    { Q_MCUTYPE_C74, 0,     0x1c0, 0xf5b94230, "Quattro Ver1.7", "c74_p1",   CAP_1_7 },
    { Q_MCUTYPE_C75, 0,     0x1c6, 0x6753c1fa, "Quattro Ver1.7", "c75_p1",   CAP_1_7 },
    { Q_MCUTYPE_C74, 0,     0x1c0, 0xd1f30f3b, "Quattro Ver1.8", "c74_p2",   CAP_1_7 },
    { Q_MCUTYPE_C75, 0,     0x1cc, 0xacb90b23, "Quattro Ver1.8", "c75_p2",   CAP_1_7 },
    { Q_MCUTYPE_C76, 0,     0,     -1,         "Quattro Ver3.0", "c76_bios", CAP_1_7 }, // 1994-08-10 (C76 BIOS)
    { Q_MCUTYPE_C74, 0,     0x356, 0x9b401279, "Quattro Ver1.A", "c74_p3",   CAP_1_A },
    { Q_MCUTYPE_C75, 0,     0x256, 0x4f98f373, "Quattro Ver1.A", "c75_p3",   CAP_1_A },
    { Q_MCUTYPE_C74, 0,     0x37c, 0x9f89f785, "Quattro Ver2.0", "c74_p4",   CAP_2_0 },
    { Q_MCUTYPE_C75, 0,     0x27d, 0x4bd9c373, "Quattro Ver2.0", "c75_p4",   CAP_2_0 },
    { Q_MCUTYPE_C76, 0,     0x1fc, 0x4c4f1c46, "Quattro Ver3.5", "c76_p1",   CAP_2_0 },
    { Q_MCUTYPE_SS22,0xc000,0x4000,0x9129abb6, "QuattroVer5.1B", "s22_v120", CAP_2_0 }, // 1994-12-21 (S22-BIOS ver1.20)
    { Q_MCUTYPE_C76, 0,     0x212, 0x4c4f1c46, "Quattro Ver3.7", "c76_p2",   CAP_2_0 },
    { Q_MCUTYPE_SS22,0xc000,0x4000,0xd2494e60, "Quattro Ver5.2", "s22_v130a",CAP_2_0 }, // 1995-04-20 (S22-BIOS ver1.30) One byte difference, idk where
    { Q_MCUTYPE_SS22,0xc000,0x4000,0xa4b9f552, "Quattro Ver5.2", "s22_v130", CAP_2_0 }, // 1995-04-20 (S22-BIOS ver1.30)
    { Q_MCUTYPE_C76, 0,     0x500, 0x5346ab5a, "Quattro Ver3.B", "c76_p3",   CAP_3_B },
    { Q_MCUTYPE_ND,  0,     0x8000,0xfb9c0b1f, "Quattro Ver1.2.H8", "ncv1",  CAP_NCV1}, // not yet sure what version this actually corresponds to
    { Q_MCUTYPE_ND,  0,     0x8000,0x5018a455, "Q00N01ND",       "ncv2",     CAP_0N01}, // 1995-Q2/Q3
    { Q_MCUTYPE_SS22,0xc000,0x4000,0x9ea8befd, "QX0NX3SS",       "s22_v141", CAP_0N03}, // 1995-08-29 (S22-BIOS ver1.30)
    { Q_MCUTYPE_C74, 0,     0xc50, 0x89a8798c, "Q01N0422",       "c74_p5",   CAP_0N04},
    { Q_MCUTYPE_C76, 0,     0xb38, 0xf10b481b, "Q01N0411",       "c76_p4",   CAP_0N04},

};
