MOPT ?= 
OOPT ?= 

#COPTEXEC only used on executables, not libs 
COPTEXEC ?=

COPTS= $(MOPT) $(OOPT) -ggdb -pg -std=c++0x -Wl,-rpath=\$$ORIGIN -Wall $(addprefix -I,$(INCLUDE))

LIBS= -lpthread -ldl 

ifeq ($(STATICCPP),STATICCPP) 
COPTS := -static-libstdc++ $(COPTS) 
endif 

ifeq ($(STATICC),STATICC) 
COPTS := -static-libgcc $(COPTS) COPTEXEC := -static $(COPTEXEC) 
endif 

CXX ?= g++
