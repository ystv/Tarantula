DIRS= libs/system libs/date_time

FILES = DateConversions.cpp

OBJS = $(FILES:.cpp=.o)

LIBFILE = libs/libdateconversions.a

INCLUDE := ./

include ../Makefile.inc

.PHONY: clean all $(DIRS) 

all: $(DIRS) $(LIBFILE)

$(DIRS):
	$(MAKE) --directory $@
	
$(OBJS): $(FILES)
	$(CXX) $(COPTS) -fPIC -o $@ $< -c -fPIC
	
$(LIBFILE): $(OBJS)
	ar rcs $(LIBFILE) $(OBJS) libs/date_time/src/gregorian/greg_month.o

clean:
	$(MAKE) clean --directory libs/system
	$(MAKE) clean --directory libs/date_time
	rm -f $(OBJS:.cpp=.o)
	rm -f $(LIBFILE)
