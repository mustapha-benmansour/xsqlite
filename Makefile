

CC ?= gcc
PREFIX ?= /usr/local

OPT= -O2 -Wall -fPIC  -g


CFLAGS  = -I$(PREFIX)/include/luajit-2.1 


LDFLAGS += -L/usr/lib -lluajit-5.1 -lssl -lcrypto

CFLAGS += -I$(PREFIX)/include
LDFLAGS += -L$(PREFIX)/lib -lsqlite



MOD = lsqlite
SRCS = $(MOD).c 
EXT = .so
OUT = $(MOD)

all: $(OUT)

$(OUT): $(SRCS)
	@echo CC $@
	@$(CC) $(OPT)  $(CFLAGS) -o $@ $^ $(LDFLAGS) 

test: $(OUT)
	luajit main.lua

clean:
	@rm -fv $(APP)