#Executable makefile.

include ../../Makefile.inc

INCLUDE := ../../include ../include ../../src/sqlite-amalgamation

COMMON = ../../build/Common-MemDB.o ../../build/Common-SQLite-sqlite3.o ../../build/Common-Log.o

FILES = $(shell ls -1 *.cpp | awk '{printf "%s " ,$$0}')

all: $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o)) ../../bin/ClipSniffer

../../build/$(NAME)-%.o : %.cpp
	$(CXX) $(COPTS) -fpic -o $@ -c $<

../../bin/ClipSniffer: $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o))
	$(CXX) $(COPTS) -o $@ $(COMMON) $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o)) $(LIBS) -lavcodec -lavformat -lavutil -lpugixml -L../../bin

