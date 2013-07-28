include ../../Makefile.inc

INCLUDE := ../../include ../../boost

NAME = MouseCatcher

COMMON = $(shell ls -m ../../build/Common-*.o|sed 's/,//g')

FILES = $(shell ls -1 *.cpp | awk '{printf "%s " ,$$0}')

all: $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o))

../../build/$(NAME)-%.o : %.cpp
	$(CXX) $(COPTS) $(COMMON) -fPIC -o $@ -c $< $(LIBS)

