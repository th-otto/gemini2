/* GEM Resource C Source */

#include <aes.h>
#include "hello.h"

#if !defined(WHITEBAK)
#define WHITEBAK    0x0040
#endif
#if !defined(DRAW3D)
#define DRAW3D      0x0080
#endif

#define FLAGS9  0x0200
#define FLAGS10 0x0400
#define FLAGS11 0x0800
#define FLAGS12 0x1000
#define FLAGS13 0x2000
#define FLAGS14 0x4000
#define FLAGS15 0x8000
#define STATE8  0x0100
#define STATE9  0x0200
#define STATE10 0x0400
#define STATE11 0x0800
#define STATE12 0x1000
#define STATE13 0x2000
#define STATE14 0x4000
#define STATE15 0x8000

TEDINFO rs_tedinfo[] =
{ "Willkommen bei Gemini",
  "\0",
  "\0",
  IBM  , 0, TE_LEFT , 0x1180, 0, -1, 22, 1,
  "Version 123.3sasas",
  "\0",
  "\0",
  SMALL, 0, TE_CNTR , 0x1180, 0, 0, 19, 1
};

WORD RSIB0MASK[] =
{ 0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x1FFF, 0xFF80, 0x0000, 0x0000, 
  0x1FFF, 0xFFF1, 0xFF00, 0x0000, 
  0x7FFF, 0xFFFF, 0xFFC0, 0x0000, 
  0x7FFF, 0xFFFF, 0xFFC0, 0x0000, 
  0x7FFF, 0xFFFF, 0xFFC0, 0x0000, 
  0x7FFF, 0xFFFF, 0xFFC0, 0x0000, 
  0x7FFF, 0xFFFF, 0xFFC1, 0xF800, 
  0xFFFF, 0xFFFF, 0xFFE3, 0xFE00, 
  0xFFFF, 0xFFFF, 0xFFFF, 0xFE00, 
  0xFFFF, 0xFFFF, 0xFFE3, 0xFE00, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

WORD RSIB0DATA[] =
{ 0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x4000, 0x0020, 0x0000, 0x0000, 
  0x4000, 0x0020, 0x0000, 0x0000, 
  0x47FF, 0xFE20, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x47FF, 0xFE20, 0x0000, 0x0000, 
  0x4000, 0x00A0, 0x0000, 0x0000, 
  0x4000, 0x0020, 0x0000, 0x0000, 
  0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x1000, 0x0080, 0x0000, 0x0000, 
  0x1FFF, 0xFFF1, 0xFF00, 0x0000, 
  0x6000, 0x000E, 0x00C0, 0x0000, 
  0x4000, 0x3FC4, 0x0040, 0x0000, 
  0x5800, 0x0605, 0x8040, 0x0000, 
  0x4000, 0x0004, 0x0040, 0x0000, 
  0x465A, 0x6D9B, 0x6C01, 0xF800, 
  0xEFFF, 0xFFFF, 0xFFE2, 0x8600, 
  0x8800, 0x0000, 0x031E, 0x8200, 
  0xEFFF, 0xFFFF, 0xFEE3, 0xFE00, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

WORD RSIB1MASK[] =
{ 0x3FFF, 0xFFC7, 0xFE00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x7FFF, 0xFFEF, 0xFF00, 0x0000, 
  0x3FFF, 0xFFCF, 0xFF00, 0x0000, 
  0x1FFF, 0xFF8F, 0xFF00, 0x0000, 
  0x1FFF, 0xFF8F, 0xFF00, 0x0000, 
  0x0000, 0x000F, 0xFF00, 0x0000, 
  0x36B3, 0x68DF, 0xFF0F, 0xC000, 
  0x7FFF, 0xFFFF, 0xFE1F, 0xF000, 
  0x7FFF, 0xFFFF, 0xFFFF, 0xF000, 
  0x7FFF, 0xFFFF, 0xFE1F, 0xF000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

WORD RSIB1DATA[] =
{ 0x3FFF, 0xFFC7, 0xFE00, 0x0000, 
  0x4000, 0x0028, 0x0100, 0x0000, 
  0x4000, 0x0028, 0x0100, 0x0000, 
  0x47FF, 0xFE2B, 0xFD00, 0x0000, 
  0x4800, 0x0128, 0x6100, 0x0000, 
  0x4800, 0x0128, 0x0100, 0x0000, 
  0x4800, 0x0128, 0x0100, 0x0000, 
  0x4800, 0x012B, 0xFD00, 0x0000, 
  0x4800, 0x012A, 0x0500, 0x0000, 
  0x4800, 0x012A, 0x0500, 0x0000, 
  0x4800, 0x012B, 0xFD00, 0x0000, 
  0x4800, 0x0128, 0x0100, 0x0000, 
  0x4800, 0x0128, 0x0100, 0x0000, 
  0x4800, 0x0128, 0x0100, 0x0000, 
  0x4800, 0x0128, 0x0100, 0x0000, 
  0x47FF, 0xFE28, 0x0100, 0x0000, 
  0x4000, 0x00AA, 0xA900, 0x0000, 
  0x4000, 0x0029, 0x5500, 0x0000, 
  0x3FFF, 0xFFCA, 0xA900, 0x0000, 
  0x1555, 0x5589, 0x5500, 0x0000, 
  0x1FFF, 0xFF8A, 0xA900, 0x0000, 
  0x0000, 0x0009, 0x5500, 0x0000, 
  0x36B3, 0x68DB, 0x610F, 0xC000, 
  0x7FFF, 0xFFFF, 0xFE14, 0x3000, 
  0x4000, 0x0000, 0x01F4, 0x1000, 
  0x7FFF, 0xFFFF, 0xFE1F, 0xF000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

WORD RSIB2MASK[] =
{ 0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x1FFF, 0xFF80, 0x0000, 0x0000, 
  0x3FFF, 0xFFFF, 0xF800, 0x0000, 
  0x7FFF, 0xFFFF, 0xFC00, 0x0000, 
  0x7FFF, 0xFFFF, 0xFC00, 0x0000, 
  0xFFFF, 0xFFFF, 0xFE00, 0x0000, 
  0xFFFF, 0xFFFF, 0xFF81, 0xF800, 
  0xFFFF, 0xFFFF, 0xFE43, 0xFE00, 
  0xFFFF, 0xFFFF, 0xFE33, 0xFE00, 
  0x7FFF, 0xFFFF, 0xFC0F, 0xFE00, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

WORD RSIB2DATA[] =
{ 0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x4000, 0x0020, 0x0000, 0x0000, 
  0x4000, 0x0020, 0x0000, 0x0000, 
  0x47FF, 0xFE20, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x47FF, 0xFE20, 0x0000, 0x0000, 
  0x4000, 0x00A0, 0x0000, 0x0000, 
  0x4000, 0x0020, 0x0000, 0x0000, 
  0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x1000, 0x0080, 0x0000, 0x0000, 
  0x3FFF, 0xFFFF, 0xF800, 0x0000, 
  0x5555, 0x5555, 0x5400, 0x0000, 
  0x4000, 0x0000, 0x0400, 0x0000, 
  0x9B6D, 0xB76D, 0xB200, 0x0000, 
  0xB7FF, 0xFD91, 0xDB81, 0xF800, 
  0xFFFF, 0xFFFF, 0xFE42, 0x8600, 
  0x8000, 0x0000, 0x0232, 0x8200, 
  0x7FFF, 0xFFFF, 0xFC0F, 0xFE00, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

WORD RSIB3MASK[] =
{ 0x3FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x1FFF, 0xFF80, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0xFFFF, 0xFFF0, 0x0000, 0x0000, 
  0xFFFF, 0xFFF0, 0x0000, 0x0000, 
  0xFFFF, 0xFFF0, 0x0000, 0x0000, 
  0xFFFF, 0xFFF0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0xFFFF, 0xFFF0, 0x0000, 0x0000, 
  0xFFFF, 0xFFFB, 0x0001, 0xF800, 
  0xFFFF, 0xFFFF, 0x80E3, 0xFE00, 
  0xFFFF, 0xFFFF, 0x871F, 0xFE00, 
  0x7FFF, 0xFFFF, 0xB803, 0xFE00
};

WORD RSIB3DATA[] =
{ 0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x4000, 0x0020, 0x0000, 0x0000, 
  0x4000, 0x0020, 0x0000, 0x0000, 
  0x47FF, 0xFE20, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x4800, 0x0120, 0x0000, 0x0000, 
  0x47FF, 0xFE20, 0x0000, 0x0000, 
  0x4000, 0x00A0, 0x0000, 0x0000, 
  0x4000, 0x0020, 0x0000, 0x0000, 
  0x3FFF, 0xFFC0, 0x0000, 0x0000, 
  0x1000, 0x0080, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x8000, 0x0010, 0x0000, 0x0000, 
  0x8000, 0x3FD0, 0x0000, 0x0000, 
  0x9000, 0x0610, 0x0000, 0x0000, 
  0x8000, 0x0010, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x7FFF, 0xFFE0, 0x0000, 0x0000, 
  0x8000, 0x0010, 0x0000, 0x0000, 
  0x86DB, 0x6CDB, 0x0001, 0xF800, 
  0x8FFF, 0xFFFF, 0x80E2, 0x8600, 
  0x8800, 0x0000, 0x871E, 0x8200, 
  0x6FFF, 0xFFFF, 0xB803, 0xFE00
};

WORD RSIB4MASK[] =
{ 0x3FFF, 0xFFF0, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x3FFF, 0xFFF0, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0x7FFF, 0xFFF8, 0x0000, 0x0000, 
  0xFFFF, 0xFFFC, 0x0000, 0x0000, 
  0xFFFF, 0xFFFC, 0x0000, 0x0000, 
  0xFFFF, 0xFFFC, 0x0000, 0x0000, 
  0xFFFF, 0xFFFC, 0x0000, 0x0000, 
  0xFFFF, 0xFFFC, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

WORD RSIB4DATA[] =
{ 0x3FFF, 0xFFF0, 0x0000, 0x0000, 
  0x4000, 0x0008, 0x0000, 0x0000, 
  0x4000, 0x0008, 0x0000, 0x0000, 
  0x47FF, 0xFE08, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x4800, 0x0108, 0x0000, 0x0000, 
  0x47FF, 0xFE08, 0x0000, 0x0000, 
  0x4000, 0x0088, 0x0000, 0x0000, 
  0x4000, 0x0008, 0x0000, 0x0000, 
  0x3FFF, 0xFFF0, 0x0000, 0x0000, 
  0x4000, 0x0008, 0x0000, 0x0000, 
  0x76DB, 0x6DB8, 0x0000, 0x0000, 
  0x5B6D, 0xBB68, 0x0000, 0x0000, 
  0xB7FF, 0xEDB4, 0x0000, 0x0000, 
  0x8000, 0x0004, 0x0000, 0x0000, 
  0xFFFF, 0xFFFC, 0x0000, 0x0000, 
  0x8000, 0x0004, 0x0000, 0x0000, 
  0xFFFF, 0xFFFC, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

ICONBLK rs_iconblk[] =
{ RSIB0MASK, RSIB0DATA, "\0\000\000\000\000\000\000", 0x1000|'\0',
   29,  13,   0,   0,  64,  32,  14,  32,  36,   8,
  RSIB1MASK, RSIB1DATA, "\0\000\000\000\000\000\000", 0x1000|'\0',
   29,  13,   0,   0,  64,  32,  14,  32,  36,   8,
  RSIB2MASK, RSIB2DATA, "\0\000\000\000\000\000\000", 0x1000|'\0',
   29,  13,   0,   0,  64,  32,  14,  32,  36,   8,
  RSIB3MASK, RSIB3DATA, "\0\000\000\000\000\000\000", 0x1000|'\0',
   29,  13,   0,   0,  64,  32,  14,  32,  36,   8,
  RSIB4MASK, RSIB4DATA, "\0\000\000\000\000\000\000", 0x1000|'\0',
   29,  13,   0,   0,  64,  32,  14,  32,  36,   8
};

WORD RSBB0DATA[] =
{ 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x4000, 0x00E4, 0x4000, 
  0x0004, 0x4000, 0x0004, 0x0000, 
  0x0000, 0x0000, 0x0020, 0x8000, 
  0x001F, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

WORD RSBB1DATA[] =
{ 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 
  0x0040, 0x4000, 0x0044, 0x4000, 
  0x0044, 0x4000, 0x0004, 0x0000, 
  0x0000, 0x0000, 0x0020, 0x8000, 
  0x001F, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

BITBLK rs_bitblk[] =
{ RSBB0DATA,   4,  16,   0,   0, 0x0001,
  RSBB1DATA,   4,  16,   0,   0, 0x0001
};

OBJECT rs_object[] =
{ 
  /******** Tree 0 HELLOBOX ****************************************************/
        -1, IC_TT           , HELLOVER        , G_BOX             ,   /* Object 0 HTREE */
  NONE, NORMAL, (long)0x00011120L,
  0x0000, 0x0000, 0x0019, 0x0007,
  BLINK           ,       -1,       -1, G_ICON            ,   /* Object 1 IC_TT */
  NONE, NORMAL, (long)&rs_iconblk[0],
  0x0005, 0x0001, 0x0008, 0x0802,
  NORM            ,       -1,       -1, G_IMAGE           ,   /* Object 2 BLINK */
  NONE, NORMAL, (long)&rs_bitblk[0],
  0x0005, 0x0001, 0x0004, 0x0001,
  IC_TOWER        ,       -1,       -1, G_IMAGE           ,   /* Object 3 NORM */
  NONE, NORMAL, (long)&rs_bitblk[1],
  0x0005, 0x0001, 0x0004, 0x0001,
  IC_FALCO        ,       -1,       -1, G_ICON            ,   /* Object 4 IC_TOWER */
  HIDETREE, NORMAL, (long)&rs_iconblk[1],
  0x0005, 0x0001, 0x0008, 0x0802,
  IC_MEGA         ,       -1,       -1, G_ICON            ,   /* Object 5 IC_FALCO */
  HIDETREE, NORMAL, (long)&rs_iconblk[2],
  0x000E, 0x0001, 0x0008, 0x0802,
  IC_BOOK         ,       -1,       -1, G_ICON            ,   /* Object 6 IC_MEGA */
  HIDETREE, NORMAL, (long)&rs_iconblk[3],
  0x0005, 0x0003, 0x0008, 0x0802,
         8,       -1,       -1, G_ICON            ,   /* Object 7 IC_BOOK */
  HIDETREE, NORMAL, (long)&rs_iconblk[4],
  0x000E, 0x0003, 0x0008, 0x0802,
  HELLOVER        ,       -1,       -1, G_TEXT            ,   /* Object 8  */
  NONE, NORMAL, (long)&rs_tedinfo[0],
  0x0002, 0x0005, 0x0015, 0x0001,
  HTREE           ,       -1,       -1, G_TEXT            ,   /* Object 9 HELLOVER */
  LASTOB, NORMAL, (long)&rs_tedinfo[1],
  0x0400, 0x0006, 0x0418, 0x0001
};

OBJECT *rs_trindex[] =
{ &rs_object[0]    /* Tree  0 HELLOBOX         */
};
