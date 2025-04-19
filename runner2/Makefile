top_srcdir = ..
include $(top_srcdir)/config.mk

all:: runner2.app

runner2.app: runner2.prj
	$(PCMAKE) $<

clean::
	rm -f *.o *.lib *.i *.pdb
