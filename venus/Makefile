top_srcdir = ..
include $(top_srcdir)/config.mk

all: gemini.app

gemini.app: gemini.prj ../mupfel/mupfel.o
	$(PCMAKE) $<

clean::
	rm -f *.o *.i *.pdb

# for some strange reason, m68k-atari-tos-pc-pcc hangs when invoked directly
../mupfel/mupfel.i: ../mupfel/mupfel.c
	cd ../mupfel && $(CPP) $(INCLUDES) -DMERGED=1 -Omupfel.i mupfel.c || rm $@

../mupfel/mupfel.o: ../mupfel/mupfel.i
	cd ../mupfel && $(PCC) -P -Omupfel.o mupfel.i
