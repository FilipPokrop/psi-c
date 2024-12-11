---
lang: pl-PL
---

# Przykład prezentujący wysyłanie danych spoza pasma OOB

Przykład składać się będzie z dwóch programów klienta i serwera. Klient po nawiązaniu po-
łączenia rozpocznie przesyłanie dużej ilości danych do serwera. Serwer odbierając dane, będzie
usypiany na pewien czas, co ma symulować długotrwałe przetwarzanie danych. Docelowo serwer
będzie przetwarzał dane wolniej, niż klient wysyłał, co będzie skutkowało powolnym zapełnianiem
bufora. W normalnej sytuacji po zakończeniu połączenia serwer przetwarzałby dane jeszcze przez
jakiś czas. W tym przypadku klient jako ostatnią wiadomość wysyłać będzie dane spoza pasma, które
oznaczać będą zakończenie połączenia. Gdy serwer otrzyma taką wiadomość, będzie mógł zamknąć
połączenie.

## Uruchomienie
serwer musi być uruchomiony jako pierwszy

### serwer
uruchomieni kontenera
```
docker compose run server
```
możliwe użycie
```
main.py [-h] [-w WAIT] [-s SIZE]

opcje:
  -h, --help            show this help message and exit
  -w WAIT, --wait WAIT  czas potrzebny do przetworzenia
                        jednego pakietu
  -s SIZE, --size SIZE  wielkość danych przetwarzanych 
                        jednorazowo


```

### klient
uruchomienie kontenera
```
docker compose run client
```
możliwe użycie
```
main.py [-h] [-w WAIT] [-s SIZE] [destination]

argumenty pozycyjne:
  destination - domyślnie 'server'

opcje:
  -h, --help            show this help message and exit
  -w WAIT, --wait WAIT  czas po jakim wysyłany jest kolejny poakiet
  -s SIZE, --size SIZE  wielkość pojedynczego pakietu

```

