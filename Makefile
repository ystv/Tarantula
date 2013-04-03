DIRS = src

INCLUDE=./include

include Makefile.inc

.PHONY: all $(DIRS) tests

all : $(DIRS)

$(DIRS) tests:
	$(MAKE) --directory $@


tests: src

ClipSniffer:
	$(MAKE) --directory ClipSniffer/src

clean : 
	rm -f build/*.o
	rm -f bin/Tarantula
	rm -f bin/libCasparTestApp
	rm -f bin/ClipSniffer
	rm -f bin/*.so
	rm -f tests/Test_LogTest_Info
	rm -f tests/Test_LogTest_Warn
	rm -f tests/Test_LogTest_Error
	rm -f tests/Test_LogTest_OMGWTF
	rm -f tests/Test_Crosspoint

stripped : all
	strip --only-keep-debug $< -o $<.dbg
	strip $<
