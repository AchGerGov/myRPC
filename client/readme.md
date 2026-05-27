# myRPC-client

## Сборка

```bash
make client/myRPC-client
```

## Использование

```bash
./client/myRPC-client -h 192.168.56.11 -p 1234 -s -c "whoami"
./client/myRPC-client --host 192.168.56.11 --port 1234 --stream --command "uname -a"
./client/myRPC-client -h 192.168.56.11 -p 1234 -d -c "date"
```
