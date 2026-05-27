CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=gnu11 -O2
PREFIX = /usr
DIST = dist

COMMON_SRC = common/protocol.c
COMMON_OBJ = common/protocol.o

.PHONY: all clean deb install-client install-server

all: client/myRPC-client server/myRPC-server

common/protocol.o: common/protocol.c common/protocol.h
	$(CC) $(CFLAGS) -c common/protocol.c -o common/protocol.o

client/myRPC-client: client/myRPC-client.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) client/myRPC-client.c $(COMMON_OBJ) -o client/myRPC-client

server/myRPC-server: server/myRPC-server.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) server/myRPC-server.c $(COMMON_OBJ) -o server/myRPC-server

clean:
	rm -f common/*.o client/myRPC-client server/myRPC-server
	rm -rf $(DIST)
	rm -f myrpc-client/usr/bin/myRPC-client myrpc-server/usr/bin/myRPC-server
	rm -f *.deb

deb: all
	mkdir -p $(DIST)
	cp client/myRPC-client myrpc-client/usr/bin/myRPC-client
	cp server/myRPC-server myrpc-server/usr/bin/myRPC-server
	chmod 0755 myrpc-client/usr/bin/myRPC-client myrpc-server/usr/bin/myRPC-server
	dpkg-deb --build myrpc-client $(DIST)/myrpc-client_1.0-1_amd64.deb
	dpkg-deb --build myrpc-server $(DIST)/myrpc-server_1.0-1_amd64.deb

install-client: client/myRPC-client
	install -D -m 0755 client/myRPC-client $(DESTDIR)$(PREFIX)/bin/myRPC-client

install-server: server/myRPC-server
	install -D -m 0755 server/myRPC-server $(DESTDIR)$(PREFIX)/bin/myRPC-server
	install -D -m 0644 server/myRPC.conf $(DESTDIR)/etc/myRPC/myRPC.conf
	install -D -m 0644 server/users.conf $(DESTDIR)/etc/myRPC/users.conf
	install -D -m 0644 server/myrpc-server.service $(DESTDIR)/lib/systemd/system/myrpc-server.service
