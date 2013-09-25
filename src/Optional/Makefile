include ../../Makefile.inc

INCLUDE := ../../include ../sqlite-amalgamation/ ../../boost

NAME = Optional

FILES = $(shell ls -1 *.cpp | awk '{printf "%s " ,$$0}')

all: $(addprefix ../../build/$(NAME)-,$(FILES:.cpp=.o))

../../build/$(NAME)-%.o : %.cpp
	$(CXX) $(COPTS) -fPIC -o $@ -c $< 
