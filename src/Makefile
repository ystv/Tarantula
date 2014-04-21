include ../Makefile.inc

DIRS = sqlite-amalgamation Common Optional MouseCatcher Tarantula Plugins libCaspar libPugiXML

.PHONY: all $(DIRS)

all: $(DIRS)

sqlite-amalgamation: 
	$(MAKE) -C $@
	
Common: 
	$(MAKE) -C $@

Optional: sqlite-amalgamation
	$(MAKE) -C $@
	
MouseCatcher: Common
	$(MAKE) -C $@
	
Tarantula: Common libPugiXML Optional MouseCatcher
	$(MAKE) -C $@
	
Plugins: Common Tarantula libCaspar Optional
	$(MAKE) -C $@

libCaspar:
	$(MAKE) -C $@
	
libPugiXML:
	$(MAKE) -C $@

