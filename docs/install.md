# Установка myRPC

## Сборка deb-пакетов

```bash
make deb
```

После сборки пакеты находятся в каталоге:

```text
dist/
```

## Установка сервера

На серверной машине:

```bash
sudo dpkg -i dist/myrpc-server_1.0_amd64.deb
```

## Установка клиента

На клиентской машине:

```bash
sudo dpkg -i dist/myrpc-client_1.0_amd64.deb
```

## Настройка сервера

Файл конфигурации:

```bash
sudo nano /etc/myRPC/myRPC.conf
```

Пример:

```ini
port = 1234
socket_type = stream
daemon = no
log_file = /var/log/myRPC-server.log
```

Файл whitelist:

```bash
sudo nano /etc/myRPC/users.conf
```

Пример:

```text
student
admin
```

## Запуск службы

```bash
sudo systemctl daemon-reload
sudo systemctl enable myrpc-server
sudo systemctl start myrpc-server
```

## Проверка

```bash
sudo systemctl status myrpc-server
```

## Запрос с клиента

```bash
myRPC-client -h IP_СЕРВЕРА -p 1234 -s -c "whoami"
```

## Логи

```bash
tail -f /var/log/myRPC-server.log
```
