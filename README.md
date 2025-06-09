# Siemens filesystem extractor

## Щито это
Тулза, позволяет извлекать содержимое файловой системы из FULLFLASH

Поддерживаемые платформы:
* X65
* X75

В разработке/планах на 2035:
* X85
* X55

## Как это использовать
```./ffshit <options> <fullflash path>```

### Параметры:

```
> ./ffshit -h
Siemens filesystem extractor
Usage:
  ./ffshit [OPTION...] positional parameters

  -d, --debug         Enable debugging
  -p, --path arg      Destination path. Data_<Model>_<IMEI> by default
  -m, --platform arg  Specify platform (disable autodetect).
                      [ X85 X75 X65 ]
  -h, --help          Help
```