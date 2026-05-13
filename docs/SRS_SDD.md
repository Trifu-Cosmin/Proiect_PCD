# SRS / SDD - T17 Analiza Semantica Cod Sursa

## 1. Introducere

Acest document descrie cerintele software si designul aplicatiei pentru tema T17 - Analiza Semantica Cod Sursa, dezvoltata in cadrul materiei PCD.

Aplicatia are rolul de a analiza fisiere sursa C/C++ si de a returna informatii despre structura codului, erori, warning-uri si alte elemente detectate cu ajutorul bibliotecii libclang.

Proiectul este implementat in C pe Linux / WSL si foloseste o arhitectura client-server bazata pe socket-uri TCP.

---

## 2. Scopul proiectului

Scopul proiectului este dezvoltarea unei aplicatii care permite:

- trimiterea unui fisier sursa de la client catre server
- analizarea fisierului pe server
- generarea unui raport de analiza
- returnarea raportului catre client
- administrarea serverului printr-un client admin
- utilizarea unui client suplimentar in alt limbaj, respectiv Python

Aplicatia reprezinta o baza functionala pentru un sistem distribuit de analiza semantica a codului sursa.

---

## 3. Domeniul aplicatiei

Aplicatia este formata din urmatoarele componente principale:

- analyzer CLI
- server TCP
- client normal in C
- client admin in C
- client normal in Python
- fisiere de configurare
- protocol de comunicare TCP
- documentatie OpenAPI pentru o posibila extensie REST

---

## 4. Descriere generala

Fluxul principal al aplicatiei este:

```txt
Client -> Server -> Analyzer -> Server -> Client
```

Clientul trimite un fisier sursa catre server. Serverul salveaza fisierul primit, ruleaza analyzer-ul intr-un proces separat, preia rezultatul analizei si il trimite inapoi clientului.

Clientul admin poate trimite comenzi administrative catre server, precum afisarea statisticilor, logurilor, fisierelor uploadate si rapoartelor generate.

Clientul Python implementeaza o varianta alternativa de client normal, folosind acelasi protocol TCP.

---

## 5. Cerinte functionale

### CF1. Citire configuratie

Aplicatia trebuie sa citeasca setarile analyzer-ului din fisierul:

```txt
config/app.cfg
```

Acest lucru se realizeaza folosind biblioteca externa `libconfig`.

---

### CF2. Selectare fisier sursa pentru analyzer

Analyzer-ul trebuie sa poata primi fisierul sursa prin:

```txt
-f <fisier>
```

sau prin variabila de mediu:

```txt
SOURCE_FILE
```

Exemple:

```bash
./analyzer -f tests/sample.c -v
```

```bash
export SOURCE_FILE=tests/sample.c
./analyzer
```

---

### CF3. Analiza codului sursa

Analyzer-ul trebuie sa foloseasca `libclang` pentru parsarea codului C/C++.

Analyzer-ul trebuie sa parcurga AST-ul generat de libclang si sa extraga informatii despre structura codului.

---

### CF4. Extragere statistici din cod

Aplicatia trebuie sa afiseze urmatoarele informatii:

- numar functii
- numar variabile
- numar instructiuni `if`
- numar bucle `for`
- numar bucle `while`
- numar diagnostice

---

### CF5. Afisare diagnostice

Aplicatia trebuie sa afiseze diagnosticele generate de libclang, precum:

- warning-uri
- erori de sintaxa
- erori de tip
- variabile neinitializate

---

### CF6. Server TCP

Serverul trebuie sa foloseasca socket-uri TCP si sa porneasca pe:

```txt
127.0.0.1:8080
```

Serverul trebuie sa accepte conexiuni de la clienti.

---

### CF7. Tratare concurenta clienti

Serverul trebuie sa trateze clientii concurent folosind `fork()`.

Pentru fiecare conexiune acceptata:

1. serverul principal face `fork()`
2. procesul copil trateaza clientul
3. procesul parinte continua sa accepte alti clienti
4. procesele copil terminate sunt curatate pentru a evita procese zombie

---

### CF8. Autentificare user/admin

Serverul trebuie sa accepte comanda:

```txt
LOGIN <username> <password>
```

Datele de autentificare sunt citite din:

```txt
config/users.cfg
```

Format:

```txt
username:password:role
```

Exemplu:

```txt
user1:pass1:user
admin:admin123:admin
```

---

### CF9. Separare roluri

Aplicatia trebuie sa separe operatiile in functie de rol:

Rol `user`:
- upload fisier
- download raport

Rol `admin`:
- statistici server
- loguri server
- listare fisiere uploadate
- listare rapoarte generate

---

### CF10. Upload fisier client-server

Clientul normal trebuie sa poata trimite un fisier catre server folosind comanda:

```txt
UPLOAD <filename> <size>
<file_bytes>
```

Serverul salveaza fisierul in:

```txt
uploads/
```

---

### CF11. Rulare analyzer pe server

Serverul trebuie sa ruleze analyzer-ul intr-un proces separat folosind:

- `fork()`
- `exec`
- `pipe`
- `waitpid()`

Output-ul analyzer-ului este preluat prin pipe de catre server.

---

### CF12. Returnare raport catre client

Dupa analiza, serverul trebuie sa trimita raportul catre client folosind raspuns de tip:

```txt
RESULT <size>
<payload>
```

---

### CF13. Salvare rapoarte

Serverul trebuie sa salveze rapoartele generate in directorul:

```txt
reports/
```

Exemple:

```txt
sample_report.txt
bad_report.txt
```

---

### CF14. Download raport server-client

Clientul trebuie sa poata descarca un raport generat anterior folosind comanda:

```txt
DOWNLOAD_REPORT <report_name>
```

Serverul trimite raportul folosind raspuns de tip:

```txt
FILE <filename> <size>
<file_bytes>
```

Clientul C salveaza fisierele descarcate in:

```txt
downloads/
```

Clientul Python salveaza fisierele descarcate in:

```txt
downloads_py/
```

---

### CF15. Client admin

Clientul admin trebuie sa ofere urmatoarele operatii:

- `STATS`
- `LOGS`
- `LIST_UPLOADS`
- `LIST_REPORTS`
- `QUIT`

Clientul admin se autentifica automat cu userul `admin`.

---

### CF16. Logging server

Serverul trebuie sa scrie evenimente importante in:

```txt
logs/server.log
```

Exemple de evenimente:

- server pornit
- client conectat
- autentificare reusita
- fisier uploadat
- raport generat
- raport descarcat
- comanda admin executata
- erori aparute

---

### CF17. Statistici persistente

Serverul trebuie sa pastreze statistici simple in:

```txt
logs/stats.txt
```

Acest fisier este folosit pentru a pastra numarul de fisiere analizate si ultimul fisier analizat.

---

### CF18. Client Python

Aplicatia trebuie sa includa si un client in alt limbaj.

Clientul Python trebuie sa poata:

- face login automat ca user normal
- trimite un fisier pentru analiza
- descarca un raport generat de server

Exemple:

```bash
python3 python_client/client.py upload tests/sample.c
python3 python_client/client.py download sample_report.txt
```

---

## 6. Cerinte nefunctionale

### CNF1. Limbaj de programare

Componentele principale sunt implementate in C.

Clientul suplimentar este implementat in Python.

---

### CNF2. Sistem de operare

Aplicatia este dezvoltata si testata pe Linux / WSL.

---

### CNF3. Compilare fara warning-uri

Proiectul trebuie sa compileze fara warning-uri folosind:

```bash
make clean
make
```

Compilarea foloseste flags:

```txt
-Wall -Wextra -Werror -pedantic -std=c11
```

---

### CNF4. Structura modulara

Codul este impartit in fisiere separate:

```txt
src/analyzer.c
src/main.c
src/server.c
src/client.c
src/admin_client.c
python_client/client.py
```

---

### CNF5. Documentatie cod

Fisierele sursa contin comentarii explicative pentru functiile importante si pentru rolul fiecarei componente.

---

### CNF6. Configurare externa

Setarile analyzer-ului sunt definite in `config/app.cfg`.

Utilizatorii sunt definiti in `config/users.cfg`.

---

### CNF7. Extensibilitate

Proiectul este conceput astfel incat sa poata fi extins ulterior cu:

- job queue
- operatii admin suplimentare
- API REST real
- analiza semantica mai avansata
- transfer de fisiere mari in chunks
- sesiuni persistente

---

## 7. Arhitectura sistemului

Arhitectura actuala este:

```txt
               +----------------+
               |  Client C      |
               +--------+-------+
                        |
                        | TCP
                        v
+----------------+      +----------------+      +----------------+
| Client Python  +----->|  Server TCP    +----->|   Analyzer     |
+----------------+      +--------+-------+      +----------------+
                                 |
                                 |
                                 v
                        +----------------+
                        | Admin Client C |
                        +----------------+
```

Serverul este componenta centrala. Acesta primeste cereri de la clienti, verifica autentificarea, proceseaza comenzile si ruleaza analyzer-ul cand este necesar.

---

## 8. Componente software

### 8.1 Analyzer

Fisier:

```txt
src/analyzer.c
```

Rol:

- citeste configuratia cu libconfig
- primeste fisierul sursa prin CLI sau variabila de mediu
- foloseste libclang pentru analiza codului
- afiseaza statistici si diagnostice

Exemplu rulare:

```bash
./analyzer -f tests/sample.c -v
```

---

### 8.2 Main launcher

Fisier:

```txt
src/main.c
```

Rol:

- pastreaza compatibilitatea cu executabilul `main`
- lanseaza executabilul `analyzer`
- permite rularea veche:

```bash
./main -f tests/sample.c -v
```

---

### 8.3 Server TCP

Fisier:

```txt
src/server.c
```

Rol:

- porneste serverul TCP
- accepta conexiuni
- trateaza clienti concurent folosind `fork()`
- verifica autentificarea
- primeste fisiere
- ruleaza analyzer-ul prin `fork/exec/pipe`
- trimite raspunsuri catre clienti
- salveaza rapoarte
- scrie loguri
- raspunde la comenzi admin

---

### 8.4 Client normal C

Fisier:

```txt
src/client.c
```

Rol:

- se conecteaza la server
- face login automat cu `user1/pass1`
- trimite fisiere pentru analiza
- descarca rapoarte

Exemple:

```bash
./client tests/sample.c
./client download sample_report.txt
```

---

### 8.5 Client admin C

Fisier:

```txt
src/admin_client.c
```

Rol:

- se conecteaza la server
- face login automat cu `admin/admin123`
- trimite comenzi administrative
- afiseaza raspunsurile serverului

Comenzi disponibile:

```txt
STATS
LOGS
LIST_UPLOADS
LIST_REPORTS
QUIT
```

---

### 8.6 Client Python

Fisier:

```txt
python_client/client.py
```

Rol:

- client alternativ in alt limbaj
- foloseste acelasi protocol TCP
- face login ca user normal
- permite upload si download raport

Exemple:

```bash
python3 python_client/client.py upload tests/sample.c
python3 python_client/client.py download sample_report.txt
```

---

## 9. Structura proiectului

```txt
Proiect_PCD/
├── config/
│   ├── app.cfg
│   └── users.cfg
├── docs/
│   ├── openapi.yaml
│   ├── protocol.md
│   └── SRS_SDD.md
├── python_client/
│   └── client.py
├── src/
│   ├── analyzer.c
│   ├── main.c
│   ├── server.c
│   ├── client.c
│   └── admin_client.c
├── tests/
│   ├── sample.c
│   ├── bad.c
│   ├── good_math.c
│   ├── good_while.c
│   ├── good_nested.c
│   ├── bad_uninitialized.c
│   ├── bad_syntax.c
│   └── bad_type.c
├── Makefile
├── README.md
└── .gitignore
```

---

## 10. Structura mesajelor

### 10.1 LOGIN

```txt
LOGIN <username> <password>
```

Exemplu:

```txt
LOGIN user1 pass1
```

Raspuns posibil:

```txt
RESULT <size>
OK role=user
```

---

### 10.2 UPLOAD

```txt
UPLOAD <filename> <size>
<file_bytes>
```

Exemplu:

```txt
UPLOAD sample.c 582
```

---

### 10.3 DOWNLOAD_REPORT

```txt
DOWNLOAD_REPORT <report_name>
```

Exemplu:

```txt
DOWNLOAD_REPORT sample_report.txt
```

---

### 10.4 STATS

```txt
STATS
```

---

### 10.5 LOGS

```txt
LOGS
```

---

### 10.6 LIST_UPLOADS

```txt
LIST_UPLOADS
```

---

### 10.7 LIST_REPORTS

```txt
LIST_REPORTS
```

---

### 10.8 RESULT

```txt
RESULT <size>
<payload>
```

---

### 10.9 FILE

```txt
FILE <filename> <size>
<file_bytes>
```

---

## 11. Fluxuri de executie

### FCE1 - Analiza locala prin analyzer CLI

1. Utilizatorul ruleaza:

```bash
./analyzer -f tests/sample.c -v
```

2. Analyzer-ul citeste configuratia din `config/app.cfg`.
3. Analyzer-ul parseaza fisierul cu libclang.
4. Analyzer-ul parcurge AST-ul.
5. Analyzer-ul afiseaza statisticile si diagnosticele.

---

### FCE2 - Upload si analiza fisier prin client C

1. Serverul este pornit cu:

```bash
./server
```

2. Clientul ruleaza:

```bash
./client tests/sample.c
```

3. Clientul se conecteaza la server.
4. Clientul trimite `LOGIN user1 pass1`.
5. Serverul autentifica userul.
6. Clientul trimite comanda `UPLOAD`.
7. Serverul salveaza fisierul in `uploads/`.
8. Serverul ruleaza analyzer-ul intr-un proces separat.
9. Serverul salveaza raportul in `reports/`.
10. Serverul trimite raportul catre client.
11. Clientul afiseaza rezultatul.

---

### FCE3 - Download raport prin client C

1. Clientul ruleaza:

```bash
./client download sample_report.txt
```

2. Clientul se conecteaza la server.
3. Clientul trimite login.
4. Clientul trimite `DOWNLOAD_REPORT sample_report.txt`.
5. Serverul cauta raportul in `reports/`.
6. Serverul trimite fisierul folosind raspuns `FILE`.
7. Clientul salveaza fisierul in `downloads/`.

---

### FCE4 - Operatie admin STATS

1. Admin clientul ruleaza:

```bash
./admin_client
```

2. Adminul alege optiunea `Server stats`.
3. Clientul admin trimite login cu `admin/admin123`.
4. Serverul verifica rolul `admin`.
5. Clientul admin trimite `STATS`.
6. Serverul citeste statisticile din `logs/stats.txt`.
7. Serverul trimite raspunsul catre admin client.

---

### FCE5 - Operatie admin LOGS

1. Adminul alege optiunea `Show server logs`.
2. Clientul admin trimite `LOGS`.
3. Serverul citeste `logs/server.log`.
4. Serverul trimite logurile catre clientul admin.

---

### FCE6 - Client Python upload

1. Utilizatorul ruleaza:

```bash
python3 python_client/client.py upload tests/sample.c
```

2. Clientul Python se conecteaza la server.
3. Clientul Python trimite login.
4. Clientul Python trimite fisierul.
5. Serverul analizeaza fisierul.
6. Clientul Python afiseaza raportul.

---

### FCE7 - Tratarea concurenta a clientilor

1. Serverul asteapta conexiuni.
2. Un client se conecteaza.
3. Serverul face `fork()`.
4. Procesul copil trateaza clientul.
5. Procesul parinte ramane liber pentru alti clienti.
6. La final, procesul copil se inchide.
7. Serverul curata procesele copil terminate pentru a evita procese zombie.

---

## 12. Biblioteci si apeluri folosite

### libconfig

Folosita pentru citirea fisierului:

```txt
config/app.cfg
```

Rol:

- nume proiect
- argumente pentru clang
- optiunea de afisare diagnostice

---

### libclang

Folosita pentru:

- parsare cod sursa
- generare AST
- detectare diagnostice
- numarare functii, variabile si structuri de control

---

### Socket-uri TCP

Folosite pentru comunicarea client-server.

Apeluri importante:

- `socket`
- `bind`
- `listen`
- `accept`
- `connect`
- `send`
- `recv`

---

### Procese si comunicare interna

Folosite pentru server concurent si rularea analyzer-ului.

Apeluri importante:

- `fork`
- `exec`
- `pipe`
- `dup2`
- `waitpid`

---

## 13. Date si fisiere generate la runtime

### uploads/

Contine fisierele primite de la client.

### reports/

Contine rapoartele generate de analyzer.

### logs/server.log

Contine logurile serverului.

### logs/stats.txt

Contine statistici persistente.

### downloads/

Contine rapoartele descarcate de clientul C.

### downloads_py/

Contine rapoartele descarcate de clientul Python.

---

## 14. Testare

### Testare build

```bash
make clean
make
```

Rezultat asteptat:

- compilare fara erori
- compilare fara warning-uri

---

### Testare analyzer

```bash
./analyzer -f tests/good_math.c -v
./analyzer -f tests/good_while.c -v
./analyzer -f tests/good_nested.c -v
```

Rezultat asteptat:

```txt
Diagnostics count: 0
```

Pentru fisiere cu probleme:

```bash
./analyzer -f tests/bad_uninitialized.c -v
./analyzer -f tests/bad_syntax.c -v
./analyzer -f tests/bad_type.c -v
```

Rezultat asteptat:

- warning-uri sau erori intentionate
- raport generat corect

---

### Testare client-server

Terminal 1:

```bash
./server
```

Terminal 2:

```bash
./client tests/sample.c
./client tests/bad.c
./client download sample_report.txt
```

Terminal 3:

```bash
./admin_client
```

Terminal 4:

```bash
python3 python_client/client.py upload tests/sample.c
python3 python_client/client.py download sample_report.txt
```

---

## 15. Specificatie OpenAPI

Specificatia OpenAPI este disponibila in:

```txt
docs/openapi.yaml
```

Aceasta descrie o posibila extensie REST pentru sistem.

Endpoint-uri documentate:

- `POST /auth/login`
- `POST /analyze`
- `GET /reports/{reportId}`
- `GET /admin/stats`
- `GET /admin/logs`
- `GET /admin/uploads`
- `GET /admin/reports`
- `GET /health`

---

## 16. Limitari curente

Implementarea actuala este functionala, dar are cateva limitari:

- autentificarea este simpla si bazata pe fisier text
- parolele nu sunt criptate
- nu exista sesiuni persistente
- serverul proceseaza o comanda principala per conexiune
- nu exista job queue real
- nu exista API REST implementat efectiv, doar specificatie OpenAPI
- transferul fisierelor mari nu este optimizat complet pentru sute de MB
- analiza semantica este de baza si poate fi extinsa

---

## 17. Directii viitoare

Proiectul poate fi extins cu:

- job queue
- analiza complexitatii mai avansata
- detectare variabile nefolosite
- detectare posibile memory leaks
- API REST real
- hashing parole
- sesiuni pentru utilizatori
- operatii admin suplimentare
- stergere rapoarte
- curatare loguri
- transfer fisiere mari in chunks
- interfata web

---

## 18. Concluzie

Aplicatia implementeaza o solutie functionala pentru analiza semantica de baza a codului sursa C/C++.

Sistemul include:

- analyzer bazat pe libclang
- configurare cu libconfig
- server TCP concurent
- client normal C
- client admin C
- client Python
- autentificare user/admin
- upload si download fisiere
- loguri si rapoarte
- protocol documentat
- specificatie OpenAPI

Prin aceste componente, proiectul respecta directia temei T17 si ofera o baza clara pentru extinderi ulterioare.