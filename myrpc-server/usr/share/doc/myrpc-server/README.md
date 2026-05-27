# myRPC

Учебный механизм удаленного выполнения bash-команд через сокеты.

## Состав

- `myRPC-client` — консольный клиент.
- `myRPC-server` — сервер/демон.
- Транспорт: TCP stream или UDP dgram.
- Протокол: текстовый `login: escaped_command`.

## Сборка

```bash
make all
```

## DEB-пакеты

```bash
make deb
```

Пакеты появятся в каталоге `dist/`.
