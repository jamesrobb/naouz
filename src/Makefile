CC = gcc
CPPFLAGS =
CFLAGS = -std=c11 -D_XOPEN_SOURCE=700 -O2 -Wall -Wextra -Wformat=2
CFLAGS += `pkg-config --cflags --libs glib-2.0`
LDFLAGS =
LOADLIBES =
LDLIBS =

.DEFAULT: all
.PHONY: all
all: httpd

clean:
	rm -f *.o

distclean: clean
	rm -f httpd
