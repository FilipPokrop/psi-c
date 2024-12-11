---
lang: pl-PL
---

# Program Ping

Program ping stworzony w języku C.

## Uruchomienie
program można uruchomić z wykorzystaniem dockera 
```
docker compose run --rm client
```
## Użycie

```
python main.py [-h] [-c COUNT] [-s SIZE] [-w WAIT] destination

positional arguments:
  destination

options:
  -h, --help   show this help message and exit
  -c COUNT     zatrzymaj po otrzmaniu COUNT odpowiedzi
  -s SIZE      użyj SIZE jako liczby bitów do wysłania
  -w WAIT      czas oczekiwania na odpowiedź
```
