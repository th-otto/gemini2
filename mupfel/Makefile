top_srcdir = ..
include $(top_srcdir)/config.mk

all: mupfel.ttp

mupfel.ttp: mupfel.prj mupfel.o
	$(PCMAKE) $<

clean::
	rm -f *.o *.lib *.i *.pdb

# for some strange reason, m68k-atari-tos-pc-pcc hangs when invoked directly
mupfel.i: mupfel.c
	$(CPP) -I.. $(INCLUDES) -O$@ $< || rm -f $@

mupfel.o: mupfel.i
	$(PCC) -P -O$@ $<
