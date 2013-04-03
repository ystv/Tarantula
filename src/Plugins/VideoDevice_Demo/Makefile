include ../../../Makefile.inc

#through some shell magic, get the name of the current directory
PLUGINNAME = $(shell pwd |sed  's/\// /g' |awk '{print $$NF}')

FILES = $(shell ls -m *.cpp|sed 's/,//g')

EXTRAS = ../../../build/Common-TarantulaPlugin.o ../../../build/Tarantula-Device.o ../../../build/Tarantula-VideoDevice.o ../../../build/Common-Misc.o ../../../build/Tarantula-CallBackTools.o

all: $(addprefix ../../../bin/,$(addsuffix .so,$(PLUGINNAME)))

INCLUDE := ../../../include

../../../bin/$(PLUGINNAME).so : $(addprefix ../../../build/$(PLUGINNAME)-,$(FILES:.cpp=.o)) $(EXTRAS)
	$(CXX) $(COPTS) --shared -fPIC -o $@ $(COMMON_OBJS)  $(addprefix ../../../build/$(PLUGINNAME)-,$(FILES:.cpp=.o)) $(EXTRAS) -L../../../bin -lCaspar

../../../build/$(PLUGINNAME)-%.o: %.cpp
	$(CXX) $(COPTS) --shared -fPIC -o $@ -c $<