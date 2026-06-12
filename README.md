# T17 - Analiza Semantica Cod Sursa

Proiect pentru disciplina **Programare Concurenta si Distribuita**.

Aplicatia implementeaza un sistem client-server pentru analiza statica a fisierelor sursa C. Clientii pot trimite fisiere catre server, serverul ruleaza un analyzer bazat pe `libclang`, genereaza rapoarte text si permite administrarea printr-un client separat.

---

## Functionalitati principale

- server TCP INET pentru clientii normali;
- socket UNIX pentru clientul admin;
- client normal in C;
- client admin in C;
- client remote alternativ in Python;
- autentificare user/admin;
- upload fisiere C catre server;
- analiza fisierelor cu `libclang`;
- generare rapoarte in `reports/`;
- download rapoarte de la server;
- loguri in `logs/server.log`;
- statistici in `logs/stats.txt`;
- job tracking in `logs/jobs.log`;
- monitorizare `reports/` cu `inotify`;
- server concurent cu `select()` si `fork()`.

---

## Arhitectura

```txt
Client C normal      \
                     -> INET TCP 127.0.0.1:8080 -> Server -> Analyzer -> Report
Client Python        /

Admin Client C       -> UNIX socket /tmp/t17_admin.sock -> Server

Watch Reports        -> inotify -> reports/
```

Serverul asculta simultan pe doua socket-uri:

- `127.0.0.1:8080` pentru clientii normali;
- `/tmp/t17_admin.sock` pentru clientul admin.

Serverul foloseste `select()` pentru a monitoriza ambele socket-uri si `fork()` pentru a trata fiecare client intr-un proces separat.

---

## Tehnologii folosite

- C
- Python
- TCP sockets
- UNIX domain sockets
- `select()`
- `fork()`
- `exec()`
- `pipe()`
- `waitpid()`
- `inotify`
- `libclang`
- `libconfig`
- Makefile

---

## Structura proiectului

```txt
Proiect_PCD/
├── Makefile
├── README.md
├── README.tex
├── config/
│   ├── app.cfg
│   └── users.cfg
├── docs/
│   ├── SRS_SDD.tex
│   ├── protocol.tex
│   └── openapi.yaml
├── python_client/
│   └── client.py
├── src/
│   ├── analyzer.c
│   ├── main.c
│   ├── server.c
│   ├── client.c
│   ├── admin_client.c
│   └── watch_reports.c
├── tests/
│   ├── sample.c
│   ├── bad.c
│   ├── bad_syntax.c
│   ├── bad_type.c
│   ├── bad_uninitialized.c
│   ├── good_math.c
│   ├── good_nested.c
│   └── good_while.c
├── uploads/
├── reports/
├── logs/
├── downloads/
└── downloads_py/
```

Directoarele `uploads/`, `reports/`, `logs/`, `downloads/` si `downloads_py/` sunt generate la runtime si nu trebuie urcate pe Git.

---

## Dependinte

Pe Ubuntu / WSL:

```bash
sudo apt update
sudo apt install build-essential libclang-dev libconfig-dev python3
```

Proiectul foloseste `libclang` si `libconfig`.

In Makefile este folosita calea pentru LLVM 18:

```txt
/usr/lib/llvm-18/include
/usr/lib/llvm-18/lib
```

Daca sistemul are alta versiune LLVM, aceste cai trebuie ajustate in `Makefile`.

---

## Compilare

```bash
make clean
make
```

Executabile generate:

```txt
analyzer
main
server
client
admin_client
watch_reports
```

---

## Rulare server

Pornire server:

```bash
./server
```

Output asteptat:

```txt
INET server running on 127.0.0.1:8080
UNIX admin socket running on /tmp/t17_admin.sock
Waiting for clients with select()...
```

Serverul trebuie lasat pornit intr-un terminal separat.

---

## Client normal C

Upload si analiza fisier:

```bash
./client tests/sample.c
```

Alt exemplu:

```bash
./client tests/bad_type.c
```

Download raport:

```bash
./client download sample_report.txt
```

Rapoartele descarcate de clientul C sunt salvate in:

```txt
downloads/
```

---

## Client Python

Upload fisier:

```bash
python3 python_client/client.py upload tests/sample.c
```

Download raport:

```bash
python3 python_client/client.py download sample_report.txt
```

Rapoartele descarcate de clientul Python sunt salvate in:

```txt
downloads_py/
```

---

## Admin client

Pornire admin client:

```bash
./admin_client
```

Admin clientul se conecteaza prin socket UNIX:

```txt
/tmp/t17_admin.sock
```

Meniu admin:

```txt
1. Server stats
2. Show server logs
3. List uploaded files
4. List reports
5. Server status
6. Clear server logs
7. Delete report
8. List users
9. Show analysis jobs
10. Quit
```

Comenzi admin implementate:

| Optiune | Comanda trimisa | Descriere |
|---|---|---|
| 1 | `STATS` | Afiseaza statistici despre fisiere analizate |
| 2 | `LOGS` | Afiseaza logurile serverului |
| 3 | `LIST_UPLOADS` | Listeaza fisierele incarcate |
| 4 | `LIST_REPORTS` | Listeaza rapoartele generate |
| 5 | `SERVER_STATUS` | Afiseaza configuratia serverului |
| 6 | `CLEAR_LOGS` | Goleste logurile serverului |
| 7 | `DELETE_REPORT <name>` | Sterge un raport |
| 8 | `LIST_USERS` | Listeaza userii fara parole |
| 9 | `JOBS` | Afiseaza istoricul joburilor |
| 10 | `QUIT` | Inchide conexiunea |

---

## Monitorizare rapoarte cu inotify

Programul `watch_reports` monitorizeaza directorul `reports/`.

Pornire:

```bash
./watch_reports
```

Apoi, intr-un alt terminal, se ruleaza un client:

```bash
./client tests/sample.c
```

Output posibil in watcher:

```txt
Watching directory: reports
Waiting for report changes...
[2026-06-12 12:26:09] report created: sample_report.txt
[2026-06-12 12:26:09] report modified: sample_report.txt
[2026-06-12 12:26:09] report written: sample_report.txt
```

---

## Analyzer

Analyzer-ul foloseste `libclang` pentru a analiza fisiere C.

Exemplu rulare directa:

```bash
./analyzer -f tests/sample.c -v
```

Analyzer-ul extrage informatii precum:

- numar functii;
- numar variabile;
- numar instructiuni `if`;
- numar instructiuni `for`;
- numar instructiuni `while`;
- diagnostice `libclang`;
- warning-uri si erori.

Serverul ruleaza analyzer-ul folosind `fork()`, `exec()` si `pipe()`, apoi trimite rezultatul inapoi catre client.

---

## Autentificare

Utilizatorii sunt definiti in:

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

Clientul normal foloseste rolul `user`.

Clientul admin foloseste rolul `admin` si are voie sa execute comenzile de administrare doar prin socket UNIX.

---

## Protocol

Fiecare conexiune incepe cu:

```txt
LOGIN <username> <password>
```

Exemplu:

```txt
LOGIN user1 pass1
```

Raspuns server:

```txt
RESULT <size>
OK role=user
```

Pentru upload:

```txt
UPLOAD <filename> <size>
<file_content>
```

Pentru download raport:

```txt
DOWNLOAD_REPORT <report_name>
```

Pentru raspuns text:

```txt
RESULT <size>
<body>
```

Pentru raspuns fisier:

```txt
FILE <filename> <size>
<file_content>
```

Protocolul complet este descris in:

```txt
docs/protocol.tex
```

---

## Job tracking

La fiecare upload si analiza, serverul scrie statusul jobului in:

```txt
logs/jobs.log
```

Exemplu:

```txt
[2026-06-12 12:17:08] sample.c QUEUED
[2026-06-12 12:17:08] sample.c RUNNING
[2026-06-12 12:17:09] sample.c DONE
[2026-06-12 12:17:09] bad_type.c QUEUED
[2026-06-12 12:17:09] bad_type.c RUNNING
[2026-06-12 12:17:09] bad_type.c DONE
```

Joburile pot fi vazute din admin client cu optiunea:

```txt
9. Show analysis jobs
```

---

## Loguri si statistici

Logurile serverului sunt salvate in:

```txt
logs/server.log
```

Statisticile sunt salvate in:

```txt
logs/stats.txt
```

Adminul poate vedea aceste informatii direct din `admin_client`.

---

## Testare rapida

Terminal 1:

```bash
./server
```

Terminal 2:

```bash
./watch_reports
```

Terminal 3:

```bash
./client tests/sample.c
./client tests/bad_type.c
./client download sample_report.txt
```

Terminal 4:

```bash
./admin_client
```

In admin client se pot testa:

```txt
1. Server stats
4. List reports
5. Server status
8. List users
9. Show analysis jobs
```

---

## Exemple de fisiere de test

Fisiere bune:

```txt
tests/sample.c
tests/good_math.c
tests/good_nested.c
tests/good_while.c
```

Fisiere cu probleme:

```txt
tests/bad.c
tests/bad_syntax.c
tests/bad_type.c
tests/bad_uninitialized.c
```

---

## Curatare

Curatare executabile si `uploads/`:

```bash
make clean
```

Curatare manuala completa:

```bash
rm -rf uploads reports logs downloads downloads_py
rm -f /tmp/t17_admin.sock
```

---

## Documentatie

Documentatia oficiala este in LaTeX:

```txt
docs/SRS_SDD.tex
docs/protocol.tex
README.tex
```

Generare PDF:

```bash
pdflatex README.tex
pdflatex -output-directory=docs docs/SRS_SDD.tex
pdflatex -output-directory=docs docs/protocol.tex
```

README-ul principal pentru GitHub ramane:

```txt
README.md
```

---

## Functionalitati bifate pentru proiect

- socket INET TCP;
- socket UNIX / local;
- comunicare client-server;
- client normal;
- client admin;
- client in alta limba, Python;
- transfer fisiere client -> server;
- transfer fisiere server -> client;
- autentificare;
- roluri user/admin;
- server concurent cu `fork()`;
- monitorizare socket-uri cu `select()`;
- executie analyzer cu `exec()`;
- comunicare cu analyzer prin `pipe()`;
- job tracking;
- monitorizare cu `inotify`;
- documentatie LaTeX.

---

## Concluzie

Proiectul implementeaza o aplicatie distribuita locala pentru analiza semantica a codului sursa C. Sistemul foloseste mai multe concepte importante din programarea concurenta si distribuita: socket-uri INET, socket-uri UNIX, procese, pipe-uri, `select()`, autentificare, transfer de fisiere si monitorizare cu `inotify`.