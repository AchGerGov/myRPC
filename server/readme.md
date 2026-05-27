# myRPC-server

## Сборка

```bash
make server/myRPC-server
```

## Конфиги

- `/etc/myRPC/myRPC.conf` — порт, тип сокета, режим демона.
- `/etc/myRPC/users.conf` — список разрешенных пользователей, один логин на строку.

## Запуск вручную

```bash
sudo ./server/myRPC-server -f -c ./server/myRPC.conf -l ./myrpc.log
```

## Запуск службой

```bash
sudo systemctl enable --now myrpc-server
```
