include ../../Makefile.inc

MODNAME = libpugixml

FILES = $(shell ls -m *.cpp|sed 's/,//g')

EXTRAS = 

all: $(addprefix ../../bin/,$(addsuffix .so,$(MODNAME)))

INCLUDE := ../../include

../../bin/$(MODNAME).so : $(addprefix ../../build/$(MODNAME)-,$(FILES:.cpp=.o)) $(EXTRAS)
	$(CXX) $(COPTS) --shared -fPIC -o $@  $(addprefix ../../build/$(MODNAME)-,$(FILES:.cpp=.o)) $(EXTRAS)

../../build/$(MODNAME)-%.o: %.cpp
	$(CXX) $(COPTS) --shared -fPIC -o $@ -c $<
    
