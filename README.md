# PCD T17 - Semantic Analyzer

## Descriere proiect

Acest proiect implementeaza o aplicatie in limbajul C pentru analiza semantica a codului sursa (C/C++), utilizand biblioteci externe.

Aplicatia:
- citeste configuratia dintr-un fisier folosind libconfig
- primeste un fisier sursa prin argument CLI sau variabila de mediu
- analizeaza codul folosind libclang
- extrage informatii despre structura codului
- afiseaza diagnostice (erori si warnings)

Proiectul reprezinta o baza pentru extindere ulterioara catre o arhitectura client-server.

---

## Functionalitati

- Citire configurare din config/app.cfg
- Parsare argumente din linia de comanda (getopt)
- Analiza cod folosind AST (libclang)
- Afisare statistici:
  - numar functii
  - numar variabile
  - numar instructiuni if
  - numar bucle for
  - numar bucle while
- Afisare diagnostice (optional)

---

## Structura proiect
Proiect_PCD/
│
├── src/
│ └── main.c
│
├── config/
│ └── app.cfg
│
├── tests/
│ ├── sample.c
│ └── bad.c
│
├── docs/
│ └── openapi.yaml
│
├── Makefile
└── README.md


---

## Cerinte

- Linux / WSL
- gcc
- make
- libconfig
- libclang

---

## Instalare dependente (WSL / Ubuntu)

```bash
sudo apt update
sudo apt install build-essential libconfig-dev libclang-dev

#compilare
make clean
make

#Rulare
#Rulare cu argument CLI
./main -f tests/sample.c

#Rulare cu diagnostice detaliate
./main -f tests/bad.c -v

#Rulare cu variabile de mediu
export SOURCE_FILE=tests/sample.c
./main

#Exemple output
#Cod Corect
=== Analysis Result ===
Project: PCD T17 Semantic Analyzer
Source file: tests/sample.c
Diagnostics count: 0
Functions count: 3
Variables count: 8
If statements count: 1
For loops count: 1
While loops count: 0

#Cod cu erori
Diagnostics count: 1
error: expected expression

#Configurare(app.cfg)
project_name = "PCD T17 Semantic Analyzer";

clang_args = (
  "-std=c11",
  "-Wall",
  "-Wextra",
  "-pedantic"
);

show_diagnostics = true;

#Tehnologii utilizate
C (C11)
libconfig
libclang
getopt
Makefile

#Posibile extensii
server TCP
client CLI sau Python
API REST (OpenAPI)
analiza mai avansata a codului