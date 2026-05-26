# Тестирование

Тестирование выполняется shell-скриптами.

## Сборка

```bash
./scripts/build.sh
```

## Полный запуск тестов

```bash
./scripts/run_tests.sh
```

## Проверяемые сценарии

```text
1. Сборка проекта.
2. TCP stream запрос.
3. UDP dgram запрос.
4. Запрет пользователя не из whitelist.
5. Ошибка при выполнении команды.
```

## CI/CD

Основной CI-скрипт:

```bash
ci/test.sh
```

Он запускает:

```bash
./scripts/run_tests.sh
```

## Ручная проверка

Запуск сервера:

```bash
sudo systemctl start myrpc-server
```

Проверка статуса:

```bash
sudo systemctl status myrpc-server
```

Запрос с клиента:

```bash
myRPC-client -h 127.0.0.1 -p 1234 -s -c "whoami"
```
