#Executable makefile.

include ../../Makefile.inc

INCLUDE := ../../include ../include ../../src/sqlite-amalgamation

COMMON = ../../build/Optional-MemDB.o ../../build/Optional-SQLite-sqlite3.o ../../build/Common-Log.o

FILES = $(shell ls -1 *.cpp | awk '{printf "%s " ,$$0}')

all: $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o)) ../../bin/ClipSniffer

../../build/$(NAME)-%.o : %.cpp
	$(CXX) $(COPTS) -fpic $(shell pkg-config --cflags libavformat) -o $@ -c $<

../../bin/ClipSniffer: $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o))
	$(CXX) $(COPTS) -o $@ $(COMMON) $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o)) \
	$(LIBS) $(shell pkg-config --libs libavformat) -lpugixml -L../../bin

