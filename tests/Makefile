include ../Makefile.inc

INCLUDE := ../include

COMMON_OBJS = $(shell ls ../build/Common-*.o -m |sed 's/,//')

all: Test_LogTest_Info Test_LogTest_Warn Test_LogTest_Error Test_LogTest_OMGWTF Test_Crosspoint
	./Test_LogTest_Info
	./Test_LogTest_Warn
	./Test_LogTest_Error
	./Test_LogTest_OMGWTF
	./Test_Crosspoint

../build/Test-%.o: %.cpp
	$(CXX) $(COPTEXEC) $(COPTS) -DTest_Info -I../include -I./ -o $@ -c $<

Test_LogTest_Info : ../build/Test-Test_Log.o Test_LogTest.cpp ../build/Test-Test_Base.o $(COMMON_OBJS)
	$(CXX) $(COPTEXEC) $(COPTS) -DTest_Info -I../include -I./ -o $@ Test_Log.cpp Test_LogTest.cpp ../build/Test-Test_Base.o $(COMMON_OBJS)

Test_LogTest_Warn : ../build/Test-Test_Log.o Test_LogTest.cpp ../build/Test-Test_Base.o $(COMMON_OBJS)
	$(CXX) $(COPTEXEC) $(COPTS) -DTest_Warn -I../include -I./ -o $@ Test_Log.cpp Test_LogTest.cpp ../build/Test-Test_Base.o $(COMMON_OBJS)

Test_LogTest_Error : ../build/Test-Test_Log.o Test_LogTest.cpp ../build/Test-Test_Base.o $(COMMON_OBJS)
	$(CXX) $(COPTEXEC) $(COPTS) -DTest_Error -I../include -I./ -o $@ Test_Log.cpp Test_LogTest.cpp ../build/Test-Test_Base.o $(COMMON_OBJS)

Test_LogTest_OMGWTF : ../build/Test-Test_Log.o Test_LogTest.cpp ../build/Test-Test_Base.o $(COMMON_OBJS)
	$(CXX) $(COPTEXEC) $(COPTS) -DTest_OMGWTF -I../include -I./ -o $@ Test_Log.cpp Test_LogTest.cpp ../build/Test-Test_Base.o $(COMMON_OBJS)
	
Test_Crosspoint : ../build/Test-Test_Crosspoint.o ../build/Test-Test_Base.o ../build/Tarantula-CrosspointDevice.o ../build/Tarantula-Device.o $(COMMON_OBJS)
	$(CXX) $(COPTEXEC) $(COPTS) -I../include -I./ -o $@ ../build/Test-Test_Crosspoint.o ../build/Test-Test_Base.o ../build/Tarantula-CrosspointDevice.o ../build/Tarantula-Device.o $(COMMON_OBJS)