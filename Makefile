OUT:=libsilk.so
CFLAGS:=-Wall -Wextra -std=c99 -O2 -fPIC -fvisibility=hidden -g
PREFIX:=/usr/local

SRC:=$(wildcard src/*.c)
OBJ:=$(subst src/,build/,$(SRC:.c=.o))

all: $(OUT) silk

$(OUT): $(OBJ)
	$(CC) -shared -o $(OUT) $(OBJ)

silk: $(OUT)
	$(CC) -Wall -Wextra -std=c99 -o $@ -Iinclude -L. -lsilk silk.c

$(OBJ): | build

build:
	mkdir $@

build/%.o: src/%.c
	$(CC) $(CFLAGS) -Iinclude -c -o $@ $<

clean:
	rm -rf build $(OUT) silk

install: $(OUT) silk
	cp $(OUT) $(PREFIX)/lib
	cp include/silk.h $(PREFIX)/include
	cp silk $(PREFIX)/bin

uninstall:
	rm $(PREFIX)/lib/$(OUT)
	rm $(PREFIX)/include/silk.h
	rm $(PREFIX)/bin/silk

.PHONY: all clean install uninstall
