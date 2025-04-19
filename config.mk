BS=\${empty}

PCBINDIR=$(top_srcdir)/pc/bin

PCMAKE = $(PCBINDIR)/pcmake
PCC = $(PCBINDIR)/m68k-atari-tos-pc-pcc
CPP = $(PCBINDIR)/m68k-atari-tos-pc-cpp

INCLUDES = '-I$(top_srcdir)' '-I$(top_srcdir)${BS}pc${BS}include'
