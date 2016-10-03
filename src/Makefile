CC = gcc
CPPFLAGS =
#CFLAGS = -std=c11 -D_XOPEN_SOURCE=700 -O2 -Wall
CFLAGS = -std=c11 -D_XOPEN_SOURCE=700 -O2 -Wall -Wextra -Wformat=2
CFLAGS += `pkg-config --cflags glib-2.0`
OBJFLAGS = -c
LDFLAGS =
LOADLIBES =
LDLIBS = `pkg-config --libs glib-2.0`

.DEFAULT: all
.PHONY: all
all: httpd

httpd: httpd.o client_connection.o page.o message.o util.o log.o
	$(CC) $(CFLAGS) httpd.o client_connection.o page.o message.o util.o log.o -o httpd $(LDLIBS) $(LDFLAGS)

httpd.o: httpd.c
	$(CC) $(OBJFLAGS) $(CFLAGS) httpd.c

request.o: message.c
	$(CC) $(OBJFLAGS) $(CFLAGS) message.c

client_connection.o: client_connection.c
	$(CC) $(OBJFLAGS) $(CFLAGS) client_connection.c

page.o: page.c
	$(CC) $(OBJFLAGS) $(CFLAGS) page.c

util.o: util.c
	$(CC) $(OBJFLAGS) $(CFLAGS) util.c

log.o: log.c
	$(CC) $(OBJFLAGS) $(CFLAGS) log.c

clean:
	rm -f *.o

distclean: clean
	rm -f httpd
