CLEANDIRS = common exec_job mupfel nls runner2 venus xrsrc

all::
	$(MAKE) clean
	$(MAKE) -C mupfel
	$(MAKE) clean
	$(MAKE) -C venus
	$(MAKE) -C runner2
	$(MAKE) clean

clean::
	for i in $(CLEANDIRS); do $(MAKE) -C $$i clean; done
