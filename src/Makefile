CC = gcc
CPPFLAGS =
CFLAGS = -std=c11 -D_XOPEN_SOURCE=700 -O2 -Wall
#CFLAGS = -std=c11 -D_XOPEN_SOURCE=700 -O2 -Wall -Wextra -Wformat=2
CFLAGS += `pkg-config --cflags --libs glib-2.0`
OBJFLAGS = -c
LDFLAGS =
LOADLIBES =
LDLIBS =

.DEFAULT: all
.PHONY: all
all: httpd

httpd: httpd.o request.o util.o
	$(CC) httpd.o request.o util.o -o httpd

httpd.o: httpd.c
	$(CC) $(OBJFLAGS) $(CFLAGS) httpd.c

request.o: request.c
	$(CC) $(OBJFLAGS) $(CFLAGS) request.c

util.o: util.c
	$(CC) $(OBJFLAGS) $(CFLAGS) util.c

clean:
	rm -f *.o

distclean: clean
	rm -f httpd
