#Executable makefile.

include ../../Makefile.inc

INCLUDE := ../../include ../../boost

NAME = $(shell pwd |sed  's/\// /g' |awk '{print $$NF}')

COMMON = $(shell ls -m ../../build/Common-*.o|sed 's/,//g')

EXTRAS = ../../build/MouseCatcher-* ../../build/Optional-*

FILES = $(shell ls -1 *.cpp | awk '{printf "%s " ,$$0}')

all: $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o)) ../../bin/Tarantula

../../build/$(NAME)-%.o : %.cpp
	$(CXX) $(COPTS) -fpic -o $@ -c $<

../../bin/Tarantula: $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o)) $(EXTRAS)
	$(CXX) -rdynamic $(COPTS) -o $@ $(COMMON) $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o)) $(EXTRAS) $(LIBS) -lpugixml -lrt -L../../bin
