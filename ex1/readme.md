---
lang: pl-PL
---

# Przykład wykorzystujący nieblokujący connect()

Przykład składać się będzie z dwóch programów klienta i serwera komunikujących się za pomocą
protokołu TCP. Serwer będzie oczekiwał na próbę nawiązania połączenia z klientem. Dodatkowo
będzie on skonfigurowany w taki sposób, aby pakiety były wysyłane z opóźnieniem, co pozwoli
naocznie zaobserwować, że connect jest nieblokujący. Klient będzie próbował nawiązać połączenie
z serwerem wykorzystując `connect()` w wersji nieblokującej. Pomiędzy wywołaniem systemowym
a odpowiedzią o nawiązaniu połączenia program będzie wykonywał dodatkowe czynności. Ponieważ
głównym celem tego przykładu jest prezentacja sposobu realizacji nieblokującego `connect()` w celu
zachowania czytelności jedyną dodatkową czynnością będzie wyświetlenie stosownego komunikatu w
terminalu


## Uruchomienie

### serwer
Musi być uruchomiony jako pierwszy
```
docker compose run server
```
### klient
```
docker compose run client
```


## Dodanie opóźnienia po stronie serwera 

Dla lepszego uwidocznienia działania nieblokującego `connect()` można dodać opóźnienie po stronie serwera. Najprostszym sposobem jest wykorzystanie narzędzia `tc` wpisując komendę
```
docker compose exec server tc qdisc add dev eth0 root netem delay <time>
```
### Uwaga 
Czas opóżnienia nie może być większy niż 3 sekunedy.
w miejsce `time` wpisujemy czas opóźnienia w milisekundach.

