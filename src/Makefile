include ../Makefile.inc

DIRS = sqlite-amalgamation Common MouseCatcher Tarantula Plugins libCaspar libPugiXML

.PHONY: all $(DIRS)

all: $(DIRS)

sqlite-amalgamation: 
	$(MAKE) -C $@
	
Common: sqlite-amalgamation
	$(MAKE) -C $@
	
MouseCatcher: Common
	$(MAKE) -C $@
	
Tarantula: Common libPugiXML
	$(MAKE) -C $@
	
Plugins: Common Tarantula libCaspar
	$(MAKE) -C $@

libCaspar:
	$(MAKE) -C $@
	
libPugiXML:
	$(MAKE) -C $@

