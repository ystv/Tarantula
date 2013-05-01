include ../Makefile.inc

FILES = libs/system/src/error_code.cpp

LIBFILE = libs/libboost_system.a

OBJS = $(FILES:.cpp=.o)

INCLUDE := ./ libs/system/src

.PHONY: clean all

all: $(LIBFILE)

$(LIBFILE): $(OBJS)
	ar rcs $(LIBFILE) $(OBJS)

$(OBJS): $(FILES)
	$(CXX) $(COPTS) -o $@ $< -c -fPIC -DBOOST_SYSTEM_STATIC_LINK=\"1\"

clean:
	rm -f $(LIBFILE) $(OBJS)

