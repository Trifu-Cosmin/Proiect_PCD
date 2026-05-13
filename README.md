# PCD T17 - Semantic Analyzer

## Descriere proiect

Acest proiect implementeaza o aplicatie client-server in limbajul C pentru analiza semantica a codului sursa C/C++.

Aplicatia permite trimiterea unui fisier sursa catre un server TCP, rularea unei analize folosind libclang si returnarea raportului catre client. Pe langa clientul normal exista si un client de administrare, folosit pentru operatii simple asupra serverului.

Proiectul foloseste:
- libconfig pentru citirea configuratiei
- libclang pentru analiza codului sursa
- getopt pentru parsarea argumentelor din linia de comanda
- socket-uri TCP pentru comunicarea client-server
- fork, exec si pipe pentru rularea analyzer-ului intr-un proces separat
- autentificare simpla pentru separarea userilor normali de admin
- fork per client pentru tratarea concurenta a conexiunilor
- client Python ca exemplu de client in alt limbaj

Proiectul este dezvoltat pentru tema T17 - Analiza Semantica Cod Sursa, din cadrul materiei PCD.

---

## Functionalitati implementate

- Analyzer CLI pentru fisiere C/C++
- Citire configuratie din `config/app.cfg`
- Selectare fisier prin argument CLI sau variabila de mediu
- Analiza codului cu libclang
- Numarare:
  - functii
  - variabile
  - instructiuni if
  - bucle for
  - bucle while
- Afisare diagnostice generate de libclang
- Server TCP
- Tratare concurenta a clientilor folosind `fork()`
- Client normal in C
- Client admin in C
- Client normal in Python
- Autentificare user/admin prin `config/users.cfg`
- Separare roluri:
  - user normal pentru upload si download raport
  - admin pentru statistici, loguri si listare fisiere
- Upload fisier client -> server
- Salvare fisiere uploadate in `uploads/`
- Salvare rapoarte in `reports/`
- Download raport server -> client
- Logging server in `logs/server.log`
- Protocol documentat in `docs/protocol.md`
- Specificatie OpenAPI in `docs/openapi.yaml`
- Document SRS/SDD in `docs/SRS_SDD.md`
- Fisiere de test pentru cod valid si cod cu erori/warning-uri

---

## Structura proiectului

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

## Cerinte

- Linux / WSL
- gcc
- make
- libconfig
- libclang
- python3

---

## Instalare dependente pe WSL / Ubuntu

```bash
sudo apt update
sudo apt install build-essential libconfig-dev libclang-dev clang python3 -y
```

---

## Compilare

```bash
make clean
make
```

Comanda genereaza urmatoarele executabile:

```txt
analyzer
main
server
client
admin_client
```

---

## Configurare analyzer

Fisierul `config/app.cfg` contine setarile pentru analyzer:

```cfg
project_name = "PCD T17 Semantic Analyzer";

clang_args = (
  "-std=c11",
  "-Wall",
  "-Wextra",
  "-pedantic"
);

show_diagnostics = true;
```

---

## Autentificare

Fisierul `config/users.cfg` contine utilizatorii aplicatiei.

Format:

```txt
username:password:role
```

Exemplu:

```txt
user1:pass1:user
admin:admin123:admin
```

Clientul normal foloseste automat:

```txt
user1 / pass1
```

Clientul admin foloseste automat:

```txt
admin / admin123
```

Rolul `user` permite:
- upload fisier
- download raport

Rolul `admin` permite:
- statistici server
- afisare loguri
- listare fisiere uploadate
- listare rapoarte generate

---

## Rulare analyzer CLI

### Analiza fisier valid

```bash
./analyzer -f tests/sample.c -v
```

sau:

```bash
./main -f tests/sample.c -v
```

### Analiza fisier cu warning-uri

```bash
./analyzer -f tests/bad.c -v
```

### Rulare cu variabila de mediu

```bash
export SOURCE_FILE=tests/sample.c
./analyzer
```

---

## Rulare server

Intr-un terminal separat:

```bash
./server
```

Serverul porneste pe:

```txt
127.0.0.1:8080
```

Serverul accepta conexiuni TCP si trateaza clientii concurent folosind `fork()`.

---

## Rulare client normal C

Pentru upload si analiza fisier:

```bash
./client tests/sample.c
```

sau:

```bash
./client upload tests/sample.c
```

Pentru fisier cu warning-uri:

```bash
./client tests/bad.c
```

---

## Download raport cu clientul C

Dupa ce serverul a generat un raport in directorul `reports/`, clientul il poate descarca folosind:

```bash
./client download sample_report.txt
```

sau:

```bash
./client download bad_report.txt
```

Raportul va fi salvat local in directorul:

```txt
downloads/
```

Prin aceasta comanda, proiectul suporta transfer server -> client.

---

## Rulare client admin C

Intr-un terminal separat:

```bash
./admin_client
```

Admin clientul afiseaza un meniu:

```txt
1. Server stats
2. Show server logs
3. List uploaded files
4. List reports
5. Quit
```

---

## Rulare client Python

Clientul Python se afla in:

```txt
python_client/client.py
```

Upload si analiza fisier:

```bash
python3 python_client/client.py upload tests/sample.c
```

Download raport:

```bash
python3 python_client/client.py download sample_report.txt
```

Rapoartele descarcate prin clientul Python sunt salvate in:

```txt
downloads_py/
```

---

## Exemple output client

### Fisier valid

```txt
=== Analysis Result ===
Project: PCD T17 Semantic Analyzer
Source file: uploads/sample.c
Diagnostics count: 0
Functions count: 3
Variables count: 8
If statements count: 1
For loops count: 1
While loops count: 0

=== Diagnostics ===
Nu exista diagnostice.

Parse completed.
```

---

### Fisier cu warning-uri

```txt
=== Analysis Result ===
Project: PCD T17 Semantic Analyzer
Source file: uploads/bad.c
Diagnostics count: 1
Functions count: 1
Variables count: 4
If statements count: 0
For loops count: 0
While loops count: 0

=== Diagnostics ===
uploads/bad.c:13:20: warning: variable 'x' is uninitialized when used here [-Wuninitialized]

Parse completed.
```

---

## Fisiere de test

Proiectul include mai multe fisiere de test in directorul `tests/`.

### Fisiere valide

```txt
tests/sample.c
tests/good_math.c
tests/good_while.c
tests/good_nested.c
```

Aceste fisiere ar trebui sa produca:

```txt
Diagnostics count: 0
```

### Fisiere cu warning-uri sau erori intentionate

```txt
tests/bad.c
tests/bad_uninitialized.c
tests/bad_syntax.c
tests/bad_type.c
```

Aceste fisiere sunt folosite pentru a demonstra ca analyzer-ul detecteaza probleme in cod.

Exemple:
- variabila folosita neinitializata
- functie non-void fara valoare returnata
- conversie incompatibila intre pointer si int

---

## Testare rapida analyzer

```bash
make clean
make

./analyzer -f tests/good_math.c -v
./analyzer -f tests/good_while.c -v
./analyzer -f tests/good_nested.c -v

./analyzer -f tests/bad_uninitialized.c -v
./analyzer -f tests/bad_syntax.c -v
./analyzer -f tests/bad_type.c -v
```

Pentru fisierele `good_*`, rezultatul asteptat este `Diagnostics count: 0`.

Pentru fisierele `bad_*`, warning-urile sau erorile sunt intentionate.

---

## Testare rapida client-server

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

## Directoare generate la rulare

La rulare, serverul si clientii pot genera urmatoarele directoare:

```txt
uploads/
reports/
logs/
downloads/
downloads_py/
```

Rolul lor:

- `uploads/` contine fisierele trimise de client catre server
- `reports/` contine rapoartele generate de analyzer
- `logs/` contine logurile serverului si statisticile persistente
- `downloads/` contine rapoartele descarcate de clientul C
- `downloads_py/` contine rapoartele descarcate de clientul Python

Aceste directoare sunt generate la runtime si sunt ignorate de git.

---

## Protocol

Protocolul client-server este descris in:

```txt
docs/protocol.md
```

Comenzi implementate:
- `LOGIN <username> <password>`
- `UPLOAD <filename> <size>`
- `DOWNLOAD_REPORT <report_name>`
- `STATS`
- `LOGS`
- `LIST_UPLOADS`
- `LIST_REPORTS`
- `RESULT <size>`
- `FILE <filename> <size>`

---

## OpenAPI

Specificatia OpenAPI este disponibila in:

```txt
docs/openapi.yaml
```

Aceasta descrie o posibila extensie REST pentru proiect, cu endpoint-uri precum:
- `POST /auth/login`
- `POST /analyze`
- `GET /admin/stats`
- `GET /admin/logs`
- `GET /admin/uploads`
- `GET /admin/reports`
- `GET /reports/{reportId}`
- `GET /health`

---

## Tehnologii utilizate

- C
- Python
- gcc
- make
- libconfig
- libclang
- getopt
- TCP sockets
- fork / exec / pipe
- fisiere de configurare
- logging in fisier

---

## Observatii

Proiectul contine o implementare functionala pentru analiza semantica de baza a codului sursa C/C++. Serverul poate primi fisiere de la clienti, poate rula analiza intr-un proces separat si poate returna raportul rezultat.

Implementarea actuala include:
- client normal C
- client admin C
- client Python
- server TCP concurent
- autentificare simpla
- transfer bidirectional de fisiere
- loguri si rapoarte generate automat