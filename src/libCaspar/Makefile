include ../../Makefile.inc

MODNAME = libCaspar

FILES = $(shell ls -m *.cpp|sed 's/,//g')

EXTRAS = ../../build/Common-Misc.o

all: $(addprefix ../../bin/,$(addsuffix .so,$(MODNAME))) ../../bin/libCasparTestApp

INCLUDE := ../../include ../../boost

../../bin/$(MODNAME).so : $(addprefix ../../build/$(MODNAME)-,$(FILES:.cpp=.o)) $(EXTRAS)
	$(CXX) $(COPTS) --shared -fPIC -o $@ $(COMMON_OBJS)  $(addprefix ../../build/$(MODNAME)-,$(FILES:.cpp=.o)) $(EXTRAS) -L../../boost/libs -lboost_system

../../build/$(MODNAME)-%.o: %.cpp
	$(CXX) $(COPTS) --shared -fPIC -o $@ -c $<
	
../../bin/libCasparTestApp: ../../bin/$(MODNAME).so
	$(CXX) $(COPTS) -o $@ testApp.cpp -L../../bin -lCaspar $(LIBS) -lpugixml -L../../bin -L../../boost/libs -lboost_system
    