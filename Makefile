CC      := $(shell command -v musl-gcc 2>/dev/null || echo gcc)
CFLAGS  := -O2 -Wall -Wextra
LDFLAGS := -static

syswatch: syswatch.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f syswatch

.PHONY: clean
