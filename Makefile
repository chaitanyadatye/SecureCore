# Makefile

include ./config.in

all:
	cd main && $(MAKE) all
	cd hpc && $(MAKE) all
	cp scripts/clear_ipc bin

clean:
	cd main && $(MAKE) clean
	cd hpc && $(MAKE) clean
	cd bin && rm -f *

