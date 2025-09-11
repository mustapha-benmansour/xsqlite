

CC ?= gcc
PREFIX ?= /usr/local

OPT= -O2 -Wall -fPIC -shared


CFLAGS = -I$(PREFIX)/include
CFLAGS += -I$(PREFIX)/include/luajit-2.1 
LDFLAGS = -L$(PREFIX)/lib -lluajit-5.1 -lsqlite3



MOD = xsqlite
SRCS = $(MOD).c 
EXT = .so
OUT = $(MOD)$(EXT)

all: $(OUT)

$(OUT): $(SRCS)
	@echo CC $@
	@$(CC) $(OPT)  $(CFLAGS) -o $@ $^ $(LDFLAGS) 

test: $(OUT)
	luajit main.lua

install:

clean:
	@rm -fv $(OUT)

