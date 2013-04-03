include ../../Makefile.inc

DIRS = $(shell ls -l | egrep ^d |awk '{printf "%s " , $$NF}')

.PHONY: all $(DIRS)

all: $(DIRS)

$(DIRS): 
	$(MAKE) -C $@
