# BackupService

## Сборка (на Linux)
    sudo apt install mingw-w64
    make

Отладка: к любой команде можно добавить `--debug` — тогда каждый вызов каждой функции пишет в `service.log` строку с именем файла и функции. `install --debug` — служба всегда пишет отладку, `start --debug` — только на этот запуск.

## Что из чего состоит
| Файл | Что делает |
|---|---|
| `src/main.cpp` | смотрит на команду (install / start / stop / remove / без аргументов) |
| `src/scp.cpp` | SCP: установка, запуск, остановка, удаление службы |
| `src/service.cpp` | сама служба: ServiceMain, ControlHandler, главный цикл |
| `src/config.cpp` | читает config.ini |
| `src/mask.cpp` | проверяет имя файла по маске с `*` и `?` |
| `src/scanner.cpp` | обходит папку со всеми подпапками (сам архив пропускает) |
| `src/zip.cpp` | читает и обновляет ZIP-архив вручную, средствами WinAPI (без 7zip) |
| `src/log.cpp` | пишет service.log рядом с exe |
