# SRS / SDD - T17 Analiza Semantica Cod Sursa

## 1. Introducere

Acest document descrie cerintele si designul aplicatiei pentru tema T17 - Analiza Semantica Cod Sursa.

Aplicatia are rolul de a analiza fisiere sursa C/C++ si de a returna informatii despre structura codului si diagnosticele generate. Proiectul este dezvoltat in C, pe Linux / WSL, si foloseste biblioteci externe relevante pentru analiza codului.

---

## 2. Scopul proiectului

Scopul aplicatiei este de a permite analizarea unui fisier sursa C/C++ folosind libclang. Sistemul poate rula local prin analyzer CLI, dar include si o prima arhitectura client-server, in care un client trimite fisierul catre server, iar serverul ruleaza analiza si returneaza raportul.

---

## 3. Domeniu

Aplicatia acopera urmatoarele componente:

- analyzer CLI
- server TCP
- client normal
- client admin
- protocol simplu de comunicare
- specificatie OpenAPI pentru o posibila interfata REST

---

## 4. Cerinte functionale

### CF1. Citire configuratie

Aplicatia trebuie sa citeasca setarile din fisierul `config/app.cfg`, folosind libconfig.

### CF2. Selectare fisier sursa

Analyzer-ul trebuie sa poata primi fisierul sursa prin:
- argument CLI: `-f <fisier>`
- variabila de mediu: `SOURCE_FILE`

### CF3. Analiza cod sursa

Analyzer-ul trebuie sa foloseasca libclang pentru parsarea fisierului sursa.

### CF4. Extragere statistici

Aplicatia trebuie sa afiseze informatii precum:
- numar de functii
- numar de variabile
- numar de instructiuni if
- numar de bucle for
- numar de bucle while
- numar de diagnostice

### CF5. Afisare diagnostice

Aplicatia trebuie sa poata afisa diagnosticele generate de libclang.

### CF6. Server TCP

Serverul trebuie sa accepte conexiuni TCP de la clienti.

### CF7. Transfer fisier client-server

Clientul normal trebuie sa poata trimite un fisier sursa catre server.

### CF8. Rulare analiza pe server

Serverul trebuie sa salveze fisierul primit si sa ruleze analyzer-ul intr-un proces separat.

### CF9. Returnare rezultat

Serverul trebuie sa trimita rezultatul analizei inapoi clientului.

### CF10. Client admin

Clientul admin trebuie sa poata cere statistici simple de la server.

---

## 5. Cerinte nefunctionale

- Aplicatia este implementata in limbajul C.
- Aplicatia ruleaza pe Linux / WSL.
- Compilarea se face cu gcc, folosind Makefile.
- Codul trebuie sa compileze fara warning-uri.
- Proiectul trebuie sa aiba comentarii explicative.
- Se folosesc biblioteci externe relevante: libconfig si libclang.
- Comunicarea client-server foloseste TCP.
- Structura proiectului trebuie sa fie simpla si usor de extins.

---

## 6. Arhitectura sistemului

Arhitectura actuala este formata din urmatoarele componente:

```txt
Client normal  -> Server TCP -> Analyzer -> Server TCP -> Client normal
Client admin   -> Server TCP -> Raspuns statistici
```

### Analyzer

Analyzer-ul este componenta care foloseste libclang pentru parsarea fisierului sursa. Acesta poate fi rulat direct din terminal sau poate fi apelat de server.

### Server TCP

Serverul primeste fisiere de la client, le salveaza in directorul `uploads/`, ruleaza analyzer-ul prin `fork`, `exec` si `pipe`, apoi returneaza rezultatul catre client.

### Client normal

Clientul normal trimite un fisier sursa catre server si afiseaza raportul primit.

### Client admin

Clientul admin trimite comanda `STATS` catre server si afiseaza statisticile primite.

---

## 7. Structura mesajelor

### Upload fisier

```txt
UPLOAD <filename> <size>
<file_bytes>
```

### Cerere statistici admin

```txt
STATS
```

### Raspuns server

```txt
RESULT <size>
<payload>
```

---

## 8. FCE - Fluxuri de executie

### FCE 1 - Analiza locala prin CLI

1. Utilizatorul ruleaza `./analyzer -f tests/sample.c -v`.
2. Aplicatia citeste configuratia din `config/app.cfg`.
3. Aplicatia parseaza fisierul cu libclang.
4. Aplicatia afiseaza raportul in consola.

### FCE 2 - Analiza fisier prin client-server

1. Serverul este pornit cu `./server`.
2. Clientul ruleaza `./client tests/sample.c`.
3. Clientul trimite fisierul catre server.
4. Serverul salveaza fisierul in `uploads/`.
5. Serverul porneste analyzer-ul intr-un proces separat.
6. Serverul citeste rezultatul prin pipe.
7. Serverul trimite raportul inapoi clientului.
8. Clientul afiseaza rezultatul.

### FCE 3 - Cerere statistici admin

1. Serverul este pornit.
2. Admin clientul ruleaza `./admin_client`.
3. Admin clientul trimite comanda `STATS`.
4. Serverul returneaza statusul si numarul de fisiere analizate.

### FCE 4 - Fisier cu warning-uri

1. Clientul trimite fisierul `tests/bad.c`.
2. Serverul ruleaza analyzer-ul.
3. Analyzer-ul detecteaza warning-uri.
4. Raportul contine diagnosticele generate de libclang.

---

## 9. Biblioteci externe

### libconfig

libconfig este folosita pentru citirea fisierului de configurare `config/app.cfg`.

### libclang

libclang este folosita pentru analiza codului sursa si pentru extragerea informatiilor din AST.

---

## 10. Specificatie OpenAPI

Specificatia OpenAPI este definita in fisierul:

```txt
docs/openapi.yaml
```

Aceasta descrie o posibila interfata REST pentru functionalitatile aplicatiei:
- analizare fisier
- statistici admin
- returnare raport
- verificare status server

---

## 11. Stadiul actual al implementarii

Sunt implementate urmatoarele componente:

- analyzer CLI
- server TCP minimal
- client normal
- client admin
- transfer fisier client -> server
- rezultat server -> client
- protocol documentat
- OpenAPI
- Makefile
- fisiere de test

---

## 12. Extensii viitoare

Pentru urmatoarele etape se pot adauga:

- autentificare user/admin
- logging server
- operatii admin suplimentare
- job queue
- client Python
- transfer bidirectional de fisiere
- server concurent pentru mai multi clienti
- API REST real
- analiza mai avansata a codului