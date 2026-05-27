# myRPC-client

`myRPC-client` — консольная утилита на языке C.

Она отправляет команду Bash на сервер `myRPC-server`.

## Аргументы

```bash
-h, --host       IP-адрес сервера
-p, --port       порт сервера
-c, --command    команда Bash
-s, --stream     использовать TCP stream socket
-d, --dgram      использовать UDP datagram socket
--help           справка
```

## Пример

```bash
myRPC-client -h 192.168.1.10 -p 1234 -s -c "whoami"
```

## Сборка

```bash
make all
```
