# Главный Makefile для сборки клиента и сервера
.PHONY: all clean deb client server

all: client server

client:
	$(MAKE) -C client

server:
	$(MAKE) -C server

clean:
	$(MAKE) -C client clean
	$(MAKE) -C server clean
	rm -f *.deb

deb: client server
	$(MAKE) -C client deb
	$(MAKE) -C server deb
	mv client/*.deb ../ || true
	mv server/*.deb ../ || true
