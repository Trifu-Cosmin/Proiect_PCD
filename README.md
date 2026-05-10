# PCD T17 - Semantic Analyzer

## Descriere proiect

Acest proiect implementeaza o aplicatie in limbajul C pentru analiza semantica a codului sursa C/C++.

Aplicatia foloseste:
- libconfig pentru citirea configuratiei
- libclang pentru analiza codului sursa
- getopt pentru parsarea argumentelor din linia de comanda
- socket-uri TCP pentru comunicarea client-server
- fork, exec si pipe pentru rularea analyzer-ului intr-un proces separat

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
- Server TCP minimal
- Client normal care trimite fisiere catre server
- Admin client pentru statistici server
- Protocol documentat in `docs/protocol.md`
- Specificatie OpenAPI in `docs/openapi.yaml`

---

## Structura proiectului

```txt
Proiect_PCD/
├── config/
│   └── app.cfg
├── docs/
│   ├── openapi.yaml
│   └── protocol.md
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

### Terminal 2 - client normal

```bash
./client tests/sample.c
```

Pentru fisier cu warning-uri:

```bash
./client tests/bad.c
```

### Terminal 3 - admin client

```bash
./admin_client
```

---

## Exemplu output client

```txt
=== Analysis Result ===
Project: PCD T17 Semantic Analyzer
Source file: uploads/sample.c
Diagnostics count: 1
Functions count: 3
Variables count: 8
If statements count: 1
For loops count: 1
While loops count: 0

=== Diagnostics ===
uploads/sample.c:39:2: warning: no newline at end of file [-Wnewline-eof]

Parse completed.
```

---

## Configurare

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

## Protocol

Protocolul client-server este descris in:

```txt
docs/protocol.md
```

Comenzi implementate:
- `UPLOAD <filename> <size>`
- `STATS`
- `RESULT <size>`

---

## OpenAPI

Specificatia OpenAPI este disponibila in:

```txt
docs/openapi.yaml
```

Aceasta descrie o posibila extensie REST pentru proiect, cu endpoint-uri precum:
- `POST /analyze`
- `GET /admin/stats`
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

---

## Posibile extensii

- autentificare pentru user/admin
- logging server
- job queue
- client Python
- transfer bidirectional de fisiere
- salvare rapoarte in director separat
- operatii admin suplimentare
- server concurent pentru mai multi clienti
- API REST real