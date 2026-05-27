# myRPC

`myRPC` — учебный клиент-серверный проект на языке C.

Проект реализует простой механизм удалённого вызова команд через socket.

## Состав проекта

```text
myRPC-client  консольный клиент
myRPC-server  серверная программа
```

## Возможности

```text
TCP stream socket
UDP datagram socket
whitelist пользователей
конфигурационные файлы в /etc/myRPC
выполнение Bash-команд
обработка stdout и stderr
worker-процессы через fork
обработка сигналов
логирование
deb-пакеты
systemd-служба
тесты для CI/CD
```

## Структура

```text
src/client/      код клиента
src/server/      код сервера
src/common/      общий код
config/          примеры конфигурации
docs/            документация
scripts/         скрипты сборки и тестов
tests/           тесты
ci/              CI/CD скрипт
packaging/       структура deb-пакетов
dist/            собранные deb-пакеты
```

## Сборка

```bash
make all
```

## Очистка

```bash
make clean
```

## Сборка deb-пакетов

```bash
make deb
```

## Запуск сервера

```bash
myRPC-server
```

## Пример клиента

```bash
myRPC-client -h 127.0.0.1 -p 1234 -s -c "whoami"
```

## Документация

```text
docs/architecture.md  архитектура
docs/testing.md       тестирование
docs/install.md       установка
```
