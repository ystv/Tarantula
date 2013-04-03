include ../../Makefile.inc

INCLUDE := ../../include

NAME = Common-SQLite

FILES = sqlite3.c

all: $(addprefix ../../build/$(NAME)-,$(FILES:.c=.o))

../../build/$(NAME)-%.o : %.c
	gcc $(COPTS) -fPIC -o $@ -c $< $(LIBS)

