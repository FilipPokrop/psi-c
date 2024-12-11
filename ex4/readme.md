---
lang: pl-PL
---

# Program do snifowania pakietów

Program otworzy specjalne gniazdo pozwalające na odczytanie wszystkich pakietów nadesłanych
do hosta. Następnie analizować będzie kolejne nagłówki poszczególnych protokołów. Program ob-
sługuje tylko nagłówki IP w wersji 4. Następnie na podstawie zawartości pola protocol analizowany
będzie nagłówek jednego z protokołów warstwy wyższej. Dodatkowo podczas przetwarzania nagłów-
ków sprawdzane będą sumy kontrolne w celu walidacji otrzymanych danych. Ostatecznie informacje
uzyskane z analizy poszczególnych pakietów zostaną wyświetlone w terminalu.
Rozpoznaje i przetwarza nagłówki protokołów:
* TCP
* UDP
* ICMP

## Uruchomienie
program można uruchomić z wykorzystaniem dockera 
```
docker compose run --rm client
```
