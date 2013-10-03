include ../../Makefile.inc

INCLUDE := ../../include

NAME = Optional-SQLite

FILES = sqlite3.c

all: $(addprefix ../../build/$(NAME)-,$(FILES:.c=.o))

../../build/$(NAME)-%.o : %.c
	gcc $(COPTS) -DSQLITE_DEFAULT_MEMSTATUS=0 -fPIC -o $@ -c $< $(LIBS)

