# PCD T17 - Semantic Analyzer

## Descriere proiect

Acest proiect implementeaza o aplicatie in limbajul C pentru analiza semantica a codului sursa C/C++.

Aplicatia foloseste:
- libconfig pentru citirea configuratiei
- libclang pentru analiza codului sursa
- getopt pentru parsarea argumentelor din linia de comanda
- socket-uri TCP pentru comunicarea client-server
- fork, exec si pipe pentru rularea analyzer-ului intr-un proces separat
- autentificare simpla pentru separarea userilor normali de admin

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
- Client normal
- Client admin
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
├── src/
│   ├── analyzer.c
│   ├── main.c
│   ├── server.c
│   ├── client.c
│   └── admin_client.c
├── tests/
│   ├── sample.c
│   └── bad.c
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

---

## Instalare dependente pe WSL / Ubuntu

```bash
sudo apt update
sudo apt install build-essential libconfig-dev libclang-dev clang -y
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

## Rulare server si client

### Terminal 1 - server

```bash
./server
```

Serverul porneste pe:

```txt
127.0.0.1:8080
```

---

### Terminal 2 - client normal

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

### Download raport de la server

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

Prin aceasta comanda, proiectul suporta si transfer server -> client.

---

### Terminal 3 - admin client

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

## Directoare generate la rulare

La rulare, serverul poate genera urmatoarele directoare:

```txt
uploads/
reports/
logs/
downloads/
```

Rolul lor:

- `uploads/` contine fisierele trimise de client catre server
- `reports/` contine rapoartele generate de analyzer
- `logs/` contine logurile serverului
- `downloads/` contine rapoartele descarcate de client

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
- `GET /reports/{reportId}`
- `GET /health`

---

## Tehnologii utilizate

- C
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

## Testare rapida

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

---

## Posibile extensii

- job queue
- client Python
- server concurent pentru mai multi clienti
- transfer fisiere mari in chunks
- API REST real
- operatii admin suplimentare
- stergere rapoarte
- curatare loguri
- istoric analize