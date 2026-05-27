# myRPC-server

`myRPC-server` — серверная программа на языке C.

Сервер принимает команды от клиента.
Пользователь проверяется по whitelist.
Если пользователь разрешён, сервер выполняет команду.

## Конфигурация

Основной файл:

```text
/etc/myRPC/myRPC.conf
```

Пример:

```ini
port = 1234
socket_type = stream
daemon = no
log_file = /var/log/myRPC-server.log
```

Файл разрешённых пользователей:

```text
/etc/myRPC/users.conf
```

Пример:

```text
student
admin
```

## Запуск

```bash
myRPC-server
```

## Запуск как служба

```bash
sudo systemctl start myrpc-server
sudo systemctl status myrpc-server
```
